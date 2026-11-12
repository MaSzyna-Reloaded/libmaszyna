#include "maszyna/tracks/MaszynaTrackRenderingBackend.hpp"

#include "game_data/GameDataServer.hpp"
#include "maszyna/MaszynaGameDirectory.hpp"
#include "maszyna/tracks/MaszynaTrackLook.hpp"
#include "plugins/ReloadedPluginServer.hpp"
#include "rendering/ReloadedMaterialServer.hpp"
#include "rendering/TrackRenderingServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hashfuncs.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /// A track is drawn in segments of this length at least and at most [m]
        constexpr double MIN_CURVED_MESH_SEGMENT_LENGTH = 2.0;
        constexpr double MAX_MESH_SEGMENT_LENGTH = 10.0;
        /// An arc's segment as a share of its radius, a curve's as a share of its chord
        constexpr double ARC_SEGMENT_PER_RADIUS = 0.02;
        constexpr double CURVE_SEGMENT_PER_CHORD = 0.1;
        /// A rail profile's line: x, y, normal x, normal y, u
        constexpr int PROFILE_POINT_VALUE_COUNT = 5;
        /// The rail profile of a track that names none: x, y, normal x, normal y, u
        constexpr double DEFAULT_RAIL_PROFILE[][PROFILE_POINT_VALUE_COUNT] = {
                {0.111, -0.180, 1.000, 0.000, 0.00},    {0.046, -0.150, 0.707, 0.707, 0.15},
                {0.044, -0.050, 0.707, -0.707, 0.25},   {0.073, -0.038, 0.707, -0.707, 0.35},
                {0.072, -0.010, 0.707, 0.707, 0.40},    {0.052, -0.000, 0.000, 1.000, 0.45},
                {0.020, -0.000, 0.000, 1.000, 0.55},    {0.000, -0.010, -0.707, 0.707, 0.60},
                {-0.001, -0.038, -0.707, -0.707, 0.65}, {0.028, -0.050, -0.707, -0.707, 0.75},
                {0.026, -0.150, -0.707, 0.707, 0.85},   {-0.039, -0.180, -1.000, 0.000, 1.00},
        };
        /// The rail profiles, in the game directory: models/tory/railprofile_<name>.txt; its lines after
        /// RAIL_PROFILE_BLADE_SECTION are the blade's
        constexpr const char *RAIL_PROFILE_DIRECTORY = "models/tory";
        constexpr const char *RAIL_PROFILE_PREFIX = "railprofile_";
        constexpr const char *RAIL_PROFILE_FILE = "railprofile_%s.txt";
        constexpr const char *RAIL_PROFILE_COMMENT = "//";
        constexpr const char *RAIL_PROFILE_BLADE_SECTION = "// switch blade";
        constexpr const char *RAIL_PROFILE_NUMBER = "-?\\d+(?:\\.\\d+)?";
        /// A .scn track's optional trackbed texture, a switch's (Track.cpp:2378)
        constexpr const char *TRACKBED_PARAMETER = "trackbed";
        /// track_get_look()'s keys, MaszynaTrackData's names
        constexpr const char *LOOK_MATERIAL1 = "material1";
        constexpr const char *LOOK_MATERIAL2 = "material2";
        constexpr const char *LOOK_MATERIAL_TRACKBED = "material_trackbed";
        constexpr const char *LOOK_TEX_LENGTH = "tex_length";
        constexpr const char *LOOK_TEX_HEIGHT = "tex_height";
        constexpr const char *LOOK_TEX_WIDTH = "tex_width";
        constexpr const char *LOOK_TEX_SLOPE = "tex_slope";
        constexpr const char *LOOK_RAILPROFILE = "railprofile";
        /// The streaming's names of the cells' owners, by CellKind
        constexpr const char *CELL_STREAM_OWNER_NAMES[] = {"roads", "track cells"};
        /// _cell_preload()'s result: the cell's surfaces, an Array of them per CellLayer, and its
        /// switches' blades...
        enum PreloadedCell {
            PRELOADED_SURFACES,
            PRELOADED_BLADES,
            PRELOADED_CELL_FIELD_COUNT,
        };
        /// ...a surface: the name of its material and its arrays...
        enum PreloadedSurface {
            SURFACE_MATERIAL,
            SURFACE_ARRAYS,
            SURFACE_FIELD_COUNT,
        };
        /// ...and the blades of a switch: its track, where their meshes stand (the start of its curve),
        /// their materials, their arrays and how each turns
        enum PreloadedBlades {
            BLADES_TRACK_RENDER,
            BLADES_ORIGIN,
            BLADES_SWITCH_IS_RIGHT,
            BLADES_PRIMARY_MATERIAL,
            BLADES_SECONDARY_MATERIAL,
            BLADES_PRIMARY_ARRAYS,
            BLADES_SECONDARY_ARRAYS,
            BLADES_PRIMARY_PIVOT,
            BLADES_PRIMARY_ANGLE_FACTOR,
            BLADES_SECONDARY_PIVOT,
            BLADES_SECONDARY_ANGLE_FACTOR,
            BLADES_FIELD_COUNT,
        };

    } // namespace

    void MaszynaTrackRenderingBackend::_bind_methods() {
        BIND_ENUM_CONSTANT(SLOT_PRIMARY_RAIL);
        BIND_ENUM_CONSTANT(SLOT_SECONDARY_RAIL);
        BIND_ENUM_CONSTANT(SLOT_PRIMARY_BLADE);
        BIND_ENUM_CONSTANT(SLOT_SECONDARY_BLADE);
        BIND_ENUM_CONSTANT(SLOT_TRACKBED);
        BIND_ENUM_CONSTANT(SLOT_MAX);
        BIND_ENUM_CONSTANT(CELL_LAYER_RAILS);
        BIND_ENUM_CONSTANT(CELL_LAYER_BED);
        BIND_ENUM_CONSTANT(CELL_LAYER_MAX);

        ClassDB::bind_method(
                D_METHOD("track_get_meshes", "track_render"), &MaszynaTrackRenderingBackend::track_get_meshes);
        ClassDB::bind_method(
                D_METHOD("track_get_instances", "track_render"), &MaszynaTrackRenderingBackend::track_get_instances);
        ClassDB::bind_method(D_METHOD("track_get_look", "track_render"), &MaszynaTrackRenderingBackend::track_get_look);
        ClassDB::bind_method(
                D_METHOD("track_get_road_cell", "track_render"), &MaszynaTrackRenderingBackend::track_get_road_cell);
        ClassDB::bind_method(D_METHOD("road_cell_exists", "cell"), &MaszynaTrackRenderingBackend::road_cell_exists);
        ClassDB::bind_method(
                D_METHOD("road_cell_get_instance", "cell"), &MaszynaTrackRenderingBackend::road_cell_get_instance);
        ClassDB::bind_method(D_METHOD("road_cell_get_mesh", "cell"), &MaszynaTrackRenderingBackend::road_cell_get_mesh);
        ClassDB::bind_method(
                D_METHOD("track_get_track_cell", "track_render"), &MaszynaTrackRenderingBackend::track_get_track_cell);
        ClassDB::bind_method(D_METHOD("track_cell_exists", "cell"), &MaszynaTrackRenderingBackend::track_cell_exists);
        ClassDB::bind_method(
                D_METHOD("track_cell_get_instance", "cell", "layer"),
                &MaszynaTrackRenderingBackend::track_cell_get_instance);
        ClassDB::bind_method(
                D_METHOD("track_cell_get_mesh", "cell", "layer"), &MaszynaTrackRenderingBackend::track_cell_get_mesh);
        ClassDB::bind_static_method(
                get_class_static(), D_METHOD("mesh_curve_bake_interval", "curve"),
                &MaszynaTrackRenderingBackend::mesh_curve_bake_interval);
        ClassDB::bind_static_method(
                get_class_static(),
                D_METHOD("switch_blade_layout", "is_right_switch", "f_offset1", "f_offset2", "f_max_offset"),
                &MaszynaTrackRenderingBackend::switch_blade_layout);
        ClassDB::bind_static_method(
                get_class_static(), D_METHOD("unique_endpoint_connection", "track", "endpoint_index"),
                &MaszynaTrackRenderingBackend::unique_endpoint_connection);
    }

    MaszynaTrackRenderingBackend::MaszynaTrackRenderingBackend() {
        track_material_options[ReloadedMaterialServer::OPTION_CULL_DISABLED] = true;
        rail_profile_number = RegEx::create_from_string(RAIL_PROFILE_NUMBER);
        for (const auto &values: DEFAULT_RAIL_PROFILE) {
            ProfilePoint point;
            point.x = values[0];
            point.y = values[1];
            point.normal_x = values[2];
            point.normal_y = values[3];
            point.u = values[4];
            default_rail_profile.rail.push_back(point);
        }
        default_rail_profile.blade = default_rail_profile.rail;
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            // the data read again builds every track again (TrackRenderingServer), and with it the
            // pieces of the cells
            game_data->connect(
                    GameDataServer::data_unload_requested_signal,
                    callable_mp(this, &MaszynaTrackRenderingBackend::_on_data_unload_requested));
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            // the owners live as long as the streaming does
            for (int kind = 0; kind < CELL_KIND_MAX; kind++) {
                stream_owners[kind] = streaming->owner_create(
                        CELL_STREAM_OWNER_NAMES[kind], callable_mp(this, &MaszynaTrackRenderingBackend::_cell_preload),
                        callable_mp(this, &MaszynaTrackRenderingBackend::_cell_build),
                        callable_mp(this, &MaszynaTrackRenderingBackend::_cell_clear));
            }
        }
        looks = ResourceLoader::get_singleton()->load(LOOKS_PATH);
        ERR_FAIL_COND_MSG(looks.is_null(), "MaszynaTrackRenderingBackend: no track looks at " + String(LOOKS_PATH));
    }

    MaszynaTrackRenderingBackend::~MaszynaTrackRenderingBackend() {
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->disconnect(
                    GameDataServer::data_unload_requested_signal,
                    callable_mp(this, &MaszynaTrackRenderingBackend::_on_data_unload_requested));
        }
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (rendering == nullptr) {
            return;
        }
        for (const KeyValue<RID, Track> &item: tracks) {
            for (const RID &instance: item.value.instances) {
                if (instance.is_valid()) {
                    rendering->free_rid(instance);
                }
            }
        }
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        for (const KeyValue<RID, Cell> &item: cells) {
            if (streaming != nullptr) {
                streaming->stream_free(item.value.stream_rid);
            }
            for (const RID &instance: item.value.instances) {
                if (instance.is_valid()) {
                    rendering->free_rid(instance);
                }
            }
        }
    }

    bool MaszynaTrackRenderingBackend::TrackGeometry::is_valid() const {
        return track.is_valid() && curve1.is_valid();
    }

    bool MaszynaTrackRenderingBackend::CellKey::operator==(const CellKey &p_other) const {
        return scenario == p_other.scenario && coords == p_other.coords && kind == p_other.kind;
    }

    uint32_t MaszynaTrackRenderingBackend::CellKeyHasher::hash(const CellKey &p_key) {
        return hash_murmur3_one_32(
                static_cast<uint32_t>(p_key.kind),
                hash_murmur3_one_64(p_key.scenario.get_id(), HashMapHasherDefault::hash(p_key.coords)));
    }

    void MaszynaTrackRenderingBackend::track_create(const RID &p_track_render, const RID &p_track) {
        Track track;
        track.track = p_track;
        tracks.insert(p_track_render, track);
    }

    /// The track's meshes and instances go, and a streamed track leaves its cell
    void MaszynaTrackRenderingBackend::track_free(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        if (track->cell.is_valid()) {
            _cell_remove(p_track_render, *track);
        }
        _reset_track_instances(*track);
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (const RID &instance: track->instances) {
            if (instance.is_valid()) {
                rendering->free_rid(instance);
            }
        }
        tracks.erase(p_track_render);
    }

    /// What the track is drawn with: a .scn's track (MaszynaTrackData) with its own textures and
    /// section, any other with those its features map to (looks). A track of a cell is drawn so at once
    void MaszynaTrackRenderingBackend::track_set_description(
            const RID &p_track_render, const Ref<TrackData> &p_description) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        ERR_FAIL_COND(p_description.is_null());
        if (const MaszynaTrackData *scn_track = Object::cast_to<MaszynaTrackData>(p_description.ptr());
            scn_track != nullptr) {
            track->tex_length = scn_track->get_tex_length();
            track->tex_height = scn_track->get_tex_height();
            track->tex_width = scn_track->get_tex_width();
            track->tex_slope = scn_track->get_tex_slope();
            track->material1_name = scn_track->get_material1();
            track->material2_name = scn_track->get_material2();
            // optional switch attribute, Track.cpp:2378
            track->material_trackbed_name = scn_track->get_parameters().get(TRACKBED_PARAMETER, "");
            track->railprofile = scn_track->get_railprofile();
        } else {
            ERR_FAIL_COND(looks.is_null());
            const bool road = p_description->get_kind() == TrackServer::TRACK_ROAD;
            const Ref<MaszynaTrackLook> look =
                    road ? looks->get_road_looks().get(
                                   p_description->get_surface(),
                                   looks->get_road_looks().get(looks->get_default_surface(), Variant()))
                         : looks->get_rail_looks().get(
                                   p_description->get_bed(),
                                   looks->get_rail_looks().get(looks->get_default_bed(), Variant()));
            ERR_FAIL_COND_MSG(look.is_null(), "MaszynaTrackRenderingBackend: no look for the track's features");
            track->material1_name = p_description->get_centre_line() && !look->get_material1_centre_line().is_empty()
                                            ? look->get_material1_centre_line()
                                            : look->get_material1();
            track->material2_name = !road || p_description->get_shoulder() ? look->get_material2() : String();
            track->tex_length = look->get_tex_length() > 0.0 ? look->get_tex_length() : p_description->get_width();
            track->tex_height = look->get_tex_height();
            track->tex_width = look->get_tex_width();
            track->tex_slope = look->get_tex_slope();
            track->material_trackbed_name = String();
            track->railprofile = look->get_railprofile();
        }
        if (track->cell.is_valid()) {
            _cell_update_track(p_track_render, *track);
        }
    }

    /// A track of a cell is drawn by it as long as it is visible; one drawn by itself shows its instances
    void MaszynaTrackRenderingBackend::track_set_visible(const RID &p_track_render, const bool p_visible) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->visible = p_visible;
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (const RID &instance: track->instances) {
            if (instance.is_valid()) {
                rendering->instance_set_visible(instance, p_visible);
            }
        }
        if (track->cell.is_valid()) {
            _cell_update_track(p_track_render, *track);
        }
    }

    void MaszynaTrackRenderingBackend::track_set_scenario(const RID &p_track_render, const RID &p_scenario) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->scenario = p_scenario;
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (const RID &instance: track->instances) {
            if (instance.is_valid()) {
                rendering->instance_set_scenario(instance, p_scenario);
            }
        }
        if (track->cell.is_valid()) {
            // into the cell of its new scenario
            _cell_remove(p_track_render, *track);
            _cell_add(p_track_render, *track);
        }
    }

    void MaszynaTrackRenderingBackend::track_set_switch_blade_offsets(
            const RID &p_track_render, const double p_offset1, const double p_offset2) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        const SwitchBladeLayout layout = _switch_blade_layout(
                track->switch_is_right, p_offset1, p_offset2, track_server->get_switch_max_offset());
        track->primary_blade.offset = layout.primary_blade_offset;
        track->secondary_blade.offset = layout.secondary_blade_offset;
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (track->instances[SLOT_PRIMARY_BLADE].is_valid()) {
            rendering->instance_set_transform(
                    track->instances[SLOT_PRIMARY_BLADE],
                    MaszynaTrackMeshBuilder::build_blade_transform(
                            track->origin, track->primary_blade.pose, layout.primary_blade_offset));
        }
        if (track->instances[SLOT_SECONDARY_BLADE].is_valid()) {
            rendering->instance_set_transform(
                    track->instances[SLOT_SECONDARY_BLADE],
                    MaszynaTrackMeshBuilder::build_blade_transform(
                            track->origin, track->secondary_blade.pose, layout.secondary_blade_offset));
        }
    }

    bool MaszynaTrackRenderingBackend::track_stream(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, false);
        _cell_add(p_track_render, *track);
        return track->cell.is_valid();
    }

    /// A track of a cell is drawn by the cell as it is now; one drawn by itself is built at once
    void MaszynaTrackRenderingBackend::track_build(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        if (track->cell.is_valid()) {
            _cell_update_track(p_track_render, *track);
            return;
        }
        TrackSnapshot snapshot;
        if (!_read_track_piece(p_track_render, *track, snapshot)) {
            return;
        }
        _clear_track_meshes(*track);
        track->origin = snapshot.piece.curve1.p1;
        track->switch_is_right = snapshot.piece.switch_is_right;
        const PieceGeometry geometry = MaszynaTrackMeshBuilder::build_piece_geometry(snapshot.piece);
        const Geometry *slot_geometries[SLOT_MAX] = {
                &geometry.primary_rails, &geometry.secondary_rails, &geometry.primary_blade, &geometry.secondary_blade,
                &geometry.bed};
        const String slot_materials[SLOT_MAX] = {
                snapshot.primary_material, snapshot.secondary_material, snapshot.primary_material,
                snapshot.secondary_material, snapshot.bed_material};
        for (int slot = 0; slot < SLOT_MAX; slot++) {
            if (slot_geometries[slot]->vertices.is_empty()) {
                continue;
            }
            Ref<ArrayMesh> mesh;
            mesh.instantiate();
            _mesh_add_surface(
                    mesh, MaszynaTrackMeshBuilder::build_surface_arrays(*slot_geometries[slot], track->origin),
                    slot_materials[slot]);
            track->meshes[slot] = mesh;
            _slot_draw(*track, static_cast<Slot>(slot));
        }
        track->primary_blade.pose = geometry.primary_blade_pose;
        track->secondary_blade.pose = geometry.secondary_blade_pose;
        const TrackGeometry switch_geometry = _read_track_geometry(track->track);
        track_set_switch_blade_offsets(
                p_track_render, switch_geometry.switch_f_offset1, switch_geometry.switch_f_offset2);
    }

    void MaszynaTrackRenderingBackend::track_clear(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        _clear_track_meshes(*track);
    }

    /* A track drawn by itself whose trackbed is borrowed from its neighbours
     * (_copy_adjacent_trackbed_material(), Track.cpp:3326) - a switch, a track declaring "none" - is
     * built again with the neighbours it has now: one built before the topology (the editor, until
     * its Rebuild topology) had the default bed's (_resolve_trackbed()). The pieces of the cells are
     * read again with their neighbours - a scenery's tracks go into their cells as it loads, before
     * the topology is rebuilt - and the cells are streamed again */
    void MaszynaTrackRenderingBackend::topology_rebuilt() {
        TrackServer *track_server = TrackServer::get_instance();
        TrackRenderingServer *track_rendering = TrackRenderingServer::get_instance();
        ERR_FAIL_NULL(track_server);
        ERR_FAIL_NULL(track_rendering);
        Vector<RID> borrowing;
        for (const KeyValue<RID, Track> &item: tracks) {
            const Track &track = item.value;
            if (track.meshes[SLOT_PRIMARY_RAIL].is_valid() && !track.cell.is_valid() &&
                track.material_trackbed_name.is_empty() &&
                (track_server->track_is_switch(track.track) || track.material2_name.is_empty())) {
                borrowing.push_back(item.key);
            }
        }
        for (const RID &track_render: borrowing) {
            track_rendering->track_rebuild(track_render);
        }
        for (KeyValue<RID, Cell> &item: cells) {
            Vector<TrackSnapshot> snapshots;
            for (const RID &track_render: item.value.tracks) {
                const Track *track = tracks.getptr(track_render);
                TrackSnapshot snapshot;
                if (track != nullptr && track->visible && _read_track_piece(track_render, *track, snapshot)) {
                    snapshots.push_back(snapshot);
                }
            }
            {
                MutexLock lock(cells_mutex);
                item.value.snapshots = snapshots;
            }
            _cell_stream(item.key, item.value);
        }
    }

    Dictionary MaszynaTrackRenderingBackend::track_get_meshes(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, Dictionary());
        Dictionary meshes;
        for (int slot = 0; slot < SLOT_MAX; slot++) {
            if (track->meshes[slot].is_valid()) {
                meshes[slot] = track->meshes[slot];
            }
        }
        return meshes;
    }

    Dictionary MaszynaTrackRenderingBackend::track_get_instances(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, Dictionary());
        Dictionary instances;
        for (int slot = 0; slot < SLOT_MAX; slot++) {
            if (track->instances[slot].is_valid()) {
                instances[slot] = track->instances[slot];
            }
        }
        return instances;
    }

    Dictionary MaszynaTrackRenderingBackend::track_get_look(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, Dictionary());
        Dictionary look;
        look[LOOK_MATERIAL1] = track->material1_name;
        look[LOOK_MATERIAL2] = track->material2_name;
        look[LOOK_MATERIAL_TRACKBED] = track->material_trackbed_name;
        look[LOOK_TEX_LENGTH] = track->tex_length;
        look[LOOK_TEX_HEIGHT] = track->tex_height;
        look[LOOK_TEX_WIDTH] = track->tex_width;
        look[LOOK_TEX_SLOPE] = track->tex_slope;
        look[LOOK_RAILPROFILE] = track->railprofile;
        return look;
    }

    RID MaszynaTrackRenderingBackend::track_get_road_cell(const RID &p_track_render) const {
        return _track_get_cell(p_track_render, CELL_ROADS);
    }

    bool MaszynaTrackRenderingBackend::road_cell_exists(const RID &p_cell) const {
        const Cell *cell = cells.getptr(p_cell);
        return cell != nullptr && cell->kind == CELL_ROADS;
    }

    RID MaszynaTrackRenderingBackend::road_cell_get_instance(const RID &p_cell) const {
        return _cell_get_instance(p_cell, CELL_ROADS, CELL_LAYER_BED);
    }

    Ref<ArrayMesh> MaszynaTrackRenderingBackend::road_cell_get_mesh(const RID &p_cell) const {
        return _cell_get_mesh(p_cell, CELL_ROADS, CELL_LAYER_BED);
    }

    RID MaszynaTrackRenderingBackend::track_get_track_cell(const RID &p_track_render) const {
        return _track_get_cell(p_track_render, CELL_TRACKS);
    }

    bool MaszynaTrackRenderingBackend::track_cell_exists(const RID &p_cell) const {
        const Cell *cell = cells.getptr(p_cell);
        return cell != nullptr && cell->kind == CELL_TRACKS;
    }

    RID MaszynaTrackRenderingBackend::track_cell_get_instance(const RID &p_cell, const CellLayer p_layer) const {
        return _cell_get_instance(p_cell, CELL_TRACKS, p_layer);
    }

    Ref<ArrayMesh> MaszynaTrackRenderingBackend::track_cell_get_mesh(const RID &p_cell, const CellLayer p_layer) const {
        return _cell_get_mesh(p_cell, CELL_TRACKS, p_layer);
    }

    RID MaszynaTrackRenderingBackend::_track_get_cell(const RID &p_track_render, const CellKind p_kind) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, RID());
        const Cell *cell = cells.getptr(track->cell);
        return cell != nullptr && cell->kind == p_kind ? track->cell : RID();
    }

    RID MaszynaTrackRenderingBackend::_cell_get_instance(
            const RID &p_cell, const CellKind p_kind, const CellLayer p_layer) const {
        const Cell *cell = cells.getptr(p_cell);
        ERR_FAIL_COND_V(cell == nullptr || cell->kind != p_kind, RID());
        ERR_FAIL_INDEX_V(p_layer, CELL_LAYER_MAX, RID());
        return cell->instances[p_layer];
    }

    Ref<ArrayMesh> MaszynaTrackRenderingBackend::_cell_get_mesh(
            const RID &p_cell, const CellKind p_kind, const CellLayer p_layer) const {
        const Cell *cell = cells.getptr(p_cell);
        ERR_FAIL_COND_V(cell == nullptr || cell->kind != p_kind, Ref<ArrayMesh>());
        ERR_FAIL_INDEX_V(p_layer, CELL_LAYER_MAX, Ref<ArrayMesh>());
        return cell->meshes[p_layer];
    }

    double MaszynaTrackRenderingBackend::mesh_curve_bake_interval(const Ref<TrackCurve> &p_curve) {
        ERR_FAIL_COND_V(p_curve.is_null(), MAX_MESH_SEGMENT_LENGTH);
        if (!Math::is_zero_approx(p_curve->get_radius())) {
            return CLAMP(
                    Math::abs(p_curve->get_radius()) * ARC_SEGMENT_PER_RADIUS, MIN_CURVED_MESH_SEGMENT_LENGTH,
                    MAX_MESH_SEGMENT_LENGTH);
        }
        if (p_curve->get_c1().is_zero_approx() && p_curve->get_c2().is_zero_approx()) {
            return MAX_MESH_SEGMENT_LENGTH;
        }
        return CLAMP(
                p_curve->get_p1().distance_to(p_curve->get_p2()) * CURVE_SEGMENT_PER_CHORD,
                MIN_CURVED_MESH_SEGMENT_LENGTH, MAX_MESH_SEGMENT_LENGTH);
    }

    Dictionary MaszynaTrackRenderingBackend::switch_blade_layout(
            const bool p_is_right_switch, const double p_f_offset1, const double p_f_offset2,
            const double p_f_max_offset) {
        const SwitchBladeLayout layout =
                _switch_blade_layout(p_is_right_switch, p_f_offset1, p_f_offset2, p_f_max_offset);
        Dictionary result;
        result["primary_blade_mirrored"] = layout.primary_blade_mirrored;
        result["primary_other_mirrored"] = layout.primary_other_mirrored;
        result["secondary_blade_mirrored"] = layout.secondary_blade_mirrored;
        result["secondary_other_mirrored"] = layout.secondary_other_mirrored;
        result["primary_blade_offset"] = layout.primary_blade_offset;
        result["secondary_blade_offset"] = layout.secondary_blade_offset;
        return result;
    }

    Ref<TrackEndpointRef>
    MaszynaTrackRenderingBackend::unique_endpoint_connection(const RID &p_track, const int p_endpoint_index) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL_V(track_server, Ref<TrackEndpointRef>());
        const TypedArray<TrackEndpointRef> connections = track_server->track_get_endpoint_connections(
                p_track, static_cast<TrackServer::EndpointIndex>(p_endpoint_index));
        if (connections.size() != 1) {
            return {};
        }
        return connections[0];
    }

    MaszynaTrackRenderingBackend::TrackGeometry MaszynaTrackRenderingBackend::_read_track_geometry(const RID &p_track) {
        TrackGeometry geometry;
        TrackServer *track_server = TrackServer::get_instance();
        if (track_server == nullptr || !track_server->track_exists(p_track)) {
            return geometry;
        }
        const Ref<TrackCurve> curve1 = track_server->track_get_curve(p_track, TrackServer::TRACK_COMMON);
        if (curve1.is_null()) {
            return geometry;
        }
        geometry.track = p_track;
        geometry.curve1 = curve1;
        if (track_server->track_get_endpoints(p_track).size() > TrackServer::CURVE2_P1) {
            geometry.curve2 = track_server->track_get_curve(p_track, TrackServer::TRACK_DIVERGING);
        }
        geometry.width = track_server->track_get_width(p_track);
        geometry.switch_common_endpoint_index = track_server->track_get_common_endpoint_index(p_track);
        const RID switch_rid = track_server->track_get_switch(p_track);
        if (switch_rid.is_valid()) {
            geometry.switch_f_offset1 = track_server->switch_get_f_offset1(switch_rid);
            geometry.switch_f_offset2 = track_server->switch_get_f_offset2(switch_rid);
        }
        return geometry;
    }

    MaszynaTrackRenderingBackend::CurveShape
    MaszynaTrackRenderingBackend::_read_curve_shape(const Ref<TrackCurve> &p_curve) {
        CurveShape shape;
        shape.p1 = p_curve->get_p1();
        shape.c1 = p_curve->get_c1();
        shape.c2 = p_curve->get_c2();
        shape.p2 = p_curve->get_p2();
        shape.roll1 = p_curve->get_roll1();
        shape.roll2 = p_curve->get_roll2();
        shape.bake_interval = mesh_curve_bake_interval(p_curve);
        return shape;
    }

    bool MaszynaTrackRenderingBackend::_is_road(const RID &p_track, const TrackGeometry &p_geometry) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL_V(track_server, false);
        return track_server->track_get_group(p_track) == TrackServer::GROUP_ROAD && p_geometry.curve2.is_null();
    }

    bool MaszynaTrackRenderingBackend::_is_switch_track(const TrackGeometry &p_geometry) {
        return p_geometry.is_valid() && p_geometry.curve2.is_valid() &&
               p_geometry.switch_common_endpoint_index != TrackServer::POINT_NONE;
    }

    double MaszynaTrackRenderingBackend::_get_track_roll(const TrackGeometry &p_geometry, const int p_endpoint_index) {
        if (!p_geometry.is_valid()) {
            return 0.0;
        }
        const Ref<TrackCurve> &curve1 = p_geometry.curve1;
        const Ref<TrackCurve> &curve2 = p_geometry.curve2;
        switch (p_endpoint_index) {
            case TrackServer::CURVE1_P1:
                return curve1->get_roll1();
            case TrackServer::CURVE1_P2:
                return curve1->get_roll2();
            case TrackServer::CURVE2_P1:
                return curve2.is_valid() ? curve2->get_roll1() : 0.0;
            case TrackServer::CURVE2_P2:
                return curve2.is_valid() ? curve2->get_roll2() : 0.0;
            default:
                return 0.0;
        }
    }

    MaszynaTrackRenderingBackend::SwitchBladeLayout MaszynaTrackRenderingBackend::_switch_blade_layout(
            const bool p_is_right_switch, const double p_f_offset1, const double p_f_offset2,
            const double p_f_max_offset) {
        SwitchBladeLayout layout;
        if (p_is_right_switch) {
            layout.primary_blade_mirrored = true;
            layout.primary_other_mirrored = false;
            layout.secondary_blade_mirrored = false;
            layout.secondary_other_mirrored = true;
            layout.primary_blade_offset = p_f_offset2;
            layout.secondary_blade_offset = -p_f_max_offset + p_f_offset1;
            return layout;
        }
        layout.primary_blade_mirrored = false;
        layout.primary_other_mirrored = true;
        layout.secondary_blade_mirrored = true;
        layout.secondary_other_mirrored = false;
        layout.primary_blade_offset = -p_f_offset2;
        layout.secondary_blade_offset = p_f_max_offset - p_f_offset1;
        return layout;
    }

    void MaszynaTrackRenderingBackend::_mesh_add_surface(
            const Ref<ArrayMesh> &p_mesh, const Array &p_arrays, const String &p_material_name) const {
        p_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, p_arrays);
        // regen_normal_maps() computes tangents for normal-mapped rail/ballast textures (the arrays
        // never set ARRAY_TANGENT), but costs ~7-9ms PER CALL almost independent of mesh size -
        // for a real scenery with hundreds of track segments (2-3 calls each: rail, trackbed, and
        // switch secondary rail), that alone was ~2.1-2.3s of a ~3s scenery load, confirmed by
        // profiling. Skipped here; the tradeoff is flatter-looking (untangented) normal-map shading
        // on rails/ballast specifically, not missing geometry or broken base-color/roughness texturing.
        if (p_material_name.is_empty()) {
            return;
        }
        ReloadedMaterialServer *materials = ReloadedMaterialServer::get_instance();
        ERR_FAIL_NULL(materials);
        p_mesh->surface_set_material(
                p_mesh->get_surface_count() - 1, materials->material_get(p_material_name, track_material_options));
    }

    bool MaszynaTrackRenderingBackend::_has_trackbed_profile_options(const Track *p_track) {
        return p_track != nullptr &&
               (!p_track->material2_name.is_empty() || !p_track->material_trackbed_name.is_empty() ||
                !Math::is_zero_approx(p_track->tex_height) || !Math::is_zero_approx(p_track->tex_width) ||
                !Math::is_zero_approx(p_track->tex_slope));
    }

    MaszynaTrackRenderingBackend::Track *MaszynaTrackRenderingBackend::_find_track(const RID &p_track) {
        TrackRenderingServer *track_rendering = TrackRenderingServer::get_instance();
        if (track_rendering == nullptr || !p_track.is_valid()) {
            return nullptr;
        }
        const RID track_render = track_rendering->track_get_render_rid(p_track);
        if (!track_render.is_valid()) {
            return nullptr;
        }
        return tracks.getptr(track_render);
    }

    const MaszynaTrackRenderingBackend::RailProfile &
    MaszynaTrackRenderingBackend::_load_rail_profile(const String &p_name) {
        if (p_name.is_empty()) {
            return default_rail_profile;
        }
        if (const RailProfile *cached = rail_profiles.getptr(p_name); cached != nullptr) {
            return *cached;
        }
        MaszynaGameDirectory *game_directory = MaszynaGameDirectory::get_instance();
        ReloadedPluginServer *plugins = ReloadedPluginServer::get_instance();
        ERR_FAIL_NULL_V(game_directory, default_rail_profile);
        ERR_FAIL_NULL_V(plugins, default_rail_profile);
        const String profile_dir = game_directory->get_game_dir().path_join(RAIL_PROFILE_DIRECTORY);
        const String profile_path = profile_dir.path_join(plugins->data_path_resolve(
                profile_dir, vformat(RAIL_PROFILE_FILE, p_name.trim_prefix(RAIL_PROFILE_PREFIX))));
        const Ref<FileAccess> file = FileAccess::file_exists(profile_path)
                                             ? FileAccess::open(profile_path, FileAccess::READ)
                                             : Ref<FileAccess>();
        if (file.is_null()) {
            ERR_PRINT("Rail profile not found: " + p_name);
            return default_rail_profile;
        }

        RailProfile profile;
        bool read_blade_profile = false;
        while (!file->eof_reached()) {
            const String line = file->get_line().strip_edges();
            if (line.is_empty()) {
                continue;
            }
            if (line.begins_with(RAIL_PROFILE_COMMENT)) {
                if (line.begins_with(RAIL_PROFILE_BLADE_SECTION)) {
                    read_blade_profile = true;
                }
                continue;
            }
            const TypedArray<RegExMatch> matches = rail_profile_number->search_all(line);
            if (matches.size() < PROFILE_POINT_VALUE_COUNT) {
                continue;
            }
            double values[PROFILE_POINT_VALUE_COUNT] = {};
            for (int index = 0; index < PROFILE_POINT_VALUE_COUNT; index++) {
                values[index] = Ref<RegExMatch>(matches[index])->get_string().to_float();
            }
            ProfilePoint point;
            point.x = values[0];
            point.y = values[1];
            point.normal_x = values[2];
            point.normal_y = values[3];
            point.u = values[4];
            if (read_blade_profile) {
                profile.blade.push_back(point);
            } else {
                profile.rail.push_back(point);
            }
        }
        if (profile.rail.is_empty()) {
            ERR_PRINT("Rail profile has no rail: " + p_name);
            return default_rail_profile;
        }
        if (profile.blade.is_empty()) {
            profile.blade = profile.rail;
        }
        return rail_profiles.insert(p_name, profile)->value;
    }

    double MaszynaTrackRenderingBackend::_load_texture_length(const String &p_material_name) {
        if (const double *length = material_texture_lengths.getptr(p_material_name); length != nullptr) {
            return *length;
        }
        ReloadedMaterialServer *materials = ReloadedMaterialServer::get_instance();
        ERR_FAIL_NULL_V(materials, -1.0);
        const double length = materials->material_get_texture_length(p_material_name);
        material_texture_lengths.insert(p_material_name, length);
        return length;
    }

    double MaszynaTrackRenderingBackend::_load_texture_ratio(const String &p_material_name) {
        if (const double *ratio = material_texture_ratios.getptr(p_material_name); ratio != nullptr) {
            return *ratio;
        }
        ReloadedMaterialServer *materials = ReloadedMaterialServer::get_instance();
        ERR_FAIL_NULL_V(materials, 1.0);
        const Vector2i size =
                p_material_name.is_empty() ? Vector2i() : materials->material_get_texture_size(p_material_name);
        const double ratio = size.y > 0 ? static_cast<double>(size.x) / size.y : 1.0;
        material_texture_ratios.insert(p_material_name, ratio);
        return ratio;
    }

    double
    MaszynaTrackRenderingBackend::_load_road_texture_length(const Track &p_track, const String &p_material_name) {
        const double size_length = p_material_name.is_empty() ? -1.0 : _load_texture_length(p_material_name);
        return size_length >= 0.0 ? size_length : p_track.tex_length;
    }

    void MaszynaTrackRenderingBackend::_resolve_trackbed(
            const Track &p_track, String &p_r_material_name, double &p_r_texture_length) {
        RIDSet visited;
        const Track *owner = _copy_adjacent_trackbed_material(&p_track, visited);
        p_r_material_name = String();
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        if (owner != nullptr) {
            p_r_material_name =
                    owner->material_trackbed_name.is_empty() ? owner->material2_name : owner->material_trackbed_name;
        } else if (track_server->get_is_topology_changed() && looks.is_valid()) {
            // the neighbours are not known yet (the editor, until its Rebuild topology): the default
            // bed's trackbed, built again with the neighbour's once they are (topology_rebuilt())
            const Ref<MaszynaTrackLook> look = looks->get_rail_looks().get(looks->get_default_bed(), Variant());
            p_r_material_name = look.is_valid() ? look->get_material2() : String();
        }
        // TTrack::texture_length() (Track.cpp:2481-2493): the material's size, else the track's own
        const double size_length = p_r_material_name.is_empty() ? -1.0 : _load_texture_length(p_r_material_name);
        p_r_texture_length = size_length >= 0.0 ? size_length : p_track.tex_length;
    }

    /// Port of TTrack::copy_adjacent_trackbed_material() (Track.cpp:3326). A switch carries no
    /// trackbed texture of its own - its second slot holds the diverging path's rail texture - and
    /// the short connectors inside a switch group commonly declare "none", so the material is
    /// borrowed from a neighbour. p_visited generalises the original's single-level Exclude argument,
    /// which cannot stop a longer loop of track from recursing forever.
    /// Returns the track whose own trackbed material is used - its size sets the mapping too.
    const MaszynaTrackRenderingBackend::Track *
    MaszynaTrackRenderingBackend::_copy_adjacent_trackbed_material(const Track *p_track, RIDSet &p_visited) {
        if (p_track == nullptr || p_visited.has(p_track->track)) {
            return nullptr;
        }
        p_visited.insert(p_track->track);

        if (!p_track->material_trackbed_name.is_empty()) {
            return p_track;
        }
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL_V(track_server, nullptr);
        const bool is_switch = track_server->track_is_switch(p_track->track);
        if (!is_switch && !p_track->material2_name.is_empty()) {
            return p_track;
        }

        for (const RID &neighbor_track: _get_trackbed_material_sources(p_track->track, is_switch)) {
            if (const Track *owner = _copy_adjacent_trackbed_material(_find_track(neighbor_track), p_visited);
                owner != nullptr) {
                return owner;
            }
        }
        return nullptr;
    }

    /// Neighbours a track may borrow its trackbed material from: the main path's two for a switch
    /// (Track.cpp:3359), and for a regular track only while it sits next to a switch
    /// (Track.cpp:3348) - plain track keeps whatever the scenery gave it.
    Vector<RID>
    MaszynaTrackRenderingBackend::_get_trackbed_material_sources(const RID &p_track, const bool p_is_switch) const {
        Vector<RID> sources;
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL_V(track_server, sources);
        if (p_is_switch) {
            const Ref<TrackBranchNeighbors> neighbors =
                    track_server->track_get_branch_neighbors(p_track, TrackServer::TRACK_COMMON);
            sources.push_back(neighbors->get_previous_track_rid());
            sources.push_back(neighbors->get_next_track_rid());
            return sources;
        }

        bool has_adjacent_switch = false;
        for (const TrackServer::EndpointIndex endpoint_index: {TrackServer::CURVE1_P1, TrackServer::CURVE1_P2}) {
            const TypedArray<TrackEndpointRef> connections =
                    track_server->track_get_endpoint_connections(p_track, endpoint_index);
            for (int index = 0; index < connections.size(); index++) {
                const Ref<TrackEndpointRef> connection = connections[index];
                sources.push_back(connection->get_track_rid());
                has_adjacent_switch = has_adjacent_switch || track_server->track_is_switch(connection->get_track_rid());
            }
        }
        if (!has_adjacent_switch) {
            sources.clear();
        }
        return sources;
    }

    // Port of switch trackbed profile source selection from Track.cpp:2753-2809.
    void MaszynaTrackRenderingBackend::_build_switch_trackbed_sections(
            const Track &p_track, const TrackGeometry &p_geometry, const TrackServer::SwitchTrack p_switch_track,
            const double p_start_roll, const double p_end_roll, const double p_rail_height,
            const double p_texture_length, Section &p_r_start, Section &p_r_end) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        const Ref<TrackBranchNeighbors> neighbors =
                track_server->track_get_branch_neighbors(p_track.track, p_switch_track);
        const TrackGeometry previous_track = neighbors->get_previous_track_rid().is_valid()
                                                     ? _read_track_geometry(neighbors->get_previous_track_rid())
                                                     : TrackGeometry();
        const TrackGeometry next_track = neighbors->get_next_track_rid().is_valid()
                                                 ? _read_track_geometry(neighbors->get_next_track_rid())
                                                 : TrackGeometry();
        const Track *previous_state = _find_track(neighbors->get_previous_track_rid());
        const Track *next_state = _find_track(neighbors->get_next_track_rid());

        const Track *source_state = &p_track;
        if (previous_track.is_valid() && _has_trackbed_profile_options(previous_state) &&
            previous_track.curve2.is_null()) {
            source_state = previous_state;
        } else if (next_track.is_valid() && _has_trackbed_profile_options(next_state) && next_track.curve2.is_null()) {
            source_state = next_state;
        }

        const double start_tex_height =
                source_state->tex_height + MaszynaTrackMeshBuilder::roll_fix_height(p_start_roll);
        const double start_tex_width = source_state->tex_width;
        const double start_tex_slope = source_state->tex_slope;
        double end_width = p_geometry.width;
        double end_tex_height = start_tex_height;
        double end_tex_width = start_tex_width;
        double end_tex_slope = start_tex_slope;

        if (next_track.is_valid() && _has_trackbed_profile_options(next_state) && next_track.curve2.is_null()) {
            end_width = next_track.width;
            end_tex_height = next_state->tex_height + MaszynaTrackMeshBuilder::roll_fix_height(_get_track_roll(
                                                              next_track, neighbors->get_next_endpoint_index()));
            end_tex_width = next_state->tex_width;
            end_tex_slope = next_state->tex_slope;
        }

        p_r_start = MaszynaTrackMeshBuilder::build_trackbed_section(
                p_geometry.width, start_tex_height, start_tex_width, start_tex_slope, p_start_roll, p_rail_height,
                p_texture_length);
        p_r_end = MaszynaTrackMeshBuilder::build_trackbed_section(
                end_width, end_tex_height, end_tex_width, end_tex_slope, p_end_roll, p_rail_height, p_texture_length);
    }

    /// Port of the road drawing of TTrack::create_geometry() (Track.cpp:1479-1517): the surface of
    /// material1 across the track's width, then the shoulders - or the curbs and sidewalks - of
    /// material2 on both sides; a part without a material name gets no section. A road has no rails:
    /// its points are its surface (Track.cpp:552-559 lifts rail tracks only).
    void MaszynaTrackRenderingBackend::_read_road_piece(
            const Track &p_track, const TrackGeometry &p_geometry, TrackSnapshot &p_snapshot) {
        TrackPiece &piece = p_snapshot.piece;
        p_snapshot.primary_material = p_track.material1_name;
        p_snapshot.bed_material = p_track.material2_name;
        if (p_track.material1_name.is_empty() && p_track.material2_name.is_empty()) {
            return;
        }
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        const Ref<TrackCurve> &curve_data = p_geometry.curve1;
        RoadLook road;
        road.width = p_geometry.width;
        road.tex_height = p_track.tex_height;
        road.tex_width = p_track.tex_width;
        road.tex_slope = p_track.tex_slope;
        // iTrapezoid & 2 (Track.cpp:355-372): the next road, joined at its start, differs in its look -
        // this one turns into it along its length
        RoadLook next_road = road;
        bool next_road_differs = false;
        const Ref<TrackEndpointRef> next_connection = unique_endpoint_connection(p_track.track, TrackServer::CURVE1_P2);
        if (next_connection.is_valid() && next_connection->get_endpoint_index() == TrackServer::CURVE1_P1) {
            const TrackGeometry next_track = _read_track_geometry(next_connection->get_track_rid());
            const Track *next_state = _find_track(next_connection->get_track_rid());
            if (next_track.is_valid() && next_state != nullptr && next_track.curve2.is_null() &&
                track_server->track_get_group(next_connection->get_track_rid()) == TrackServer::GROUP_ROAD) {
                RoadLook candidate;
                candidate.width = next_track.width;
                candidate.tex_height = next_state->tex_height;
                candidate.tex_width = next_state->tex_width;
                candidate.tex_slope = next_state->tex_slope;
                if (!candidate.same_as(road)) {
                    next_road = candidate;
                    next_road_differs = true;
                }
            }
        }
        const double roll1 = Math::deg_to_rad(curve_data->get_roll1());
        const double roll2 = Math::deg_to_rad(curve_data->get_roll2());
        const bool transition = next_road_differs || !Math::is_zero_approx(roll1) || !Math::is_zero_approx(roll2);

        const double surface_length = _load_road_texture_length(p_track, p_track.material1_name);
        const double surface_ratio = _load_texture_ratio(p_track.material1_name);
        const Section surface_start =
                MaszynaTrackMeshBuilder::build_road_section(road.width, roll1, surface_length, surface_ratio);
        const Section surface_end = MaszynaTrackMeshBuilder::build_road_section(
                next_road.width, transition ? roll2 : roll1, surface_length, surface_ratio);

        if (!p_track.material1_name.is_empty()) {
            piece.surface_start = MaszynaTrackMeshBuilder::mirror_road_section(surface_start);
            piece.surface_end = MaszynaTrackMeshBuilder::mirror_road_section(surface_end);
            piece.primary_texture_length = surface_length;
        }

        if (!p_track.material2_name.is_empty()) {
            const double side_length = _load_road_texture_length(p_track, p_track.material2_name);
            const double side_ratio = _load_texture_ratio(p_track.material2_name);
            Section right_start;
            Section left_start;
            Section right_end;
            Section left_end;
            MaszynaTrackMeshBuilder::build_road_side_sections(
                    road, surface_start, side_length, side_ratio, right_start, left_start);
            MaszynaTrackMeshBuilder::build_road_side_sections(
                    next_road, surface_end, side_length, side_ratio, right_end, left_end);
            piece.bed_texture_length = side_length;
            // Track.cpp:1503-1514: a shoulder on both sides, a sidewalk only where it has a width
            if (road.tex_height >= 0.0 || !Math::is_zero_approx(road.tex_slope)) {
                piece.right_start = MaszynaTrackMeshBuilder::mirror_road_section(right_start);
                piece.right_end = MaszynaTrackMeshBuilder::mirror_road_section(right_end);
            }
            if (road.tex_height >= 0.0 || !Math::is_zero_approx(road.tex_width)) {
                piece.left_start = MaszynaTrackMeshBuilder::mirror_road_section(left_start);
                piece.left_end = MaszynaTrackMeshBuilder::mirror_road_section(left_end);
            }
        }
    }

    bool MaszynaTrackRenderingBackend::_read_track_piece(
            const RID &p_track_render, const Track &p_track, TrackSnapshot &p_snapshot) {
        const TrackGeometry geometry = _read_track_geometry(p_track.track);
        TrackServer *track_server = TrackServer::get_instance();
        if (!geometry.is_valid() || track_server == nullptr) {
            return false;
        }
        const Ref<TrackCurve> &curve1 = geometry.curve1;
        const Ref<TrackCurve> &curve2 = geometry.curve2;
        TrackPiece &piece = p_snapshot.piece;
        p_snapshot.track_render = p_track_render;
        piece.curve1 = _read_curve_shape(curve1);
        piece.has_curve2 = curve2.is_valid();
        if (piece.has_curve2) {
            piece.curve2 = _read_curve_shape(curve2);
        }
        piece.width = geometry.width;
        if (_is_road(p_track.track, geometry)) {
            piece.shape = MaszynaTrackMeshBuilder::PIECE_ROAD;
            _read_road_piece(p_track, geometry, p_snapshot);
            return true;
        }

        piece.shape = _is_switch_track(geometry) ? MaszynaTrackMeshBuilder::PIECE_SWITCH
                                                 : MaszynaTrackMeshBuilder::PIECE_RAILS;
        piece.rail_profile = _load_rail_profile(p_track.railprofile);
        piece.rail_height = Math::abs(piece.rail_profile.rail[0].y);
        piece.switch_is_right = piece.shape == MaszynaTrackMeshBuilder::PIECE_SWITCH &&
                                track_server->track_is_switch_right(p_track.track);
        piece.blade_layout = _switch_blade_layout(
                piece.switch_is_right, geometry.switch_f_offset1, geometry.switch_f_offset2,
                track_server->get_switch_max_offset());
        p_snapshot.primary_material = p_track.material1_name;
        // The second path of a switch carries its own rail texture; everything else, and a switch
        // that declares none, uses the track's own one.
        p_snapshot.secondary_material = piece.has_curve2 && !p_track.material2_name.is_empty() ? p_track.material2_name
                                                                                               : p_track.material1_name;
        piece.primary_texture_length = p_track.tex_length;
        _resolve_trackbed(p_track, p_snapshot.bed_material, piece.bed_texture_length);
        if (p_snapshot.bed_material.is_empty()) {
            return true;
        }

        const double rail_height = piece.rail_height;
        const double track_width = geometry.width;
        if (piece.shape == MaszynaTrackMeshBuilder::PIECE_SWITCH) {
            // Port of switch trackbed assembly from Track.cpp:3236-3302: each branch's sections by the
            // neighbours it has
            _build_switch_trackbed_sections(
                    p_track, geometry, TrackServer::TRACK_COMMON, curve1->get_roll1(), curve1->get_roll2(), rail_height,
                    piece.bed_texture_length, piece.bed_start, piece.bed_end);
            _build_switch_trackbed_sections(
                    p_track, geometry, TrackServer::TRACK_DIVERGING, curve2->get_roll1(), curve2->get_roll2(),
                    rail_height, piece.bed_texture_length, piece.second_bed_start, piece.second_bed_end);
            return true;
        }

        double end_width = track_width;
        double end_tex_height = p_track.tex_height + MaszynaTrackMeshBuilder::roll_fix_height(curve1->get_roll1());
        double end_tex_width = p_track.tex_width;
        double end_tex_slope = p_track.tex_slope;
        const Ref<TrackEndpointRef> next_connection = unique_endpoint_connection(p_track.track, TrackServer::CURVE1_P2);
        const RID next_track_rid = next_connection.is_valid() ? next_connection->get_track_rid() : RID();
        const TrackGeometry next_track =
                next_track_rid.is_valid() ? _read_track_geometry(next_track_rid) : TrackGeometry();
        const Track *next_track_state = _find_track(next_track_rid);

        if (next_connection.is_valid() && next_track.is_valid() && next_track_state != nullptr &&
            next_track.curve2.is_null()) {
            end_width = next_track.width;
            end_tex_height = next_track_state->tex_height + MaszynaTrackMeshBuilder::roll_fix_height(_get_track_roll(
                                                                    next_track, next_connection->get_endpoint_index()));
            end_tex_width = next_track_state->tex_width;
            end_tex_slope = next_track_state->tex_slope;
        }

        piece.bed_start = MaszynaTrackMeshBuilder::build_trackbed_section(
                track_width, p_track.tex_height + MaszynaTrackMeshBuilder::roll_fix_height(curve1->get_roll1()),
                p_track.tex_width, p_track.tex_slope, curve1->get_roll1(), rail_height, piece.bed_texture_length);
        piece.bed_end = MaszynaTrackMeshBuilder::build_trackbed_section(
                end_width, end_tex_height, end_tex_width, end_tex_slope, curve1->get_roll2(), rail_height,
                piece.bed_texture_length);
        if (curve2.is_valid()) {
            piece.second_bed_start = MaszynaTrackMeshBuilder::build_trackbed_section(
                    track_width, p_track.tex_height + MaszynaTrackMeshBuilder::roll_fix_height(curve2->get_roll1()),
                    p_track.tex_width, p_track.tex_slope, curve2->get_roll1(), rail_height, piece.bed_texture_length);
            piece.second_bed_end = MaszynaTrackMeshBuilder::build_trackbed_section(
                    track_width, p_track.tex_height + MaszynaTrackMeshBuilder::roll_fix_height(curve2->get_roll2()),
                    p_track.tex_width, p_track.tex_slope, curve2->get_roll2(), rail_height, piece.bed_texture_length);
        }
        return true;
    }

    void MaszynaTrackRenderingBackend::_slot_draw(Track &p_track, const Slot p_slot) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        RID &instance = p_track.instances[p_slot];
        if (!instance.is_valid()) {
            instance = rendering->instance_create();
            if (p_track.scenario.is_valid()) {
                rendering->instance_set_scenario(instance, p_track.scenario);
            }
            rendering->instance_set_visible(instance, p_track.visible);
            if (p_slot == SLOT_TRACKBED) {
                // The trackbed casts no sun shadow: a 0.2 m bed barely shades the ground, but its slopes,
                // lit at a grazing angle, shaded themselves - a dark band that read as a gap to the
                // terrain (FINDINGS.md, 2026-09-28)
                rendering->instance_geometry_set_cast_shadows_setting(
                        instance, RenderingServer::SHADOW_CASTING_SETTING_OFF);
            }
        }
        rendering->instance_set_base(instance, p_track.meshes[p_slot]->get_rid());
        // a blade is placed as the switch sets it (track_set_switch_blade_offsets())
        if (p_slot != SLOT_PRIMARY_BLADE && p_slot != SLOT_SECONDARY_BLADE) {
            rendering->instance_set_transform(instance, Transform3D(Basis(), p_track.origin));
        }
    }

    void MaszynaTrackRenderingBackend::_reset_track_instances(Track &p_track) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (int slot = 0; slot < SLOT_MAX; slot++) {
            const RID &instance = p_track.instances[slot];
            if (!instance.is_valid()) {
                continue;
            }
            rendering->instance_set_base(instance, RID());
            if (slot == SLOT_PRIMARY_BLADE || slot == SLOT_SECONDARY_BLADE) {
                rendering->instance_set_transform(instance, Transform3D());
            }
        }
    }

    void MaszynaTrackRenderingBackend::_clear_track_meshes(Track &p_track) {
        _reset_track_instances(p_track);
        for (Ref<ArrayMesh> &mesh: p_track.meshes) {
            mesh.unref();
        }
        p_track.primary_blade.pose = BladePose();
        p_track.secondary_blade.pose = BladePose();
    }

    void MaszynaTrackRenderingBackend::_cell_add(const RID &p_track_render, Track &p_track) {
        const TrackGeometry geometry = _read_track_geometry(p_track.track);
        ERR_FAIL_COND(!geometry.is_valid());
        const Vector3 start = geometry.curve1->get_p1();
        const Vector2i coords(
                static_cast<int32_t>(Math::floor(start.x / CELL_SIZE)),
                static_cast<int32_t>(Math::floor(start.z / CELL_SIZE)));
        const CellKey key{p_track.scenario, coords, _is_road(p_track.track, geometry) ? CELL_ROADS : CELL_TRACKS};
        RID cell_rid;
        if (const RID *found = cells_by_key.getptr(key); found != nullptr) {
            cell_rid = *found;
        } else {
            const Vector2 centre = (Vector2(coords) + Vector2(0.5, 0.5)) * static_cast<real_t>(CELL_SIZE);
            Cell cell;
            cell.kind = key.kind;
            cell.scenario = p_track.scenario;
            cell.coords = coords;
            cell.origin = Vector3(centre.x, start.y, centre.y);
            cell_rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
            {
                MutexLock lock(cells_mutex);
                cells.insert(cell_rid, cell);
            }
            cells_by_key.insert(key, cell_rid);
        }
        Cell *cell = cells.getptr(cell_rid);
        if (!cell->tracks.has(p_track_render)) {
            cell->tracks.push_back(p_track_render);
        }
        p_track.cell = cell_rid;
        _cell_update_track(p_track_render, p_track);
    }

    /// The track's blades go with it, and the cell with its last track
    void MaszynaTrackRenderingBackend::_cell_remove(const RID &p_track_render, Track &p_track) {
        const RID cell_rid = p_track.cell;
        p_track.cell = RID();
        _clear_track_meshes(p_track);
        Cell *cell = cells.getptr(cell_rid);
        if (cell == nullptr) {
            return;
        }
        cell->tracks.erase(p_track_render);
        {
            MutexLock lock(cells_mutex);
            for (int index = 0; index < cell->snapshots.size(); index++) {
                if (cell->snapshots[index].track_render == p_track_render) {
                    cell->snapshots.remove_at(index);
                    break;
                }
            }
        }
        if (!cell->tracks.is_empty()) {
            _cell_stream(cell_rid, *cell);
            return;
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->stream_free(cell->stream_rid);
        }
        _cell_clear(cell_rid);
        for (const RID &instance: cell->instances) {
            if (instance.is_valid()) {
                RenderingServer::get_singleton()->free_rid(instance);
            }
        }
        cells_by_key.erase(CellKey{cell->scenario, cell->coords, cell->kind});
        MutexLock lock(cells_mutex);
        cells.erase(cell_rid);
    }

    void MaszynaTrackRenderingBackend::_cell_update_track(const RID &p_track_render, const Track &p_track) {
        Cell *cell = cells.getptr(p_track.cell);
        ERR_FAIL_NULL(cell);
        TrackSnapshot snapshot;
        const bool drawn = p_track.visible && _read_track_piece(p_track_render, p_track, snapshot);
        {
            MutexLock lock(cells_mutex);
            int index = 0;
            while (index < cell->snapshots.size() && cell->snapshots[index].track_render != p_track_render) {
                index++;
            }
            if (index == cell->snapshots.size()) {
                if (drawn) {
                    cell->snapshots.push_back(snapshot);
                }
            } else if (drawn) {
                cell->snapshots.write[index] = snapshot;
            } else {
                cell->snapshots.remove_at(index);
            }
        }
        _cell_stream(p_track.cell, *cell);
    }

    void MaszynaTrackRenderingBackend::_cell_stream(const RID &p_cell_rid, Cell &p_cell) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        TrackRenderingServer *track_rendering = TrackRenderingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        ERR_FAIL_NULL(track_rendering);
        if (p_cell.stream_rid.is_valid()) {
            streaming->stream_free(p_cell.stream_rid);
        }
        if (p_cell.streamed) {
            _cell_clear(p_cell_rid);
        }
        p_cell.stream_rid = streaming->stream_register(
                stream_owners[p_cell.kind], p_cell_rid, p_cell.origin,
                static_cast<float>(track_rendering->get_tracks_distance()));
    }

    Variant MaszynaTrackRenderingBackend::_cell_preload(const RID &p_cell) {
        Vector<TrackSnapshot> snapshots;
        Vector3 origin;
        {
            MutexLock lock(cells_mutex);
            const Cell *cell = cells.getptr(p_cell);
            if (cell == nullptr) {
                return {};
            }
            snapshots = cell->snapshots;
            origin = cell->origin;
        }
        // by layer, by the material's name
        HashMap<String, Geometry> layers[CELL_LAYER_MAX];
        Array blades;
        for (const TrackSnapshot &snapshot: snapshots) {
            const TrackPiece &piece = snapshot.piece;
            const PieceGeometry geometry = MaszynaTrackMeshBuilder::build_piece_geometry(piece);
            // a road's surface lies on the ground as its sides do
            const CellLayer rails_layer =
                    piece.shape == MaszynaTrackMeshBuilder::PIECE_ROAD ? CELL_LAYER_BED : CELL_LAYER_RAILS;
            MaszynaTrackMeshBuilder::append_geometry(
                    layers[rails_layer][snapshot.primary_material], geometry.primary_rails);
            MaszynaTrackMeshBuilder::append_geometry(
                    layers[rails_layer][snapshot.secondary_material], geometry.secondary_rails);
            MaszynaTrackMeshBuilder::append_geometry(layers[CELL_LAYER_BED][snapshot.bed_material], geometry.bed);
            if (piece.shape != MaszynaTrackMeshBuilder::PIECE_SWITCH) {
                continue;
            }
            Array blade;
            blade.resize(BLADES_FIELD_COUNT);
            blade[BLADES_TRACK_RENDER] = snapshot.track_render;
            blade[BLADES_ORIGIN] = piece.curve1.p1;
            blade[BLADES_SWITCH_IS_RIGHT] = piece.switch_is_right;
            blade[BLADES_PRIMARY_MATERIAL] = snapshot.primary_material;
            blade[BLADES_SECONDARY_MATERIAL] = snapshot.secondary_material;
            blade[BLADES_PRIMARY_ARRAYS] =
                    MaszynaTrackMeshBuilder::build_surface_arrays(geometry.primary_blade, piece.curve1.p1);
            blade[BLADES_SECONDARY_ARRAYS] =
                    MaszynaTrackMeshBuilder::build_surface_arrays(geometry.secondary_blade, piece.curve1.p1);
            blade[BLADES_PRIMARY_PIVOT] = geometry.primary_blade_pose.pivot;
            blade[BLADES_PRIMARY_ANGLE_FACTOR] = geometry.primary_blade_pose.angle_factor;
            blade[BLADES_SECONDARY_PIVOT] = geometry.secondary_blade_pose.pivot;
            blade[BLADES_SECONDARY_ANGLE_FACTOR] = geometry.secondary_blade_pose.angle_factor;
            blades.push_back(blade);
        }
        Array surfaces;
        surfaces.resize(CELL_LAYER_MAX);
        for (int layer = 0; layer < CELL_LAYER_MAX; layer++) {
            Array layer_surfaces;
            for (const KeyValue<String, Geometry> &item: layers[layer]) {
                if (item.value.vertices.is_empty()) {
                    continue;
                }
                Array surface;
                surface.resize(SURFACE_FIELD_COUNT);
                surface[SURFACE_MATERIAL] = item.key;
                surface[SURFACE_ARRAYS] = MaszynaTrackMeshBuilder::build_surface_arrays(item.value, origin);
                layer_surfaces.push_back(surface);
            }
            surfaces[layer] = layer_surfaces;
        }
        Array preloaded;
        preloaded.resize(PRELOADED_CELL_FIELD_COUNT);
        preloaded[PRELOADED_SURFACES] = surfaces;
        preloaded[PRELOADED_BLADES] = blades;
        return preloaded;
    }

    /// The meshes of the layers, a surface per material, each in the layer's instance; the switches'
    /// blades in their tracks' instances, placed as the switches stand
    void MaszynaTrackRenderingBackend::_cell_build(const RID &p_cell, const Variant &p_preloaded) {
        Cell *cell = cells.getptr(p_cell);
        if (cell == nullptr) {
            return;
        }
        cell->streamed = true;
        const Array preloaded = p_preloaded;
        ERR_FAIL_COND(preloaded.size() != PRELOADED_CELL_FIELD_COUNT);
        RenderingServer *rendering = RenderingServer::get_singleton();
        const Array surfaces = preloaded[PRELOADED_SURFACES];
        for (int layer = 0; layer < CELL_LAYER_MAX; layer++) {
            const Array layer_surfaces = surfaces[layer];
            RID &instance = cell->instances[layer];
            if (layer_surfaces.is_empty()) {
                cell->meshes[layer].unref();
                if (instance.is_valid()) {
                    rendering->instance_set_base(instance, RID());
                }
                continue;
            }
            Ref<ArrayMesh> mesh;
            mesh.instantiate();
            for (int index = 0; index < layer_surfaces.size(); index++) {
                const Array surface = layer_surfaces[index];
                _mesh_add_surface(mesh, surface[SURFACE_ARRAYS], surface[SURFACE_MATERIAL]);
            }
            cell->meshes[layer] = mesh;
            if (!instance.is_valid()) {
                instance = rendering->instance_create();
                rendering->instance_set_scenario(instance, cell->scenario);
                rendering->instance_set_transform(instance, Transform3D(Basis(), cell->origin));
                if (layer == CELL_LAYER_BED) {
                    // The bed casts no sun shadow: a 0.2 m trackbed barely shades the ground, but its
                    // slopes, lit at a grazing angle, shaded themselves - a dark band that read as a gap
                    // to the terrain (FINDINGS.md, 2026-09-28); a road's shoulders did the same
                    rendering->instance_geometry_set_cast_shadows_setting(
                            instance, RenderingServer::SHADOW_CASTING_SETTING_OFF);
                }
            }
            rendering->instance_set_base(instance, mesh->get_rid());
        }
        const Array blades = preloaded[PRELOADED_BLADES];
        for (int index = 0; index < blades.size(); index++) {
            const Array blade = blades[index];
            const RID track_render = blade[BLADES_TRACK_RENDER];
            Track *track = tracks.getptr(track_render);
            ERR_CONTINUE(track == nullptr);
            track->origin = blade[BLADES_ORIGIN];
            track->switch_is_right = blade[BLADES_SWITCH_IS_RIGHT];
            struct BladeSide {
                    Slot slot;
                    PreloadedBlades arrays;
                    PreloadedBlades material;
            };
            for (const BladeSide &side:
                 {BladeSide{SLOT_PRIMARY_BLADE, BLADES_PRIMARY_ARRAYS, BLADES_PRIMARY_MATERIAL},
                  BladeSide{SLOT_SECONDARY_BLADE, BLADES_SECONDARY_ARRAYS, BLADES_SECONDARY_MATERIAL}}) {
                Ref<ArrayMesh> mesh;
                mesh.instantiate();
                _mesh_add_surface(mesh, blade[side.arrays], blade[side.material]);
                track->meshes[side.slot] = mesh;
                _slot_draw(*track, side.slot);
            }
            track->primary_blade.pose.pivot = blade[BLADES_PRIMARY_PIVOT];
            track->primary_blade.pose.angle_factor = blade[BLADES_PRIMARY_ANGLE_FACTOR];
            track->secondary_blade.pose.pivot = blade[BLADES_SECONDARY_PIVOT];
            track->secondary_blade.pose.angle_factor = blade[BLADES_SECONDARY_ANGLE_FACTOR];
            const TrackGeometry geometry = _read_track_geometry(track->track);
            track_set_switch_blade_offsets(track_render, geometry.switch_f_offset1, geometry.switch_f_offset2);
        }
    }

    /// The cell's meshes go, and its tracks' blades with them
    void MaszynaTrackRenderingBackend::_cell_clear(const RID &p_cell) {
        Cell *cell = cells.getptr(p_cell);
        if (cell == nullptr) {
            return;
        }
        cell->streamed = false;
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (int layer = 0; layer < CELL_LAYER_MAX; layer++) {
            if (cell->instances[layer].is_valid()) {
                rendering->instance_set_base(cell->instances[layer], RID());
            }
            cell->meshes[layer].unref();
        }
        for (const RID &track_render: cell->tracks) {
            if (Track *track = tracks.getptr(track_render); track != nullptr) {
                _clear_track_meshes(*track);
            }
        }
    }

    /// The rail profiles and the materials' sizes are the old data's
    void MaszynaTrackRenderingBackend::_on_data_unload_requested() {
        rail_profiles.clear();
        material_texture_lengths.clear();
        material_texture_ratios.clear();
    }
} // namespace godot
