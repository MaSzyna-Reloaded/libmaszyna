#include "SceneryStreamingServer.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "utils/ProcessMemory.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>

namespace godot {
    const char *SceneryStreamingServer::streaming_builds_started_signal = "streaming_builds_started";
    const char *SceneryStreamingServer::streaming_builds_finished_signal = "streaming_builds_finished";
    const char *SceneryStreamingServer::streaming_camera_chunk_changed_signal = "streaming_camera_chunk_changed";
    const char *SceneryStreamingServer::streaming_camera_changed_signal = "streaming_camera_changed";
    const char *SceneryStreamingServer::chunk_cleared_signal = "chunk_cleared";

    void SceneryStreamingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("owner_create", "name", "preload", "build", "clear"), &SceneryStreamingServer::owner_create);
        ClassDB::bind_method(
                D_METHOD("stream_register", "owner", "rid", "position", "range_end"),
                &SceneryStreamingServer::stream_register);
        ClassDB::bind_method(D_METHOD("stream_free", "stream_rid"), &SceneryStreamingServer::stream_free);
        ClassDB::bind_method(D_METHOD("owner_rebuild", "owner"), &SceneryStreamingServer::owner_rebuild);
        ClassDB::bind_method(
                D_METHOD("provider_register", "provider", "scenario"), &SceneryStreamingServer::provider_register);
        ClassDB::bind_method(D_METHOD("provider_free", "provider"), &SceneryStreamingServer::provider_free);
        ClassDB::bind_method(
                D_METHOD("content_set_consumer", "kind", "adopt", "release"),
                &SceneryStreamingServer::content_set_consumer);
        ClassDB::bind_method(
                D_METHOD("provider_set_scenario", "provider", "scenario"),
                &SceneryStreamingServer::provider_set_scenario);
        ClassDB::bind_method(
                D_METHOD("streaming_set_enabled", "enabled"), &SceneryStreamingServer::streaming_set_enabled);
        ClassDB::bind_method(D_METHOD("streaming_is_enabled"), &SceneryStreamingServer::streaming_is_enabled);
        ClassDB::bind_method(
                D_METHOD("streaming_set_camera", "camera_id"), &SceneryStreamingServer::streaming_set_camera);
        ClassDB::bind_method(
                D_METHOD("streaming_set_anchor_position", "position"),
                &SceneryStreamingServer::streaming_set_anchor_position);
        ClassDB::bind_method(D_METHOD("streaming_clear_anchor"), &SceneryStreamingServer::streaming_clear_anchor);
        ClassDB::bind_method(D_METHOD("streaming_drain"), &SceneryStreamingServer::streaming_drain);
        ClassDB::bind_method(
                D_METHOD("streaming_get_draw_distance"), &SceneryStreamingServer::streaming_get_draw_distance);
        ClassDB::bind_method(
                D_METHOD("streaming_get_camera_position"), &SceneryStreamingServer::streaming_get_camera_position);
        ClassDB::bind_method(D_METHOD("streaming_has_camera"), &SceneryStreamingServer::streaming_has_camera);
        ClassDB::bind_method(
                D_METHOD("streaming_get_camera_chunk"), &SceneryStreamingServer::streaming_get_camera_chunk);
        ClassDB::bind_method(D_METHOD("chunk_get_rids", "chunk"), &SceneryStreamingServer::chunk_get_rids);
        ClassDB::bind_method(D_METHOD("streaming_is_building"), &SceneryStreamingServer::streaming_is_building);
        ClassDB::bind_method(
                D_METHOD("area_is_ready", "chunk_radius"), &SceneryStreamingServer::area_is_ready, DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("area_get_pending_count", "chunk_radius"), &SceneryStreamingServer::area_get_pending_count,
                DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("streaming_get_streamed_count"), &SceneryStreamingServer::streaming_get_streamed_count);
        ClassDB::bind_method(D_METHOD("streaming_get_statistics"), &SceneryStreamingServer::streaming_get_statistics);

        ADD_SIGNAL(MethodInfo(streaming_builds_started_signal));
        ADD_SIGNAL(MethodInfo(streaming_builds_finished_signal));
        ADD_SIGNAL(MethodInfo(streaming_camera_chunk_changed_signal, PropertyInfo(Variant::VECTOR2I, "chunk")));
        ADD_SIGNAL(MethodInfo(streaming_camera_changed_signal));
        // the last built piece of the chunk cleared as the camera went away
        ADD_SIGNAL(MethodInfo(chunk_cleared_signal, PropertyInfo(Variant::VECTOR2I, "chunk")));
    }

    SceneryStreamingServer::SceneryStreamingServer() {
        semaphore.instantiate();
        preload_queue.instantiate();
        draw_distance = ProjectSettings::get_singleton()->get_setting(DRAW_DISTANCE_SETTING, DEFAULT_DRAW_DISTANCE_M);
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &SceneryStreamingServer::_on_project_settings_changed));
    }

    /// Every piece is ranged anew - the next pass builds what came into range and clears what left it
    void SceneryStreamingServer::_on_project_settings_changed() {
        const float distance =
                ProjectSettings::get_singleton()->get_setting(DRAW_DISTANCE_SETTING, DEFAULT_DRAW_DISTANCE_M);
        MutexLock lock(mutex);
        if (distance == draw_distance) {
            return;
        }
        draw_distance = distance;
        for (KeyValue<Vector2i, Chunk> &item: chunks) {
            for (Entry &entry: item.value.entries) {
                entry.range_end = _get_range_end(entry.declared_range);
            }
            item.value.dirty = true;
        }
        content_dirty = true;
    }

    /// A piece's own range, never past the draw distance; none of its own is the draw distance
    float SceneryStreamingServer::_get_range_end(const float p_declared_range) const {
        return p_declared_range > 0.0 && p_declared_range < draw_distance ? p_declared_range : draw_distance;
    }

    /// The worker finishes the pass it is in before it is joined
    SceneryStreamingServer::~SceneryStreamingServer() {
        streaming_set_camera(0);
        streaming_drain();
    }

    void SceneryStreamingServer::streaming_drain() {
        if (worker.is_null()) {
            return;
        }
        {
            MutexLock lock(mutex);
            exiting = true;
        }
        // the preloads first: the pass waiting for them then gives up at once instead of going on
        // batch by batch, and they may be scripts, none of which may still run when the scripts go
        preload_queue->drain();
        semaphore->post();
        worker->wait_to_finish();
        worker.unref();
        // a drained queue runs nothing more: the next scenery gets a queue of its own
        preload_queue.instantiate();
        // the thread is joined, so the next plan may start a new one
        MutexLock lock(mutex);
        exiting = false;
        planning = false;
    }

    Vector2i SceneryStreamingServer::_get_chunk_key(const Vector3 &p_origin) {
        return Vector2i(
                static_cast<int32_t>(Math::floor(p_origin.x / CHUNK_SIZE_M)),
                static_cast<int32_t>(Math::floor(p_origin.z / CHUNK_SIZE_M)));
    }

    /// Distance from the camera to the nearest edge of the chunk (0 inside it), on the XZ plane -
    /// height does not tell how far a scenery piece is
    float SceneryStreamingServer::_get_chunk_distance(const Vector2i &p_key, const Vector3 &p_position) {
        const double min_x = static_cast<double>(p_key.x) * CHUNK_SIZE_M;
        const double min_z = static_cast<double>(p_key.y) * CHUNK_SIZE_M;
        const double x = MAX(0.0, MAX(min_x - p_position.x, p_position.x - (min_x + CHUNK_SIZE_M)));
        const double z = MAX(0.0, MAX(min_z - p_position.z, p_position.z - (min_z + CHUNK_SIZE_M)));
        return static_cast<float>(Math::sqrt((x * x) + (z * z)));
    }

    /// Drops the entries of freed pieces and restores the range order and the streamed count
    void SceneryStreamingServer::_sort_chunk(Chunk &p_chunk) {
        Vector<Entry> kept;
        for (const Entry &entry: p_chunk.entries) {
            if (entry_locations.has(entry.stream_rid)) {
                kept.push_back(entry);
            }
        }
        kept.sort_custom<RangeComparator>();
        p_chunk.built_count = 0;
        for (int index = 0; index < kept.size(); index++) {
            const Entry &entry = kept[index];
            entry_locations.getptr(entry.stream_rid)->index = index;
            if (entry.built) {
                p_chunk.built_count++;
            }
        }
        p_chunk.entries = kept;
        p_chunk.dirty = false;
    }

    /// Registers a rendering server with the streaming. [param preload] is called on the worker
    /// thread and its result is passed to [param build]; pass an invalid Callable when there is
    /// nothing to prepare off the main thread.
    int SceneryStreamingServer::owner_create(
            const String &p_name, const Callable &p_preload, const Callable &p_build, const Callable &p_clear) {
        MutexLock lock(mutex);
        Owner owner;
        owner.name = p_name;
        owner.preload = p_preload;
        owner.build = p_build;
        owner.clear = p_clear;
        owners.push_back(owner);
        owner_times.push_back(WorkTime());
        const int index = static_cast<int>(owners.size() - 1);
        // the pieces are logged by the owner's index (--verbose), its name only here
        UtilityFunctions::print("[SceneryStreaming] owner ", index, ": ", p_name);
        return index;
    }

    /// Clears every piece of the owner built so far, and the next plan builds again - preload
    /// included - those it still wants: what they were built of has changed (the game's data was
    /// read again). A piece out of range is built from the new data whenever it comes into range.
    void SceneryStreamingServer::owner_rebuild(const int p_owner) {
        Vector<RID> cleared;
        Callable clear;
        {
            MutexLock lock(mutex);
            ERR_FAIL_INDEX(p_owner, owners.size());
            clear = owners[p_owner].clear;
            for (KeyValue<Vector2i, Chunk> &item: chunks) {
                for (Entry &entry: item.value.entries) {
                    // a freed piece stays in its chunk until the chunk is sorted again
                    if (entry.owner == p_owner && entry.built && entry_locations.has(entry.stream_rid)) {
                        entry.built = false;
                        entry.queued_revision = 0;
                        item.value.built_count--;
                        cleared.push_back(entry.user_rid);
                    }
                }
            }
            content_dirty = true;
        }
        if (clear.is_valid()) {
            for (const RID &user_rid: cleared) {
                clear.call(user_rid);
            }
        }
    }

    /// The cells, their overhang and the kinds are asked for once: a provider's map does not change
    RID
    SceneryStreamingServer::provider_register(const Ref<SceneryStreamingProvider> &p_provider, const RID &p_scenario) {
        ERR_FAIL_COND_V(p_provider.is_null(), RID());
        Provider provider;
        provider.provider = p_provider;
        provider.scenario = p_scenario;
        for (const int32_t kind: p_provider->get_content_kinds()) {
            ERR_CONTINUE(kind < 0 || kind >= CONTENT_KIND_COUNT);
            provider.kinds.push_back(static_cast<ContentKind>(kind));
        }
        const TypedArray<Vector2i> cells = p_provider->get_chunk_cells();
        for (int64_t index = 0; index < cells.size(); index++) {
            const Vector2i cell = cells[index];
            ProviderCell &state = provider.cells[cell];
            state.overhang = MAX(0.0F, p_provider->chunk_get_overhang(cell));
            provider.max_overhang = MAX(provider.max_overhang, state.overhang);
        }
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        MutexLock lock(mutex);
        providers[rid] = provider;
        content_dirty = true;
        return rid;
    }

    /// What the provider supplied goes at once; its loads still in flight are dropped
    void SceneryStreamingServer::provider_free(const RID &p_provider) {
        std::array<Vector<RID>, CONTENT_KIND_COUNT> released;
        {
            MutexLock lock(mutex);
            Provider *provider = providers.getptr(p_provider);
            if (provider == nullptr) {
                return;
            }
            for (const Vector2i &cell: provider->adopted_cells) {
                const ProviderCell &state = provider->cells[cell];
                for (int kind = 0; kind < CONTENT_KIND_COUNT; kind++) {
                    released[kind].append_array(state.adopted_rids[kind]);
                }
            }
            providers.erase(p_provider);
            freed_pending = true;
            content_dirty = true;
        }
        _release_content(released);
    }

    void SceneryStreamingServer::provider_set_scenario(const RID &p_provider, const RID &p_scenario) {
        std::array<Vector<RID>, CONTENT_KIND_COUNT> released;
        {
            MutexLock lock(mutex);
            Provider *provider = providers.getptr(p_provider);
            ERR_FAIL_NULL(provider);
            provider->scenario = p_scenario;
            for (const Vector2i &cell: provider->adopted_cells) {
                ProviderCell &state = provider->cells[cell];
                for (int kind = 0; kind < CONTENT_KIND_COUNT; kind++) {
                    released[kind].append_array(state.adopted_rids[kind]);
                    state.adopted_rids[kind].clear();
                }
                state.adopted = false;
            }
            provider->adopted_cells.clear();
            content_dirty = true;
        }
        _release_content(released);
    }

    void SceneryStreamingServer::content_set_consumer(
            const SceneryStreamingProvider::ContentKind p_kind, const Callable &p_adopt, const Callable &p_release) {
        ERR_FAIL_INDEX(p_kind, CONTENT_KIND_COUNT);
        MutexLock lock(mutex);
        consumers[p_kind] = Consumer{p_adopt, p_release};
    }

    /// Called without the mutex: a consumer frees the pieces it registered
    void SceneryStreamingServer::_release_content(const std::array<Vector<RID>, CONTENT_KIND_COUNT> &p_rids) {
        for (int kind = 0; kind < CONTENT_KIND_COUNT; kind++) {
            Callable release;
            {
                MutexLock lock(mutex);
                release = consumers[kind].release;
            }
            if (!release.is_valid()) {
                continue;
            }
            for (const RID &rid: p_rids[kind]) {
                release.call(rid);
            }
        }
    }

    /// Registers one piece of an owner. [param range_end] of 0 or less means the piece declares no
    /// range of its own and is streamed up to the global draw distance.
    RID SceneryStreamingServer::stream_register(
            const int p_owner, const RID &p_user_rid, const Vector3 &p_position, const float p_range_end) {
        MutexLock lock(mutex);
        ERR_FAIL_INDEX_V(p_owner, owners.size(), RID());
        Entry entry;
        entry.owner = p_owner;
        entry.stream_rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        entry.user_rid = p_user_rid;
        entry.declared_range = p_range_end;
        entry.range_end = _get_range_end(p_range_end);
        const Vector2i key = _get_chunk_key(p_position);
        Chunk &chunk = chunks[key];
        chunk.entries.push_back(entry);
        chunk.dirty = true;
        entry_locations[entry.stream_rid] = EntryLocation{key, static_cast<int>(chunk.entries.size() - 1)};
        content_dirty = true;
        return entry.stream_rid;
    }

    /// The entry itself is dropped by the next _sort_chunk() - freeing a whole scenery stays O(1)
    /// per piece instead of searching the chunk for every one of them. The owner's clear callable
    /// is not called: whoever frees the piece frees its content too.
    void SceneryStreamingServer::stream_free(const RID &p_stream_rid) {
        MutexLock lock(mutex);
        const EntryLocation *location = entry_locations.getptr(p_stream_rid);
        if (location == nullptr) {
            return;
        }
        Entry *entry = _get_entry(p_stream_rid);
        if (entry != nullptr && entry->wanted_revision == target_revision && !entry->built) {
            pending_build_count--;
        }
        chunks[location->chunk].dirty = true;
        entry_locations.erase(p_stream_rid);
        freed_pending = true;
        content_dirty = true;
    }

    SceneryStreamingServer::Entry *SceneryStreamingServer::_get_entry(const RID &p_stream_rid) {
        const EntryLocation *location = entry_locations.getptr(p_stream_rid);
        if (location == nullptr) {
            return nullptr;
        }
        Chunk *chunk = chunks.getptr(location->chunk);
        if (chunk == nullptr) {
            return nullptr;
        }
        return &chunk->entries.write[location->index];
    }

    /// Queued work of pieces freed since the last frame. Unloading a scenery frees thousands of
    /// them at once, and their builds would otherwise keep running - eating the whole frame budget
    /// while the game is already showing the next screen, and delaying the next scenery's own work.
    /// Must be called with the mutex held.
    void SceneryStreamingServer::_drop_freed_work() {
        Vector<PendingBuild> builds;
        for (const PendingBuild &pending: pending_builds) {
            if (entry_locations.has(pending.stream_rid)) {
                builds.push_back(pending);
            }
        }
        pending_builds = builds;
        builds.clear();
        for (const PendingBuild &pending: pending_prefetches) {
            if (entry_locations.has(pending.stream_rid)) {
                builds.push_back(pending);
            }
        }
        pending_prefetches = builds;
        builds.clear();
        for (const PendingBuild &pending: planned_builds) {
            if (entry_locations.has(pending.stream_rid)) {
                builds.push_back(pending);
            }
        }
        planned_builds = builds;
        Vector<PendingClear> clears;
        for (const PendingClear &pending: pending_clears) {
            if (entry_locations.has(pending.stream_rid)) {
                clears.push_back(pending);
            }
        }
        pending_clears = clears;
        clears.clear();
        for (const PendingClear &pending: planned_clears) {
            if (entry_locations.has(pending.stream_rid)) {
                clears.push_back(pending);
            }
        }
        planned_clears = clears;
        Vector<PendingProvide> provides;
        for (const PendingProvide &pending: pending_provides) {
            if (providers.has(pending.provider_rid)) {
                provides.push_back(pending);
            }
        }
        pending_provides = provides;
        provides.clear();
        for (const PendingProvide &pending: planned_provides) {
            if (providers.has(pending.provider_rid)) {
                provides.push_back(pending);
            }
        }
        planned_provides = provides;
        Vector<PendingWithdraw> withdraws;
        for (const PendingWithdraw &pending: pending_withdraws) {
            if (providers.has(pending.provider_rid)) {
                withdraws.push_back(pending);
            }
        }
        pending_withdraws = withdraws;
        withdraws.clear();
        for (const PendingWithdraw &pending: planned_withdraws) {
            if (providers.has(pending.provider_rid)) {
                withdraws.push_back(pending);
            }
        }
        planned_withdraws = withdraws;
        freed_pending = false;
    }

    /// A new camera revision invalidates work which has not reached its owner yet. Already-built
    /// pieces stay alive until the new plan decides whether they are still wanted.
    void SceneryStreamingServer::_drop_stale_work() {
        Vector<PendingBuild> builds;
        for (const PendingBuild &pending: pending_builds) {
            if (pending.revision == target_revision && entry_locations.has(pending.stream_rid)) {
                builds.push_back(pending);
            }
        }
        pending_builds = builds;
        builds.clear();
        for (const PendingBuild &pending: pending_prefetches) {
            if (pending.revision == target_revision && entry_locations.has(pending.stream_rid)) {
                builds.push_back(pending);
            }
        }
        pending_prefetches = builds;
        planned_builds.clear();

        Vector<PendingClear> clears;
        for (const PendingClear &pending: pending_clears) {
            if (pending.revision == target_revision && entry_locations.has(pending.stream_rid)) {
                clears.push_back(pending);
            }
        }
        pending_clears = clears;
        planned_clears.clear();

        Vector<PendingProvide> provides;
        for (const PendingProvide &pending: pending_provides) {
            if (pending.revision == target_revision && providers.has(pending.provider_rid)) {
                provides.push_back(pending);
            }
        }
        pending_provides = provides;
        planned_provides.clear();

        Vector<PendingWithdraw> withdraws;
        for (const PendingWithdraw &pending: pending_withdraws) {
            if (pending.revision == target_revision && providers.has(pending.provider_rid)) {
                withdraws.push_back(pending);
            }
        }
        pending_withdraws = withdraws;
        planned_withdraws.clear();
    }

    /// Camera the streaming follows, by its ObjectID; without one nothing is ever built. Setting the
    /// first camera starts looking at it (_watch_camera()), clearing it (0) stops that and the work - the
    /// main loop does not exist yet when the singleton is created.
    void SceneryStreamingServer::streaming_set_camera(const uint64_t p_camera_id) {
        bool was_streaming;
        bool is_streaming;
        bool changed;
        {
            MutexLock lock(mutex);
            was_streaming = camera_id.is_valid();
            const ObjectID previous_camera_id = camera_id;
            camera_id = ObjectID(p_camera_id);
            changed = camera_id != previous_camera_id;
            is_streaming = camera_id.is_valid();
            filling = is_streaming;
            target_revision++;
            scanned_revision = 0;
            pending_build_count = 0;
            force_plan = is_streaming;
            _drop_stale_work();
        }
        if (changed) {
            emit_signal(streaming_camera_changed_signal);
        }
        if (is_streaming == was_streaming) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr) {
            return;
        }
        if (was_streaming) {
            watch_timer->disconnect("timeout", callable_mp(this, &SceneryStreamingServer::_watch_camera));
            watch_timer.unref();
            _set_applying(false);
            // nothing streams without a camera, and the tick that would announce it has stopped
            _set_building(false);
            return;
        }
        _watch_camera();
    }

    void SceneryStreamingServer::streaming_set_anchor_position(const Vector3 &p_position) {
        MutexLock lock(mutex);
        anchor_position = p_position;
        anchored = true;
    }

    void SceneryStreamingServer::streaming_clear_anchor() {
        MutexLock lock(mutex);
        anchored = false;
    }

    float SceneryStreamingServer::streaming_get_draw_distance() const {
        return draw_distance;
    }

    Vector3 SceneryStreamingServer::streaming_get_camera_position() const {
        ObjectID current_camera_id;
        {
            MutexLock lock(mutex);
            current_camera_id = camera_id;
        }
        const Camera3D *camera = Object::cast_to<Camera3D>(ObjectDB::get_instance(current_camera_id));
        return camera != nullptr && camera->is_inside_tree() ? camera->get_global_position() : last_camera_position;
    }

    bool SceneryStreamingServer::streaming_has_camera() const {
        MutexLock lock(mutex);
        return camera_id.is_valid();
    }

    Vector2i SceneryStreamingServer::streaming_get_camera_chunk() const {
        return camera_chunk;
    }

    TypedArray<RID> SceneryStreamingServer::chunk_get_rids(const Vector2i &p_chunk) const {
        TypedArray<RID> rids;
        MutexLock lock(mutex);
        const Chunk *chunk = chunks.getptr(p_chunk);
        if (chunk == nullptr) {
            return rids;
        }
        for (const Entry &entry: chunk->entries) {
            // a freed piece stays in its chunk until the chunk is sorted again
            if (entry_locations.has(entry.stream_rid)) {
                rids.push_back(entry.user_rid);
            }
        }
        return rids;
    }

    bool SceneryStreamingServer::_is_area_ready_locked(const int p_chunk_radius) const {
        return camera_id.is_valid() && scanned_revision == target_revision &&
               _get_pending_nearby_locked(p_chunk_radius) == 0;
    }

    int SceneryStreamingServer::_get_pending_nearby_locked(const int p_chunk_radius) const {
        int pending = 0;
        const Vector2i camera_key = _get_chunk_key(camera_position);
        for (int x = camera_key.x - p_chunk_radius; x <= camera_key.x + p_chunk_radius; x++) {
            for (int y = camera_key.y - p_chunk_radius; y <= camera_key.y + p_chunk_radius; y++) {
                const Chunk *chunk = chunks.getptr(Vector2i(x, y));
                if (chunk == nullptr) {
                    continue;
                }
                for (const Entry &entry: chunk->entries) {
                    if (entry.wanted_revision == target_revision && !entry.built && !entry.ahead) {
                        pending++;
                    }
                }
            }
        }
        // a provider's cell not supplied yet is content still to come
        for (const KeyValue<RID, Provider> &item: providers) {
            for (int x = camera_key.x - p_chunk_radius; x <= camera_key.x + p_chunk_radius; x++) {
                for (int y = camera_key.y - p_chunk_radius; y <= camera_key.y + p_chunk_radius; y++) {
                    const ProviderCell *cell = item.value.cells.getptr(Vector2i(x, y));
                    if (cell != nullptr && cell->wanted_revision == target_revision && !cell->adopted && !cell->ahead) {
                        pending++;
                    }
                }
            }
        }
        return pending;
    }

    bool SceneryStreamingServer::area_is_ready(const int p_chunk_radius) const {
        MutexLock lock(mutex);
        return _is_area_ready_locked(MAX(0, p_chunk_radius));
    }

    int SceneryStreamingServer::area_get_pending_count(const int p_chunk_radius) const {
        MutexLock lock(mutex);
        return _get_pending_nearby_locked(MAX(0, p_chunk_radius));
    }

    /// Pieces currently built - what the streaming actually keeps alive
    int SceneryStreamingServer::streaming_get_streamed_count() const {
        MutexLock lock(mutex);
        int count = 0;
        for (const KeyValue<Vector2i, Chunk> &item: chunks) {
            count += item.value.built_count;
        }
        return count;
    }

    /// What the streaming is doing right now, for the "Scenery Streaming" debug window
    Dictionary SceneryStreamingServer::streaming_get_statistics() const {
        Dictionary statistics;
        MutexLock lock(mutex);
        int streamed = 0;
        int active_chunks = 0;
        for (const KeyValue<Vector2i, Chunk> &item: chunks) {
            streamed += item.value.built_count;
            if (item.value.built_count > 0) {
                active_chunks++;
            }
        }
        int64_t supplied_cells = 0;
        for (const KeyValue<RID, Provider> &item: providers) {
            supplied_cells += static_cast<int64_t>(item.value.adopted_cells.size());
        }
        statistics["owners"] = owners.size();
        statistics["providers"] = providers.size();
        statistics["supplied_cells"] = supplied_cells;
        statistics["pending_provides"] = pending_provides.size() + planned_provides.size();
        statistics["registered"] = entry_locations.size();
        statistics["streamed"] = streamed;
        statistics["chunks"] = chunks.size();
        statistics["active_chunks"] = active_chunks;
        statistics["pending_builds"] = pending_build_count;
        statistics["pending_prefetches"] = pending_prefetches.size();
        statistics["pending_clears"] = pending_clears.size() + planned_clears.size();
        statistics["plan_msec"] = plan_msec;
        statistics["build_rate"] = build_rate;
        statistics["passes"] = passes;
        statistics["planning"] = planning;
        statistics["target_revision"] = target_revision;
        statistics["scanned_revision"] = scanned_revision;
        statistics["pending_nearby"] = _get_pending_nearby_locked(1);
        statistics["nearby_ready"] = _is_area_ready_locked(1);
        statistics["budget_msec"] = filling && pending_build_count + pending_clears.size() > CATCHUP_BACKLOG
                                            ? CATCHUP_BUDGET_MSEC
                                            : BUDGET_MSEC;
        statistics["filling"] = filling;
        // main thread time in the last full second, by owner and for the providers' cells [ms]
        Dictionary owner_msec;
        Dictionary owner_max_msec;
        for (int index = 0; index < owners.size(); index++) {
            owner_msec[owners[index].name] =
                    static_cast<double>(owner_times[index].last_usec) / LibMaszynaUnits::USEC_PER_MSEC;
            owner_max_msec[owners[index].name] =
                    static_cast<double>(owner_times[index].last_max_usec) / LibMaszynaUnits::USEC_PER_MSEC;
        }
        statistics["owner_msec"] = owner_msec;
        statistics["owner_max_msec"] = owner_max_msec;
        statistics["provide_msec"] = static_cast<double>(provide_time.last_usec) / LibMaszynaUnits::USEC_PER_MSEC;
        statistics["provide_max_msec"] =
                static_cast<double>(provide_time.last_max_usec) / LibMaszynaUnits::USEC_PER_MSEC;
        statistics["withdraw_msec"] = static_cast<double>(withdraw_time.last_usec) / LibMaszynaUnits::USEC_PER_MSEC;
        statistics["draw_distance"] = draw_distance;
        statistics["chunk_size"] = CHUNK_SIZE_M;
        statistics["camera_position"] = camera_position;
        statistics["camera_chunk"] = _get_chunk_key(camera_position);
        statistics["has_camera"] = camera_id.is_valid();
        return statistics;
    }

    void SceneryStreamingServer::_request_plan(const Vector3 &p_position) {
        MutexLock lock(mutex);
        // a jump - another vehicle, a teleport - leaves an area to fill, as a new camera does
        if (p_position.distance_to(camera_position) > CHUNK_SIZE_M) {
            filling = true;
        }
        camera_position = p_position;
        target_revision++;
        scanned_revision = 0;
        // pending_build_count stays until the pass replaces it: a camera moving through a backlog
        // would otherwise report no builds between every two passes (streaming_builds_finished)
        content_dirty = false;
        force_plan = false;
        _drop_stale_work();
        if (planning) {
            return;
        }
        planning = true;
        if (worker.is_null()) {
            worker.instantiate();
            worker->start(callable_mp(this, &SceneryStreamingServer::_worker_loop));
        }
        semaphore->post();
    }

    void SceneryStreamingServer::streaming_set_enabled(const bool p_enabled) {
        streaming_enabled = p_enabled;
    }

    bool SceneryStreamingServer::streaming_is_enabled() const {
        return streaming_enabled;
    }

    /// Looks at the camera, the anchor and the content every WATCH_INTERVAL_SEC and plans when
    /// they changed; the work a plan leaves is applied per frame (_process_streaming())
    void SceneryStreamingServer::_watch_camera() {
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        ERR_FAIL_NULL(tree);
        watch_timer = tree->create_timer(WATCH_INTERVAL_SEC, true, false, true);
        watch_timer->connect("timeout", callable_mp(this, &SceneryStreamingServer::_watch_camera));
        if (!streaming_enabled) {
            return;
        }
        ObjectID current_camera_id;
        bool requested;
        {
            MutexLock lock(mutex);
            current_camera_id = camera_id;
            // the anchor moving into another chunk, or going, is a change of what is kept
            const Vector2i anchor_key = anchored ? _get_chunk_key(anchor_position) : Vector2i();
            if (anchored != has_anchor_chunk || anchor_key != anchor_chunk) {
                has_anchor_chunk = anchored;
                anchor_chunk = anchor_key;
                content_dirty = true;
            }
            requested = force_plan || content_dirty;
        }
        const Camera3D *camera = Object::cast_to<Camera3D>(ObjectDB::get_instance(current_camera_id));
        if (camera == nullptr || !camera->is_inside_tree()) {
            return;
        }
        const Vector3 position = camera->get_global_position();
        const uint64_t now = Time::get_singleton()->get_ticks_msec();
        const float movement = static_cast<float>(position.distance_to(last_camera_position));
        requested = requested || movement >= CAMERA_STEP_M || (movement > 0.0 && now - last_plan_msec >= INTERVAL_MSEC);
        if (requested) {
            last_plan_msec = now;
            last_camera_position = position;
            _request_plan(position);
            const Vector2i chunk = _get_chunk_key(position);
            if (chunk != camera_chunk) {
                camera_chunk = chunk;
                emit_signal(streaming_camera_chunk_changed_signal, chunk);
            }
        }
        bool has_work;
        {
            MutexLock lock(mutex);
            has_work = _has_work_locked();
        }
        if (has_work) {
            _set_applying(true);
        }
    }

    /// The work of the plan, a frame budget at a time, until none is left
    void SceneryStreamingServer::_process_streaming() {
        if (!streaming_enabled) {
            _set_applying(false);
            return;
        }
        _apply_plan();
        bool is_building;
        bool has_work;
        {
            MutexLock lock(mutex);
            // after the frame's work, not before the next: the tick stops once the work is done
            if (filling && _is_area_ready_locked(1)) {
                filling = false;
            }
            is_building = planning || pending_build_count > 0;
            has_work = _has_work_locked();
        }
        _set_building(is_building);
        if (!has_work) {
            _set_applying(false);
        }
    }

    bool SceneryStreamingServer::_has_work_locked() const {
        return planning || freed_pending || planned_builds.size() > 0 || planned_clears.size() > 0 ||
               planned_provides.size() > 0 || planned_withdraws.size() > 0 || pending_builds.size() > 0 ||
               pending_prefetches.size() > 0 || pending_clears.size() > 0 || pending_provides.size() > 0 ||
               pending_withdraws.size() > 0;
    }

    void SceneryStreamingServer::_set_applying(const bool p_applying) {
        if (p_applying == applying) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        ERR_FAIL_NULL(tree);
        applying = p_applying;
        if (applying) {
            tree->connect("process_frame", callable_mp(this, &SceneryStreamingServer::_process_streaming));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &SceneryStreamingServer::_process_streaming));
    }

    void SceneryStreamingServer::_set_building(const bool p_building) {
        if (p_building == building) {
            return;
        }
        building = p_building;
        emit_signal(building ? streaming_builds_started_signal : streaming_builds_finished_signal);
    }

    bool SceneryStreamingServer::streaming_is_building() const {
        return building;
    }

    /// Applies incrementally published work within a frame budget. Every task is validated against
    /// the current camera revision immediately before it reaches its owner.
    void SceneryStreamingServer::_apply_plan() {
        bool catching_up;
        {
            MutexLock lock(mutex);
            if (planned_builds.size() > 0 || planned_clears.size() > 0) {
                for (const PendingBuild &build: planned_builds) {
                    (build.ahead ? pending_prefetches : pending_builds).push_back(build);
                }
                pending_clears.append_array(planned_clears);
                planned_builds.clear();
                planned_clears.clear();
                pending_builds.sort_custom<DistanceComparator>();
                pending_prefetches.sort_custom<DistanceComparator>();
            }
            if (planned_provides.size() > 0 || planned_withdraws.size() > 0) {
                pending_provides.append_array(planned_provides);
                pending_withdraws.append_array(planned_withdraws);
                planned_provides.clear();
                planned_withdraws.clear();
                pending_provides.sort_custom<DistanceComparator>();
            }
            if (freed_pending) {
                _drop_freed_work();
            }
            catching_up = filling && pending_build_count + pending_clears.size() > CATCHUP_BACKLOG;
        }

        Time *time = Time::get_singleton();
        const uint64_t now = time->get_ticks_usec();
        const uint64_t deadline = now + (static_cast<uint64_t>(LibMaszynaUnits::USEC_PER_MSEC) *
                                         (catching_up ? CATCHUP_BUDGET_MSEC : BUDGET_MSEC));
        if (now - build_rate_usec >= static_cast<uint64_t>(LibMaszynaUnits::USEC_PER_SECOND)) {
            build_rate = applied_builds;
            applied_builds = 0;
            build_rate_usec = now;
            for (WorkTime &owner_time: owner_times) {
                owner_time.roll();
            }
            provide_time.roll();
            withdraw_time.roll();
        }

        // Clearing first gives back what the builds below take.
        while (pending_clears.size() > 0) {
            const PendingClear pending = pending_clears[pending_clears.size() - 1];
            pending_clears.resize(pending_clears.size() - 1);
            bool apply = false;
            bool chunk_cleared = false;
            Vector2i key;
            {
                MutexLock lock(mutex);
                Entry *entry = _get_entry(pending.stream_rid);
                if (pending.revision == target_revision && entry != nullptr && entry->built &&
                    entry->wanted_revision != target_revision) {
                    entry->built = false;
                    key = entry_locations[pending.stream_rid].chunk;
                    Chunk &chunk = chunks[key];
                    chunk.built_count--;
                    chunk_cleared = chunk.built_count == 0;
                    apply = true;
                }
            }
            if (apply && pending.clear.is_valid()) {
                if (verbose) {
                    UtilityFunctions::print(
                            "[SceneryStreaming] clearing owner ", pending.owner, " piece ", pending.user_rid.get_id());
                }
                const uint64_t started = time->get_ticks_usec();
                pending.clear.call(pending.user_rid);
                owner_times.write[pending.owner].add(time->get_ticks_usec() - started);
                cleared_since_release++;
            }
            if (chunk_cleared) {
                emit_signal(chunk_cleared_signal, key);
            }
            if (time->get_ticks_usec() >= deadline) {
                return;
            }
        }
        // a provider's cell out of reach: what was made of its content goes, pieces and all
        while (pending_withdraws.size() > 0) {
            const PendingWithdraw pending = pending_withdraws[pending_withdraws.size() - 1];
            pending_withdraws.resize(pending_withdraws.size() - 1);
            std::array<Vector<RID>, CONTENT_KIND_COUNT> released;
            {
                MutexLock lock(mutex);
                Provider *provider = providers.getptr(pending.provider_rid);
                ProviderCell *cell = provider != nullptr ? provider->cells.getptr(pending.cell) : nullptr;
                if (pending.revision == target_revision && cell != nullptr && cell->adopted &&
                    cell->wanted_revision != target_revision) {
                    released.swap(cell->adopted_rids);
                    cell->adopted = false;
                    provider->adopted_cells.erase(pending.cell);
                }
            }
            const uint64_t started = time->get_ticks_usec();
            _release_content(released);
            withdraw_time.add(time->get_ticks_usec() - started);
            if (time->get_ticks_usec() >= deadline) {
                return;
            }
        }
        // what the cleared pieces held is free, but the allocator keeps it until asked - and asking
        // takes long on a heap of gigabytes, so the worker asks
        if (cleared_since_release >= RELEASE_CLEARED_PIECES) {
            cleared_since_release = 0;
            MutexLock lock(mutex);
            release_requested = true;
        }

        // a provider's cell in reach: its content goes to the consumers, which register its pieces
        while (pending_provides.size() > 0) {
            const PendingProvide pending = pending_provides[pending_provides.size() - 1];
            pending_provides.resize(pending_provides.size() - 1);
            std::array<Consumer, CONTENT_KIND_COUNT> kind_consumers;
            RID scenario;
            bool apply = false;
            {
                MutexLock lock(mutex);
                Provider *provider = providers.getptr(pending.provider_rid);
                ProviderCell *cell = provider != nullptr ? provider->cells.getptr(pending.cell) : nullptr;
                apply = pending.revision == target_revision && cell != nullptr && !cell->adopted &&
                        cell->wanted_revision == target_revision;
                if (cell != nullptr && cell->queued_revision == pending.revision) {
                    cell->queued_revision = 0;
                }
                if (apply) {
                    kind_consumers = consumers;
                    scenario = provider->scenario;
                }
            }
            if (!apply) {
                continue;
            }
            const uint64_t started = time->get_ticks_usec();
            std::array<Vector<RID>, CONTENT_KIND_COUNT> adopted;
            if (verbose) {
                UtilityFunctions::print("[SceneryStreaming] adopting cell ", pending.cell);
            }
            for (int kind = 0; kind < CONTENT_KIND_COUNT; kind++) {
                const Callable &adopt = kind_consumers[kind].adopt;
                ERR_CONTINUE_MSG(
                        !adopt.is_valid() && !pending.items[kind].is_empty(),
                        "No consumer of supplied content of kind " + itos(kind));
                for (int64_t index = 0; index < pending.items[kind].size(); index++) {
                    const RID rid = adopt.call(pending.items[kind][index], scenario);
                    if (rid.is_valid()) {
                        adopted[kind].push_back(rid);
                    }
                }
            }
            bool kept = false;
            {
                MutexLock lock(mutex);
                Provider *provider = providers.getptr(pending.provider_rid);
                ProviderCell *cell = provider != nullptr ? provider->cells.getptr(pending.cell) : nullptr;
                // the provider freed while its content was handed over takes nothing with it
                if (cell != nullptr && !cell->adopted) {
                    cell->adopted = true;
                    cell->adopted_rids = adopted;
                    provider->adopted_cells.insert(pending.cell);
                    if (!cell->ahead) {
                        pending_build_count--;
                    }
                    kept = true;
                }
            }
            if (!kept) {
                _release_content(adopted);
            }
            provide_time.add(time->get_ticks_usec() - started);
            if (time->get_ticks_usec() >= deadline) {
                return;
            }
        }

        if (!_apply_builds(pending_builds, deadline)) {
            return;
        }
        // nothing in range waits: what is ahead of the train is built with what is left, a little
        const uint64_t prefetch_deadline =
                MIN(deadline, time->get_ticks_usec() +
                                      (static_cast<uint64_t>(LibMaszynaUnits::USEC_PER_MSEC) * PREFETCH_BUDGET_MSEC));
        _apply_builds(pending_prefetches, prefetch_deadline);
    }

    bool SceneryStreamingServer::_apply_builds(Vector<PendingBuild> &p_builds, const uint64_t p_deadline) {
        Time *time = Time::get_singleton();
        while (p_builds.size() > 0) {
            const PendingBuild pending = p_builds[p_builds.size() - 1];
            p_builds.resize(p_builds.size() - 1);
            bool apply = false;
            {
                MutexLock lock(mutex);
                Entry *entry = _get_entry(pending.stream_rid);
                apply = pending.revision == target_revision && entry != nullptr && !entry->built &&
                        entry->wanted_revision == target_revision;
                if (entry != nullptr && entry->queued_revision == pending.revision) {
                    entry->queued_revision = 0;
                }
            }
            if (apply && pending.build.is_valid()) {
                if (verbose) {
                    UtilityFunctions::print(
                            "[SceneryStreaming] building owner ", pending.owner, " piece ", pending.user_rid.get_id());
                }
                const uint64_t started = time->get_ticks_usec();
                pending.build.call(pending.user_rid, pending.preloaded);
                owner_times.write[pending.owner].add(time->get_ticks_usec() - started);
                MutexLock lock(mutex);
                Entry *entry = _get_entry(pending.stream_rid);
                if (pending.revision == target_revision && entry != nullptr && !entry->built &&
                    entry->wanted_revision == target_revision) {
                    entry->built = true;
                    chunks[entry_locations[pending.stream_rid].chunk].built_count++;
                    if (!pending.ahead) {
                        pending_build_count--;
                    }
                }
                applied_builds++;
            }
            if (time->get_ticks_usec() >= p_deadline) {
                return false;
            }
        }
        return true;
    }

    /// Computes the complete desired set first, then preloads and publishes it nearest-first. A
    /// changed camera revision aborts the pass between individual preloads.
    bool SceneryStreamingServer::_plan(const uint64_t p_revision, const Vector3 &p_camera_position) {
        const uint64_t started_msec = Time::get_singleton()->get_ticks_msec();
        Vector<PendingBuild> entering;
        Vector<PendingClear> leaving;
        Vector<PendingProvide> supplying;
        Vector<PendingWithdraw> withdrawing;
        int wanted_unbuilt = 0;
        {
            MutexLock lock(mutex);
            if (p_revision != target_revision || !camera_id.is_valid()) {
                return false;
            }
            for (KeyValue<Vector2i, Chunk> &item: chunks) {
                Chunk &chunk = item.value;
                if (chunk.dirty) {
                    _sort_chunk(chunk);
                }
                const float distance = _get_chunk_distance(item.key, p_camera_position);
                const bool anchored = has_anchor_chunk && item.key == anchor_chunk;
                for (Entry &entry: chunk.entries) {
                    // in range, built ahead of it, kept a while after it - or in the anchor's chunk
                    const bool wanted = anchored || (entry.built ? distance <= entry.range_end + HYSTERESIS_M
                                                                 : distance <= entry.range_end + PREFETCH_M);
                    entry.ahead = distance > entry.range_end;
                    entry.wanted_revision = wanted ? p_revision : 0;
                    if (wanted && !entry.built && entry.queued_revision != p_revision) {
                        PendingBuild build;
                        build.owner = entry.owner;
                        build.ahead = entry.ahead;
                        build.preload = owners[entry.owner].preload;
                        build.build = owners[entry.owner].build;
                        build.stream_rid = entry.stream_rid;
                        build.user_rid = entry.user_rid;
                        build.chunk = item.key;
                        build.distance = distance;
                        build.revision = p_revision;
                        entering.push_back(build);
                        entry.queued_revision = p_revision;
                    } else if (!wanted && entry.built) {
                        PendingClear clear;
                        clear.owner = entry.owner;
                        clear.clear = owners[entry.owner].clear;
                        clear.stream_rid = entry.stream_rid;
                        clear.user_rid = entry.user_rid;
                        clear.revision = p_revision;
                        leaving.push_back(clear);
                    }
                    if (wanted && !entry.built && !entry.ahead) {
                        wanted_unbuilt++;
                    }
                }
            }
            // a provider's cell is wanted within the draw distance - the farthest any piece is
            // drawn (stream_register()) - of its content, which may reach out of it by its overhang
            const Vector2i camera_key = _get_chunk_key(p_camera_position);
            for (KeyValue<RID, Provider> &item: providers) {
                Provider &provider = item.value;
                if (!provider.scenario.is_valid()) {
                    continue;
                }
                // a cell in reach of the camera, or the anchor's wherever it is
                const auto plan_cell = [&](const Vector2i &p_key, const bool p_anchored) {
                    ProviderCell *cell = provider.cells.getptr(p_key);
                    if (cell == nullptr) {
                        return;
                    }
                    const float distance = _get_chunk_distance(p_key, p_camera_position);
                    const float reach = draw_distance + cell->overhang;
                    const bool wanted = p_anchored || (cell->adopted ? distance <= reach + HYSTERESIS_M
                                                                     : distance <= reach + PREFETCH_M);
                    cell->ahead = distance > reach;
                    cell->wanted_revision = wanted ? p_revision : 0;
                    if (wanted && !cell->adopted && cell->queued_revision != p_revision) {
                        PendingProvide provide;
                        provide.provider_rid = item.key;
                        provide.provider = provider.provider;
                        provide.kinds = provider.kinds;
                        provide.cell = p_key;
                        provide.distance = distance;
                        provide.revision = p_revision;
                        supplying.push_back(provide);
                        cell->queued_revision = p_revision;
                    }
                    if (wanted && !cell->adopted && !cell->ahead) {
                        wanted_unbuilt++;
                    }
                };
                const int radius = static_cast<int>(
                        Math::ceil((draw_distance + provider.max_overhang + HYSTERESIS_M) / CHUNK_SIZE_M));
                for (int x = camera_key.x - radius; x <= camera_key.x + radius; x++) {
                    for (int y = camera_key.y - radius; y <= camera_key.y + radius; y++) {
                        const Vector2i key(x, y);
                        plan_cell(key, has_anchor_chunk && key == anchor_chunk);
                    }
                }
                if (has_anchor_chunk && (Math::abs(anchor_chunk.x - camera_key.x) > radius ||
                                         Math::abs(anchor_chunk.y - camera_key.y) > radius)) {
                    plan_cell(anchor_chunk, true);
                }
                // a supplied cell the pass above did not want, however far the camera has gone
                for (const Vector2i &key: provider.adopted_cells) {
                    if (provider.cells[key].wanted_revision != p_revision) {
                        withdrawing.push_back(PendingWithdraw{item.key, key, p_revision});
                    }
                }
            }
            pending_build_count = wanted_unbuilt;
            scanned_revision = p_revision;
            planned_clears.append_array(leaving);
            planned_withdraws.append_array(withdrawing);
        }

        // the cells' content first, nearest first: a provider's terrain is what the area stands on
        supplying.sort_custom<DistanceComparator>();
        for (int64_t i = supplying.size() - 1; i >= 0; i--) {
            PendingProvide provide = supplying[i];
            {
                MutexLock lock(mutex);
                if (p_revision != target_revision || !providers.has(provide.provider_rid)) {
                    return false;
                }
            }
            for (const ContentKind kind: provide.kinds) {
                if (verbose) {
                    UtilityFunctions::print("[SceneryStreaming] loading cell ", provide.cell, " kind ", kind);
                }
                provide.items[kind] = provide.provider->chunk_load(provide.cell, kind);
            }
            MutexLock lock(mutex);
            if (p_revision != target_revision) {
                return false;
            }
            planned_provides.push_back(provide);
        }

        // nearest first, a batch at a time on the preloads' own threads (reading and decoding is
        // what makes a piece buildable), each batch published whole
        entering.sort_custom<DistanceComparator>();
        const int64_t batch_size = preload_queue->get_worker_count();
        int64_t next = entering.size() - 1;
        std::vector<PendingBuild> batch;
        std::vector<int> tasks;
        while (next >= 0) {
            batch.clear();
            {
                MutexLock lock(mutex);
                if (p_revision != target_revision) {
                    return false;
                }
                for (; next >= 0 && static_cast<int64_t>(batch.size()) < batch_size; next--) {
                    if (entry_locations.has(entering[next].stream_rid)) {
                        batch.push_back(entering[next]);
                    }
                }
            }
            tasks.assign(batch.size(), -1);
            for (size_t index = 0; index < batch.size(); index++) {
                if (batch[index].preload.is_valid()) {
                    if (verbose) {
                        UtilityFunctions::print(
                                "[SceneryStreaming] preloading owner ", batch[index].owner, " piece ",
                                batch[index].user_rid.get_id());
                    }
                    tasks[index] = preload_queue->submit(batch[index].preload.bind(batch[index].user_rid));
                }
            }
            for (size_t index = 0; index < batch.size(); index++) {
                if (tasks[index] >= 0) {
                    batch[index].preloaded = preload_queue->wait(tasks[index]);
                }
            }
            MutexLock lock(mutex);
            if (p_revision != target_revision) {
                return false;
            }
            for (const PendingBuild &build: batch) {
                const Entry *entry = _get_entry(build.stream_rid);
                if (entry != nullptr && entry->wanted_revision == p_revision) {
                    planned_builds.push_back(build);
                }
            }
        }

        MutexLock lock(mutex);
        if (p_revision != target_revision) {
            return false;
        }
        plan_msec = Time::get_singleton()->get_ticks_msec() - started_msec;
        passes++;
        return true;
    }

    void SceneryStreamingServer::_worker_loop() {
        while (true) {
            semaphore->wait();
            while (true) {
                uint64_t revision;
                Vector3 position;
                {
                    MutexLock lock(mutex);
                    if (exiting) {
                        return;
                    }
                    if (!camera_id.is_valid()) {
                        planning = false;
                        break;
                    }
                    revision = target_revision;
                    position = camera_position;
                }
                const bool completed = _plan(revision, position);
                bool release = false;
                {
                    MutexLock lock(mutex);
                    release = release_requested;
                    release_requested = false;
                }
                if (release) {
                    ProcessMemory::release_unused();
                }
                MutexLock lock(mutex);
                if (exiting) {
                    return;
                }
                if (completed && revision == target_revision) {
                    planning = false;
                    break;
                }
                if (!camera_id.is_valid()) {
                    planning = false;
                    break;
                }
            }
        }
    }
} // namespace godot
