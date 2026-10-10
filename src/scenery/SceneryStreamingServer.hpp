#pragma once

#include "scenery/SceneryStreamingProvider.hpp"
#include "utils/WorkerTaskQueue.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/classes/semaphore.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <array>

namespace godot {
    /// Spatial streaming of scenery content. A rendering server registers where each of its pieces
    /// is placed and how far it is meant to be visible; the piece is built once the camera comes
    /// within that range of the chunk holding it and cleared again when the camera leaves.
    ///
    /// A real scenery places hundreds of thousands of pieces, so building them all at load costs
    /// both the loading time and the frame rate - most of them are nowhere near the camera.
    ///
    /// Planning runs on a worker thread, together with whatever an owner wants prepared off the
    /// main thread (loading a model, building a mesh). Building and clearing run on the main
    /// thread within a per-frame time budget, because they touch RenderingServer and GDScript.
    ///
    /// Content may also be supplied by chunk (SceneryStreamingProvider): a provider's cell is asked
    /// for as the camera comes within the draw distance of it, and what it gives is handed to the
    /// consumer of its kind, which registers it as pieces like any other.
    class SceneryStreamingServer : public Object {
            GDCLASS(SceneryStreamingServer, Object)

        public:
            /// XZ size of a streaming chunk, same grid as the scenery triangle chunks
            /// (SceneryTrianglesSink::CHUNK_SIZE_M)
            static constexpr float CHUNK_SIZE_M = 1000.0;
            /// Pieces are cleared only beyond their range plus this margin, so a camera moving
            /// around a range boundary does not rebuild them over and over - rebuilding is far more
            /// expensive than keeping them a little longer. One chunk: a train shunting into the
            /// next chunk and back finds what it left built (250 m cleared short-range terrain
            /// ~550 m past its chunk, and shunting rebuilt it every time)
            static constexpr float HYSTERESIS_M = CHUNK_SIZE_M;
            /// Pieces are built ahead this far past their range - with what is left of a frame once
            /// nothing in range waits, at most PREFETCH_BUDGET_MSEC - so the world a train drives
            /// into is there already. Within HYSTERESIS_M, so a piece built ahead is not cleared at once
            static constexpr float PREFETCH_M = HYSTERESIS_M / 2;
            static constexpr uint64_t PREFETCH_BUDGET_MSEC = 1;
            /// A pass is planned at most this often...
            static constexpr uint64_t INTERVAL_MSEC = 250;
            /// ...or as soon as the camera has moved this far
            static constexpr float CAMERA_STEP_M = 50.0;
            /// The camera, the anchor and the content are looked at this often [s]; only the work
            /// a plan leaves - building, clearing - runs every frame, and only while there is some
            static constexpr double WATCH_INTERVAL_SEC = 0.1;
            /// Time spent building and clearing per frame once the streaming has caught up
            static constexpr uint64_t BUDGET_MSEC = 4;
            /// ...and while an area is filled - a new camera, or one that jumped: filling a scenery
            /// in means thousands of builds, and at the idle budget that takes minutes of pop-in
            /// (the backlog only drains at frame rate times budget). Never while driving through the
            /// world: there a frame on time beats the world appearing a little sooner.
            static constexpr uint64_t CATCHUP_BUDGET_MSEC = 16;
            /// Backlog above which the catch-up budget is used
            static constexpr int CATCHUP_BACKLOG = 64;
            /// Pieces cleared before the memory they held is given back to the system, once the
            /// clearing is done (ProcessMemory::release_unused())
            static constexpr int RELEASE_CLEARED_PIECES = 256;
            /// Fallback for maszyna/scenery/draw_distance, also the range of the pieces
            /// that declare none of their own
            static constexpr float DEFAULT_DRAW_DISTANCE_M = 3000.0;
            static constexpr const char *DRAW_DISTANCE_SETTING = "maszyna/scenery/draw_distance";

            /// Pieces in range started waiting to be built...
            static const char *streaming_builds_started_signal;
            /// ...and every one of them is built
            static const char *streaming_builds_finished_signal;
            static const char *streaming_camera_chunk_changed_signal;
            /// The camera the streaming follows was set or cleared (streaming_set_camera())
            static const char *streaming_camera_changed_signal;
            static const char *chunk_cleared_signal;

            static SceneryStreamingServer *get_instance() {
                return Object::cast_to<SceneryStreamingServer>(
                        Engine::get_singleton()->get_singleton("SceneryStreamingServer"));
            }

        private:
            struct Owner {
                    String name;      // what the debug window calls it
                    Callable preload; // (rid) -> Variant, on the worker thread; may be invalid
                    Callable build;   // (rid, preloaded) on the main thread
                    Callable clear;   // (rid) on the main thread
            };
            /// Main thread time spent in one kind of work - in the second being counted, and in the
            /// last full one for the debug window
            struct WorkTime {
                    uint64_t usec = 0;
                    uint64_t max_usec = 0;
                    uint64_t last_usec = 0;
                    uint64_t last_max_usec = 0;

                    void add(const uint64_t p_usec) {
                        usec += p_usec;
                        max_usec = MAX(max_usec, p_usec);
                    }
                    void roll() {
                        last_usec = usec;
                        last_max_usec = max_usec;
                        usec = 0;
                        max_usec = 0;
                    }
            };

            struct Entry {
                    int owner = 0;
                    RID stream_rid;
                    RID user_rid;
                    /// The range the piece declared, 0 for none - range_end follows the draw distance
                    float declared_range = 0.0;
                    float range_end = 0.0;
                    bool built = false;
                    /// Wanted only ahead of its range (PREFETCH_M): nothing waits for it
                    bool ahead = false;
                    uint64_t wanted_revision = 0; // 0 means out of range
                    uint64_t queued_revision = 0;
            };

            struct Chunk {
                    Vector<Entry> entries; // sorted by range_end, descending; freed ones until sorted
                    int built_count = 0;
                    bool dirty = false; // entries registered or unregistered since the last sort
            };

            struct PendingBuild {
                    int owner = 0;
                    bool ahead = false; // built ahead of its range, with the prefetch budget
                    Callable preload;
                    Callable build;
                    RID stream_rid;
                    RID user_rid;
                    Variant preloaded;
                    Vector2i chunk;
                    float distance = 0.0;
                    uint64_t revision = 0;
            };

            struct PendingClear {
                    int owner = 0;
                    Callable clear;
                    RID stream_rid;
                    RID user_rid;
                    uint64_t revision = 0;
            };
            using ContentKind = SceneryStreamingProvider::ContentKind;
            static constexpr int CONTENT_KIND_COUNT = SceneryStreamingProvider::CONTENT_KIND_MAX;
            /// What a server does with the content of its kind: adopt(item, scenario) -> RID, and
            /// release(RID) once the cell is let go
            struct Consumer {
                    Callable adopt;
                    Callable release;
            };
            struct ProviderCell {
                    float overhang = 0.0;
                    bool adopted = false;
                    bool ahead = false; // wanted only ahead of the draw distance (PREFETCH_M)
                    uint64_t wanted_revision = 0;
                    uint64_t queued_revision = 0;
                    /// What the consumers made of the cell's content, by kind
                    std::array<Vector<RID>, CONTENT_KIND_COUNT> adopted_rids;
            };
            struct Provider {
                    Ref<SceneryStreamingProvider> provider;
                    RID scenario;
                    Vector<ContentKind> kinds;
                    HashMap<Vector2i, ProviderCell> cells;
                    /// The cells whose content is out, wherever the camera is
                    HashSet<Vector2i> adopted_cells;
                    float max_overhang = 0.0;
            };
            struct PendingProvide {
                    RID provider_rid;
                    Ref<SceneryStreamingProvider> provider;
                    Vector<ContentKind> kinds;
                    Vector2i cell;
                    std::array<Array, CONTENT_KIND_COUNT> items;
                    float distance = 0.0;
                    uint64_t revision = 0;
            };
            struct PendingWithdraw {
                    RID provider_rid;
                    Vector2i cell;
                    uint64_t revision = 0;
            };

            /// Longest visible range first, so the pieces a chunk wants at a given distance are
            /// always a prefix of its entries
            struct RangeComparator {
                    bool operator()(const Entry &p_left, const Entry &p_right) const {
                        return p_left.range_end > p_right.range_end;
                    }
            };

            /// Farthest first: pending_builds is a priority queue kept as a sorted vector, so the
            /// nearest build is its last element and taking it costs nothing
            struct DistanceComparator {
                    bool operator()(const PendingBuild &p_left, const PendingBuild &p_right) const {
                        return p_left.distance > p_right.distance;
                    }
                    bool operator()(const PendingProvide &p_left, const PendingProvide &p_right) const {
                        return p_left.distance > p_right.distance;
                    }
            };

            Mutex mutex;
            Ref<Semaphore> semaphore;
            Ref<Thread> worker;
            bool exiting = false;
            bool planning = false;
            bool freed_pending = false; // a piece was freed, the queues may hold dead work
            bool content_dirty = false;
            bool streaming_enabled = true;
            bool force_plan = false;

            Vector<Owner> owners;
            HashMap<Vector2i, Chunk> chunks;
            /// Where a piece's entry is: its chunk, and its index in the chunk's entries - set where
            /// the entry is added and where the chunk is sorted, so an entry is found without a scan
            struct EntryLocation {
                    Vector2i chunk;
                    int index = 0;
            };
            HashMap<RID, EntryLocation> entry_locations;
            Vector3 camera_position;
            float draw_distance = DEFAULT_DRAW_DISTANCE_M;

            ObjectID camera_id;
            /// Where the anchor is - its chunk is kept built wherever the camera is - and whether
            /// there is one
            Vector3 anchor_position;
            bool anchored = false;
            /// Its chunk as the last frame saw it - main thread writes, the planner reads
            bool has_anchor_chunk = false;
            Vector2i anchor_chunk;
            Vector3 last_camera_position;
            Vector2i camera_chunk; // main thread only: the camera's chunk, as last announced
            uint64_t last_plan_msec = 0;
            uint64_t target_revision = 0;
            uint64_t scanned_revision = 0;
            int pending_build_count = 0;
            /// --verbose: every piece's preload, build and clear in the log, to find a crash by the last
            const bool verbose = OS::get_singleton()->is_stdout_verbose();
            bool building = false; // main thread only: pending_build_count > 0, as last announced
            /// Main thread only: the next look at the camera (_watch_camera()), while there is one
            Ref<SceneTreeTimer> watch_timer;
            /// Main thread only: _process_streaming() is connected to process_frame - while there is work
            bool applying = false;
            Vector<PendingBuild> planned_builds; // published by the worker
            Vector<PendingClear> planned_clears;
            /// Taken over by the main thread; builds are ordered farthest first and taken from the
            /// back, so the pieces around the camera are built first
            Vector<PendingBuild> pending_builds;
            /// Builds ahead of the range, nearest last - applied only when pending_builds is empty
            Vector<PendingBuild> pending_prefetches;
            Vector<PendingClear> pending_clears;
            HashMap<RID, Provider> providers;
            std::array<Consumer, CONTENT_KIND_COUNT> consumers;
            Vector<PendingProvide> planned_provides; // published by the worker
            Vector<PendingWithdraw> planned_withdraws;
            /// Taken over by the main thread, nearest provide last like pending_builds
            Vector<PendingProvide> pending_provides;
            Vector<PendingWithdraw> pending_withdraws;
            uint64_t plan_msec = 0;        // duration of the last planning pass
            int cleared_since_release = 0; // main thread only
            /// The cleared pieces' memory is to be given back - by the worker, malloc_trim takes long
            bool release_requested = false;
            /// The camera is new or has jumped, and the area around it is not filled yet: the
            /// catch-up budget is spent only then, never while driving through the world
            bool filling = false;
            Vector<WorkTime> owner_times; // main thread only, by owner
            /// The preloads' own threads: never the WorkerThreadPool, which the engine's loading
            /// waits for while a preload waits for the main thread (docs/findings-archive.md, 10-02)
            Ref<WorkerTaskQueue> preload_queue;
            WorkTime provide_time;  // main thread only
            WorkTime withdraw_time; // main thread only
            int applied_builds = 0; // builds applied in the second being counted
            int build_rate = 0;     // ...and in the last full second, for the debug window
            uint64_t build_rate_usec = 0;
            int passes = 0; // planning passes finished, so a caller can tell "not started yet"

            static Vector2i _get_chunk_key(const Vector3 &p_origin);
            static float _get_chunk_distance(const Vector2i &p_key, const Vector3 &p_position);
            void _sort_chunk(Chunk &p_chunk);
            Entry *_get_entry(const RID &p_stream_rid);
            bool _plan(uint64_t p_revision, const Vector3 &p_camera_position);
            void _drop_freed_work();
            void _drop_stale_work();
            bool _is_area_ready_locked(int p_chunk_radius) const;
            int _get_pending_nearby_locked(int p_chunk_radius) const;
            void _request_plan(const Vector3 &p_position);
            void _worker_loop();
            void _watch_camera();
            void _process_streaming();
            void _set_applying(bool p_applying);
            /// A plan is being made or its work waits to be applied. Must be called with the mutex held.
            bool _has_work_locked() const;
            void _apply_plan();
            void _set_building(bool p_building);
            /// Builds from the back of the queue until the deadline; false once it is reached
            bool _apply_builds(Vector<PendingBuild> &p_builds, uint64_t p_deadline);
            /// Hands each RID back to the consumer of its kind
            void _release_content(const std::array<Vector<RID>, CONTENT_KIND_COUNT> &p_rids);


        protected:
            static void _bind_methods();

        private:
            /// The draw distance follows its setting while the scenery streams
            void _on_project_settings_changed();
            float _get_range_end(float p_declared_range) const;

        public:
            SceneryStreamingServer();
            ~SceneryStreamingServer() override;

            /* Stops the planning thread and waits for the pass it is in, then leaves the server
             * able to start a fresh one. Called while the scene tree is still alive: the worker
             * calls the owner's preload Callable, which is GDScript, and that Callable is gone
             * once the scripts are - and it must not be creating rendering resources while the
             * scenery frees them (see FINDINGS.md, 2026-09-22). */
            void streaming_drain();
            int owner_create(
                    const String &p_name, const Callable &p_preload, const Callable &p_build, const Callable &p_clear);
            RID stream_register(int p_owner, const RID &p_user_rid, const Vector3 &p_position, float p_range_end);
            void stream_free(const RID &p_stream_rid);
            /* Every piece of the owner is built again from what it is built of now */
            void owner_rebuild(int p_owner);
            /* A provider supplies the content of its cells into the scenario as the camera comes
             * near them; freeing it lets go of everything it supplied */
            RID provider_register(const Ref<SceneryStreamingProvider> &p_provider, const RID &p_scenario);
            void provider_free(const RID &p_provider);
            /* What the provider supplied goes, and is supplied again into the new scenario as the
             * camera comes near; an empty RID supplies nothing (its node out of the world) */
            void provider_set_scenario(const RID &p_provider, const RID &p_scenario);
            /* The server taking a kind of supplied content: adopt(item, scenario) -> RID makes it
             * (registering its pieces), release(RID) frees it */
            void content_set_consumer(
                    SceneryStreamingProvider::ContentKind p_kind, const Callable &p_adopt, const Callable &p_release);

            /* Streaming builds and clears content on `process_frame`. Tearing a scenery down
             * frees the very RIDs it streams, and that teardown yields a frame for its budget -
             * so it has to be paused for the duration, the way the vehicles' step is. */
            void streaming_set_enabled(bool p_enabled);
            bool streaming_is_enabled() const;

            void streaming_set_camera(uint64_t p_camera_id);
            /* Where the anchor is: its chunk is kept built - never cleared, its pieces built with
             * the prefetch budget while out of the camera's range - wherever the camera goes and
             * however far it is. Said again as the anchor moves; none after streaming_clear_anchor() */
            void streaming_set_anchor_position(const Vector3 &p_position);
            void streaming_clear_anchor();
            float streaming_get_draw_distance() const;
            /// Where the streaming camera is, for anything else that has to know what is near
            Vector3 streaming_get_camera_position() const;
            bool streaming_has_camera() const;
            /// The chunk the streaming camera is in (streaming_camera_chunk_changed)
            Vector2i streaming_get_camera_chunk() const;
            /// What the owners registered in the chunk (stream_register()'s rid), built or not
            TypedArray<RID> chunk_get_rids(const Vector2i &p_chunk) const;
            /// Pieces in range are still waiting to be built (streaming_builds_started/finished)
            bool streaming_is_building() const;
            bool area_is_ready(int p_chunk_radius = 1) const;
            /// Pieces of the chunks within p_chunk_radius of the camera's still to be built
            int area_get_pending_count(int p_chunk_radius = 1) const;
            int streaming_get_streamed_count() const;
            Dictionary streaming_get_statistics() const;
    };
} // namespace godot
