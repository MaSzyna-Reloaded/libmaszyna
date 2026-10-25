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
        constexpr Vector3 UP(0.0, 1.0, 0.0);
        constexpr Vector3 RIGHT(1.0, 0.0, 0.0);
        constexpr Vector3 LEFT(-1.0, 0.0, 0.0);
        constexpr Vector3 FORWARD(0.0, 0.0, -1.0);

        /// Original engine: Segment.cpp:128-140 - a switch's curves are drawn in this many segments
        constexpr int SWITCH_SEGMENT_COUNT = 6;
        /// Original engine: Track.cpp:1371 bladelength - the blade's share of the way to the frog
        constexpr double SWITCH_BLADE_RATIO = 0.65;
        /// A track is drawn in segments of this length at least and at most [m]
        constexpr double MIN_CURVED_MESH_SEGMENT_LENGTH = 2.0;
        constexpr double MAX_MESH_SEGMENT_LENGTH = 10.0;
        /// An arc's segment as a share of its radius, a curve's as a share of its chord
        constexpr double ARC_SEGMENT_PER_RADIUS = 0.02;
        constexpr double CURVE_SEGMENT_PER_CHORD = 0.1;
        /// Original engine: Segment.cpp:84 - a roll lifts the curve by sin(roll) times this [m] (0.75 m
        /// for 1435 mm)
        constexpr double ROLL_FIX_FACTOR = 0.75;
        /// The switch trackbed's edges under the other curve go this much lower, so the two do not fight
        /// where they overlap [m]
        constexpr double SWITCH_TRACKBED_Z_FIGHT_OFFSET = 0.025;
        /// Original engine: Track.cpp:2860-2880 - a trackbed section: its outer edges, its slopes' upper
        /// edges and its crown
        constexpr int TRACKBED_SECTION_POINT_COUNT = 5;
        /// The trackbed section's points of the slopes' upper edges and of the crown (_build_trackbed_section())
        constexpr int TRACKBED_LEFT_EDGE_POINT = 1;
        constexpr int TRACKBED_CENTRE_POINT = 2;
        constexpr int TRACKBED_RIGHT_EDGE_POINT = 3;
        /// Original engine: Segment.cpp:452-492 - a strip chunk holds each section point twice, at the
        /// segment's start and at its end
        constexpr int STRIP_VERTICES_PER_POINT = 2;
        /// Original engine: Track.cpp:2855 - a trackbed texture 4 m long keeps the old fixed mapping
        constexpr double TRACKBED_OLD_MAPPING_TEXTURE_LENGTH = 4.0;
        /// Original engine: Track.cpp:2857-2858 - the old mapping's slope normals, 75 degrees up
        constexpr double TRACKBED_OLD_MAPPING_NORMAL_ANGLE = 75.0;
        /// Original engine: Track.cpp:2869, 2877 - the old mapping's u at the slope's upper edges
        constexpr double TRACKBED_OLD_MAPPING_BREAK_U = 0.33;
        /// Original engine: Track.cpp:2872 - the trackbed's crown lies this much over its edges [m]
        constexpr double TRACKBED_CROWN_LIFT = 0.01;
        /// The least length a texture or a loft is divided by [m]
        constexpr double MIN_LENGTH = 0.001;
        /// A tangent or a parallel shorter than this, squared, gives no direction
        constexpr double MIN_DIRECTION_LENGTH_SQUARED = 0.000001;
        /// A tangent this close to the vertical takes another reference for its frame
        constexpr double VERTICAL_TANGENT_DOT = 0.999;
        /// Step along both curves of a switch while looking for the frog, where they part [m]
        constexpr double FROG_SEARCH_STEP = 0.5;
        /// A switch's curve going on past its frog by more than this has its tail drawn too [m], in
        /// segments of about SWITCH_TAIL_SEGMENT_LENGTH [m]
        constexpr double SWITCH_TAIL_MIN_LENGTH = 0.05;
        constexpr double SWITCH_TAIL_SEGMENT_LENGTH = 2.0;
        /// Original engine: utilities.h:277 clamp_circular() - the switch trackbed's v wraps within this
        /// range, from its top
        constexpr double TEXTURE_V_RANGE = 1.0;
        /// Original engine: Track.cpp:3022 - a road texture of no known width is cut in its middle
        constexpr double ROAD_DEFAULT_MAP = 0.5;
        /// Original engine: Track.cpp:3160-3163 - a curb's face is sloped 1:3.75 (150 mm high, it leans
        /// 40 mm)
        constexpr double CURB_SLOPE = 3.75;
        /// Original engine: Track.cpp:3162 - the curbs are mapped from 31/64 to 32/64 and from 32/64 to
        /// 33/64 of the texture
        constexpr double CURB_LEFT_U = 0.484375;
        constexpr double CURB_RIGHT_U = 0.515625;
        /// Original engine: Track.cpp:3192 - the lean of a curb's face over this is its share of u
        constexpr double CURB_LEAN_PER_U = 2.56;
        /// The loft goes on to the curve's end, its last sampled point
        constexpr int CURVE_END = -1;
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
    } // namespace

    void MaszynaTrackRenderingBackend::_bind_methods() {
        BIND_ENUM_CONSTANT(SLOT_PRIMARY_RAIL);
        BIND_ENUM_CONSTANT(SLOT_SECONDARY_RAIL);
        BIND_ENUM_CONSTANT(SLOT_PRIMARY_BLADE);
        BIND_ENUM_CONSTANT(SLOT_SECONDARY_BLADE);
        BIND_ENUM_CONSTANT(SLOT_TRACKBED);
        BIND_ENUM_CONSTANT(SLOT_MAX);

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
            game_data->connect(
                    GameDataServer::data_unload_requested_signal,
                    callable_mp(this, &MaszynaTrackRenderingBackend::_on_data_unload_requested));
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &MaszynaTrackRenderingBackend::_on_data_reload_requested));
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            // the owner lives as long as the streaming does
            road_stream_owner = streaming->owner_create(
                    "roads", Callable(), callable_mp(this, &MaszynaTrackRenderingBackend::_road_cell_build),
                    callable_mp(this, &MaszynaTrackRenderingBackend::_road_cell_clear));
        }
        looks = ResourceLoader::get_singleton()->load(LOOKS_PATH);
        ERR_FAIL_COND_MSG(looks.is_null(), "MaszynaTrackRenderingBackend: no track looks at " + String(LOOKS_PATH));
    }

    MaszynaTrackRenderingBackend::~MaszynaTrackRenderingBackend() {
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->disconnect(
                    GameDataServer::data_unload_requested_signal,
                    callable_mp(this, &MaszynaTrackRenderingBackend::_on_data_unload_requested));
            game_data->disconnect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &MaszynaTrackRenderingBackend::_on_data_reload_requested));
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
        for (const KeyValue<RID, RoadCell> &item: road_cells) {
            if (streaming != nullptr) {
                streaming->stream_free(item.value.stream_rid);
            }
            if (item.value.instance.is_valid()) {
                rendering->free_rid(item.value.instance);
            }
        }
    }

    bool MaszynaTrackRenderingBackend::TrackGeometry::is_valid() const {
        return track.is_valid() && curve1.is_valid();
    }

    bool MaszynaTrackRenderingBackend::RoadLook::same_as(const RoadLook &p_other) const {
        return width == p_other.width && tex_height == p_other.tex_height && tex_width == p_other.tex_width &&
               tex_slope == p_other.tex_slope;
    }

    bool MaszynaTrackRenderingBackend::RoadCellKey::operator==(const RoadCellKey &p_other) const {
        return scenario == p_other.scenario && coords == p_other.coords;
    }

    uint32_t MaszynaTrackRenderingBackend::RoadCellKeyHasher::hash(const RoadCellKey &p_key) {
        return hash_murmur3_one_64(p_key.scenario.get_id(), HashMapHasherDefault::hash(p_key.coords));
    }

    void MaszynaTrackRenderingBackend::track_create(const RID &p_track_render, const RID &p_track) {
        Track track;
        track.track = p_track;
        tracks.insert(p_track_render, track);
    }

    /// The track's meshes and instances go, and a road leaves its cell
    void MaszynaTrackRenderingBackend::track_free(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        if (track->road_cell.is_valid()) {
            _road_cell_remove(p_track_render, *track);
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
    /// section, any other with those its features map to (looks)
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
            return;
        }
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
        if (RoadCell *cell = road_cells.getptr(track->road_cell); cell != nullptr) {
            _queue_road_cell_rebuild(*cell);
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
        if (track->road_cell.is_valid()) {
            // into the cell of its new scenario
            _road_cell_remove(p_track_render, *track);
            _road_cell_add(p_track_render, *track);
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
                    _build_blade_transform(
                            track->primary_blade.pivot, track->primary_blade.angle_factor,
                            layout.primary_blade_offset));
        }
        if (track->instances[SLOT_SECONDARY_BLADE].is_valid()) {
            rendering->instance_set_transform(
                    track->instances[SLOT_SECONDARY_BLADE],
                    _build_blade_transform(
                            track->secondary_blade.pivot, track->secondary_blade.angle_factor,
                            layout.secondary_blade_offset));
        }
    }

    bool MaszynaTrackRenderingBackend::track_stream(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, false);
        if (!_is_road(track->track, _read_track_geometry(track->track))) {
            return false;
        }
        _road_cell_add(p_track_render, *track);
        return true;
    }

    void MaszynaTrackRenderingBackend::track_build(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        if (track->road_cell.is_valid()) {
            if (RoadCell *cell = road_cells.getptr(track->road_cell); cell != nullptr) {
                _queue_road_cell_rebuild(*cell);
            }
            return;
        }
        TrackServer *track_server = TrackServer::get_instance();
        if (track_server == nullptr) {
            return;
        }
        const TrackGeometry geometry = _read_track_geometry(track->track);
        if (!geometry.is_valid()) {
            return;
        }

        _clear_track_meshes(*track);
        _resolve_materials(*track);

        const Ref<TrackCurve> &curve1_data = geometry.curve1;
        const Ref<TrackCurve> &curve2_data = geometry.curve2;
        const double track_width = geometry.width;
        const Ref<Curve3D> curve1 = _build_curve(curve1_data);

        if (curve1->get_baked_length() == 0.0) {
            return;
        }

        if (_is_road(track->track, geometry)) {
            _build_road(*track, geometry, curve1);
            return;
        }

        const RailProfile &rail_profile = _load_rail_profile(track->railprofile);
        const Ref<Material> trackbed_material = _resolve_trackbed_material(*track);

        if (_is_switch_track(geometry)) {
            const Ref<Curve3D> curve2 = _build_curve(curve2_data);

            Geometry primary;
            Geometry secondary;
            Geometry primary_blade;
            Geometry secondary_blade;
            const Vector<ProfilePoint> &regular_profile = rail_profile.rail;
            const Vector<ProfilePoint> &blade_profile = rail_profile.blade;
            const double rail_height = Math::abs(regular_profile[0].y);
            const bool is_right_switch = track_server->track_is_switch_right(track->track);
            track->switch_is_right = is_right_switch;
            // MaSzyna Track.cpp:1367-1453 emits switch rail segments.
            const double frog_dist = _find_switch_frog_distance(curve1, curve2, track_width);
            const PackedVector3Array primary_sampled_points = _sample_switch_curve_points(curve1, frog_dist);
            const PackedVector3Array secondary_sampled_points = _sample_switch_curve_points(curve2, frog_dist);
            const int blade_sample_count =
                    static_cast<int>(Math::ceil(static_cast<double>(SWITCH_SEGMENT_COUNT) * SWITCH_BLADE_RATIO));
            const SwitchBladeLayout layout = _switch_blade_layout(
                    is_right_switch, geometry.switch_f_offset1, geometry.switch_f_offset2,
                    track_server->get_switch_max_offset());

            _append_switch_fixed_rail_geometry(
                    primary, primary_sampled_points, regular_profile, track_width, curve1_data->get_roll1(),
                    curve1_data->get_roll2(), track->tex_length, rail_height, layout.primary_other_mirrored,
                    layout.primary_blade_mirrored, blade_sample_count);
            _append_switch_fixed_rail_geometry(
                    secondary, secondary_sampled_points, regular_profile, track_width, curve2_data->get_roll1(),
                    curve2_data->get_roll2(), track->tex_length, rail_height, layout.secondary_other_mirrored,
                    layout.secondary_blade_mirrored, blade_sample_count);
            _append_switch_blade_geometry(
                    primary_blade, primary_sampled_points, regular_profile, blade_profile, track_width,
                    curve1_data->get_roll1(), curve1_data->get_roll2(), track->tex_length, rail_height,
                    layout.primary_blade_mirrored, blade_sample_count);
            _append_switch_blade_geometry(
                    secondary_blade, secondary_sampled_points, regular_profile, blade_profile, track_width,
                    curve2_data->get_roll1(), curve2_data->get_roll2(), track->tex_length, rail_height,
                    layout.secondary_blade_mirrored, blade_sample_count);
            track->meshes[SLOT_PRIMARY_RAIL] = _build_array_mesh(primary);
            track->meshes[SLOT_SECONDARY_RAIL] = _build_array_mesh(secondary);

            track->meshes[SLOT_PRIMARY_BLADE] = _build_array_mesh(primary_blade);
            track->primary_blade.move_direction = -_build_planar_parallel(primary_sampled_points, 0);
            track->primary_blade.pivot = _get_switch_blade_pivot(
                    primary_sampled_points, regular_profile, track_width, curve1_data->get_roll1(),
                    curve1_data->get_roll2(), layout.primary_blade_mirrored, blade_sample_count, rail_height);
            track->primary_blade.angle_factor = _get_blade_angle_factor(
                    primary_sampled_points, blade_profile, track_width, curve1_data->get_roll1(),
                    layout.primary_blade_mirrored, rail_height, track->primary_blade.move_direction,
                    track->primary_blade.pivot);
            track->primary_blade.offset = layout.primary_blade_offset;

            track->meshes[SLOT_SECONDARY_BLADE] = _build_array_mesh(secondary_blade);
            track->secondary_blade.move_direction = -_build_planar_parallel(secondary_sampled_points, 0);
            track->secondary_blade.pivot = _get_switch_blade_pivot(
                    secondary_sampled_points, regular_profile, track_width, curve2_data->get_roll1(),
                    curve2_data->get_roll2(), layout.secondary_blade_mirrored, blade_sample_count, rail_height);
            track->secondary_blade.angle_factor = _get_blade_angle_factor(
                    secondary_sampled_points, blade_profile, track_width, curve2_data->get_roll1(),
                    layout.secondary_blade_mirrored, rail_height, track->secondary_blade.move_direction,
                    track->secondary_blade.pivot);
            track->secondary_blade.offset = layout.secondary_blade_offset;

            if (trackbed_material.is_valid()) {
                // Port of switch trackbed assembly from Track.cpp:3236-3302.
                track->meshes[SLOT_TRACKBED] =
                        _build_switch_trackbed_mesh(*track, geometry, curve1, curve2, rail_height, is_right_switch);
            }
        } else {
            Geometry rails;
            const double rail_height = Math::abs(rail_profile.rail[0].y);
            const Section right_start =
                    _build_rail_section(rail_profile.rail, track_width, curve1_data->get_roll1(), false);
            const Section right_end =
                    _build_rail_section(rail_profile.rail, track_width, curve1_data->get_roll2(), false);
            const Section left_start =
                    _build_rail_section(rail_profile.rail, track_width, curve1_data->get_roll1(), true);
            const Section left_end =
                    _build_rail_section(rail_profile.rail, track_width, curve1_data->get_roll2(), true);

            const PackedVector3Array curve1_points = curve1->get_baked_points();
            _append_loft_geometry_range_points(
                    rails, curve1_points, right_start, right_end, track->tex_length, rail_height, 0, CURVE_END);
            _append_loft_geometry_range_points(
                    rails, curve1_points, left_start, left_end, track->tex_length, rail_height, 0, CURVE_END);
            track->meshes[SLOT_PRIMARY_RAIL] = _build_array_mesh(rails);

            if (curve2_data.is_valid()) {
                const Ref<Curve3D> secondary_curve = _build_curve(curve2_data);
                const PackedVector3Array secondary_points = secondary_curve->get_baked_points();
                Geometry secondary;
                const Section second_right_start =
                        _build_rail_section(rail_profile.rail, track_width, curve2_data->get_roll1(), false);
                const Section second_right_end =
                        _build_rail_section(rail_profile.rail, track_width, curve2_data->get_roll2(), false);
                const Section second_left_start =
                        _build_rail_section(rail_profile.rail, track_width, curve2_data->get_roll1(), true);
                const Section second_left_end =
                        _build_rail_section(rail_profile.rail, track_width, curve2_data->get_roll2(), true);
                _append_loft_geometry_range_points(
                        secondary, secondary_points, second_right_start, second_right_end, track->tex_length,
                        rail_height, 0, CURVE_END);
                _append_loft_geometry_range_points(
                        secondary, secondary_points, second_left_start, second_left_end, track->tex_length, rail_height,
                        0, CURVE_END);
                track->meshes[SLOT_SECONDARY_RAIL] = _build_array_mesh(secondary);
            }

            if (trackbed_material.is_valid()) {
                Geometry trackbed;
                double end_width = track_width;
                double end_tex_height = track->tex_height + _get_roll_fix_height(curve1_data->get_roll1());
                double end_tex_width = track->tex_width;
                double end_tex_slope = track->tex_slope;
                const Ref<TrackEndpointRef> next_connection =
                        unique_endpoint_connection(track->track, TrackServer::CURVE1_P2);
                const RID next_track_rid = next_connection.is_valid() ? next_connection->get_track_rid() : RID();
                const TrackGeometry next_track =
                        next_track_rid.is_valid() ? _read_track_geometry(next_track_rid) : TrackGeometry();
                const Track *next_track_state = _find_track(next_track_rid);

                if (next_connection.is_valid() && next_track.is_valid() && next_track_state != nullptr &&
                    next_track.curve2.is_null()) {
                    end_width = next_track.width;
                    end_tex_height =
                            next_track_state->tex_height +
                            _get_roll_fix_height(_get_track_roll(next_track, next_connection->get_endpoint_index()));
                    end_tex_width = next_track_state->tex_width;
                    end_tex_slope = next_track_state->tex_slope;
                }

                const Section start_section = _build_trackbed_section(
                        track_width, track->tex_height + _get_roll_fix_height(curve1_data->get_roll1()),
                        track->tex_width, track->tex_slope, curve1_data->get_roll1(), rail_height,
                        track->resolved_trackbed_texture_length);
                const Section end_section = _build_trackbed_section(
                        end_width, end_tex_height, end_tex_width, end_tex_slope, curve1_data->get_roll2(), rail_height,
                        track->resolved_trackbed_texture_length);
                _append_loft_geometry_range_points(
                        trackbed, curve1_points, start_section, end_section, track->resolved_trackbed_texture_length,
                        rail_height, 0, CURVE_END);

                if (curve2_data.is_valid()) {
                    const Ref<Curve3D> second_curve = _build_curve(curve2_data);
                    const Section second_start_section = _build_trackbed_section(
                            track_width, track->tex_height + _get_roll_fix_height(curve2_data->get_roll1()),
                            track->tex_width, track->tex_slope, curve2_data->get_roll1(), rail_height,
                            track->resolved_trackbed_texture_length);
                    const Section second_end_section = _build_trackbed_section(
                            track_width, track->tex_height + _get_roll_fix_height(curve2_data->get_roll2()),
                            track->tex_width, track->tex_slope, curve2_data->get_roll2(), rail_height,
                            track->resolved_trackbed_texture_length);
                    _append_loft_geometry_range_points(
                            trackbed, second_curve->get_baked_points(), second_start_section, second_end_section,
                            track->resolved_trackbed_texture_length, rail_height, 0, CURVE_END);
                }
                track->meshes[SLOT_TRACKBED] = _build_array_mesh(trackbed);
            }
        }

        for (int slot = 0; slot < SLOT_MAX; slot++) {
            if (track->meshes[slot].is_valid()) {
                _slot_draw(*track, static_cast<Slot>(slot));
            }
        }

        // The second path of a switch carries its own rail texture; everything else, and a switch
        // that declares none, uses the track's own one.
        const Ref<Material> secondary_material =
                curve2_data.is_valid() && track->material2.is_valid() ? track->material2 : track->material1;
        const Ref<Material> slot_materials[SLOT_MAX] = {
                track->material1, secondary_material, track->material1, secondary_material, trackbed_material};
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (int slot = 0; slot < SLOT_MAX; slot++) {
            const Ref<ArrayMesh> &mesh = track->meshes[slot];
            if (mesh.is_null() || slot_materials[slot].is_null()) {
                continue;
            }
            for (int surface_index = 0; surface_index < mesh->get_surface_count(); surface_index++) {
                rendering->mesh_surface_set_material(mesh->get_rid(), surface_index, slot_materials[slot]->get_rid());
            }
        }

        track_set_switch_blade_offsets(p_track_render, geometry.switch_f_offset1, geometry.switch_f_offset2);
    }

    void MaszynaTrackRenderingBackend::track_clear(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        _clear_track_meshes(*track);
        track->material1.unref();
        track->material2.unref();
        track->material_trackbed.unref();
        track->resolved_trackbed_material.unref();
        track->resolved_trackbed_material_valid = false;
    }

    void MaszynaTrackRenderingBackend::topology_rebuilt() {
        for (KeyValue<RID, Track> &item: tracks) {
            item.value.resolved_trackbed_material_valid = false;
        }
        // a road turns into the next one it is joined to
        for (KeyValue<RID, RoadCell> &item: road_cells) {
            _queue_road_cell_rebuild(item.value);
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
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, RID());
        return track->road_cell;
    }

    bool MaszynaTrackRenderingBackend::road_cell_exists(const RID &p_cell) const {
        return road_cells.has(p_cell);
    }

    RID MaszynaTrackRenderingBackend::road_cell_get_instance(const RID &p_cell) const {
        const RoadCell *cell = road_cells.getptr(p_cell);
        ERR_FAIL_NULL_V(cell, RID());
        return cell->instance;
    }

    Ref<ArrayMesh> MaszynaTrackRenderingBackend::road_cell_get_mesh(const RID &p_cell) const {
        const RoadCell *cell = road_cells.getptr(p_cell);
        ERR_FAIL_NULL_V(cell, Ref<ArrayMesh>());
        return cell->mesh;
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

    double MaszynaTrackRenderingBackend::_get_roll_fix_height(const double p_roll_degrees) {
        return Math::abs(Math::sin(Math::deg_to_rad(p_roll_degrees)) * ROLL_FIX_FACTOR);
    }

    Ref<Curve3D> MaszynaTrackRenderingBackend::_build_curve(const Ref<TrackCurve> &p_curve) {
        Ref<Curve3D> curve;
        curve.instantiate();
        curve->set_closed(false);
        curve->set_bake_interval(static_cast<real_t>(mesh_curve_bake_interval(p_curve)));
        curve->add_point(
                p_curve->get_p1() + Vector3(0.0, static_cast<real_t>(_get_roll_fix_height(p_curve->get_roll1())), 0.0),
                Vector3(), p_curve->get_c1());
        curve->add_point(
                p_curve->get_p2() + Vector3(0.0, static_cast<real_t>(_get_roll_fix_height(p_curve->get_roll2())), 0.0),
                p_curve->get_c2(), Vector3());
        curve->set_point_tilt(0, static_cast<real_t>(p_curve->get_roll1()));
        curve->set_point_tilt(1, static_cast<real_t>(p_curve->get_roll2()));
        return curve;
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

    MaszynaTrackRenderingBackend::Section MaszynaTrackRenderingBackend::_build_rail_section(
            const Vector<ProfilePoint> &p_profile, const double p_width, const double p_roll, const bool p_mirrored) {
        Section section;
        const double half_width = 0.5 * Math::abs(p_width);
        // The loft frames here map the profile's x to the left of the track direction, the original's
        // RenderLoft to the right (Segment.cpp:398 parallel): the profile is mirrored, so the cant
        // (Track.cpp:2652) turns the other way
        const double roll_radians = -Math::deg_to_rad(p_roll);
        const double sin_roll = Math::sin(roll_radians);
        const double cos_roll = Math::cos(roll_radians);

        for (const ProfilePoint &point: p_profile) {
            SectionPoint section_point;
            section_point.u = point.u;
            if (p_mirrored) {
                section_point.position =
                        Vector3(static_cast<real_t>(((-half_width - point.x) * cos_roll) + (point.y * sin_roll)),
                                static_cast<real_t>((-(-half_width - point.x) * sin_roll) + (point.y * cos_roll)), 0.0);
                section_point.normal =
                        Vector3(static_cast<real_t>((-point.normal_x * cos_roll) + (point.normal_y * sin_roll)),
                                static_cast<real_t>((point.normal_x * sin_roll) + (point.normal_y * cos_roll)), 0.0)
                                .normalized();
                section.insert(0, section_point);
            } else {
                section_point.position =
                        Vector3(static_cast<real_t>(((half_width + point.x) * cos_roll) + (point.y * sin_roll)),
                                static_cast<real_t>((-(half_width + point.x) * sin_roll) + (point.y * cos_roll)), 0.0);
                section_point.normal =
                        Vector3(static_cast<real_t>((point.normal_x * cos_roll) + (point.normal_y * sin_roll)),
                                static_cast<real_t>((-point.normal_x * sin_roll) + (point.normal_y * cos_roll)), 0.0)
                                .normalized();
                section.push_back(section_point);
            }
        }
        return section;
    }

    // Port of trackbed profile construction from Track.cpp:2825-2982.
    MaszynaTrackRenderingBackend::Section MaszynaTrackRenderingBackend::_build_trackbed_section(
            const double p_width, const double p_tex_height, const double p_tex_width, const double p_tex_slope,
            const double p_roll, const double p_rail_height, const double p_tex_length) {
        const double half_width = 0.5 * Math::abs(p_width);
        const double side = Math::abs(p_tex_width);
        const double slope_abs = Math::abs(p_tex_slope);
        const double spread = half_width + side + slope_abs;
        double hypot_value = Math::sqrt((slope_abs * slope_abs) + (p_tex_height * p_tex_height));
        if (Math::is_zero_approx(hypot_value)) {
            hypot_value = 1.0;
        }

        double normal_x = p_tex_height / hypot_value;
        double normal_y = p_tex_slope / hypot_value;
        if (Math::is_zero_approx(normal_x) && Math::is_zero_approx(normal_y)) {
            normal_x = 1.0;
            normal_y = 0.0;
        }

        // mirrored like the rail profile (_build_rail_section())
        const double roll_radians = -Math::deg_to_rad(p_roll);
        const double sin_roll = Math::sin(roll_radians);
        const double cos_roll = Math::cos(roll_radians);
        // the texture's width is proportional to its length (fTexRatio2 is 1 for rail track,
        // Track.cpp:2930-2932); a 4 m texture keeps the old fixed mapping (Track.cpp:2855-2926)
        const double safe_tex_length = MAX(Math::abs(p_tex_length), MIN_LENGTH);
        double u_inner = 0.5 - ((half_width + side) / safe_tex_length);
        double u_outer = 0.5 - ((half_width + side + hypot_value) / safe_tex_length);
        Vector3 slope_normal = Vector3(static_cast<real_t>(normal_x), static_cast<real_t>(normal_y), 0.0).normalized();
        if (Math::is_equal_approx(p_tex_length, TRACKBED_OLD_MAPPING_TEXTURE_LENGTH)) {
            u_inner = TRACKBED_OLD_MAPPING_BREAK_U;
            u_outer = 0.0;
            slope_normal =
                    Vector3(static_cast<real_t>(Math::cos(Math::deg_to_rad(TRACKBED_OLD_MAPPING_NORMAL_ANGLE))),
                            static_cast<real_t>(Math::sin(Math::deg_to_rad(TRACKBED_OLD_MAPPING_NORMAL_ANGLE))), 0.0);
        }
        const Vector3 mirrored_normal(-slope_normal.x, slope_normal.y, 0.0);

        Section section;
        section.push_back(
                {Vector3(static_cast<real_t>((spread * cos_roll) - (p_tex_height * sin_roll)),
                         static_cast<real_t>((-spread * sin_roll) - (p_tex_height * cos_roll) - p_rail_height), 0.0),
                 slope_normal, u_outer});
        section.push_back(
                {Vector3(static_cast<real_t>((half_width + side) * cos_roll),
                         static_cast<real_t>((-(half_width + side) * sin_roll) - p_rail_height), 0.0),
                 slope_normal, u_inner});
        section.push_back({Vector3(0.0, static_cast<real_t>(-p_rail_height + TRACKBED_CROWN_LIFT), 0.0), UP, 0.5});
        section.push_back(
                {Vector3(static_cast<real_t>(-(half_width + side) * cos_roll),
                         static_cast<real_t>(((half_width + side) * sin_roll) - p_rail_height), 0.0),
                 mirrored_normal, 1.0 - u_inner});
        section.push_back(
                {Vector3(static_cast<real_t>((-spread * cos_roll) - (p_tex_height * sin_roll)),
                         static_cast<real_t>((spread * sin_roll) - (p_tex_height * cos_roll) - p_rail_height), 0.0),
                 mirrored_normal, 1.0 - u_outer});
        return section;
    }

    MaszynaTrackRenderingBackend::BladeSections MaszynaTrackRenderingBackend::_get_switch_blade_sections(
            const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_regular_profile,
            const Vector<ProfilePoint> &p_blade_profile, const double p_width, const double p_roll_start,
            const double p_roll_end, const bool p_blade_mirrored, const int p_blade_sample_count) {
        BladeSections sections;
        sections.transition_sample_count = MAX(p_blade_sample_count / 2, 1);
        double total_dist = _get_baked_segment_length(p_sampled_points, 0, CURVE_END);
        if (total_dist <= 0.0) {
            total_dist = MIN_LENGTH;
        }

        const double d_trans = _get_baked_segment_length(p_sampled_points, 0, sections.transition_sample_count);
        const double d_blade = _get_baked_segment_length(p_sampled_points, 0, p_blade_sample_count);
        const double roll_trans = Math::lerp(p_roll_start, p_roll_end, d_trans / total_dist);
        const double roll_blade = Math::lerp(p_roll_start, p_roll_end, d_blade / total_dist);

        sections.blade_start = _build_rail_section(p_blade_profile, p_width, p_roll_start, p_blade_mirrored);
        sections.reg_trans = _build_rail_section(p_regular_profile, p_width, roll_trans, p_blade_mirrored);
        sections.reg_blade = _build_rail_section(p_regular_profile, p_width, roll_blade, p_blade_mirrored);
        sections.reg_end = _build_rail_section(p_regular_profile, p_width, p_roll_end, p_blade_mirrored);
        return sections;
    }

    void MaszynaTrackRenderingBackend::_append_loft_geometry_range_points(
            Geometry &p_geometry, const PackedVector3Array &p_sampled_points, const Section &p_start_section,
            const Section &p_end_section, const double p_tex_length, const double p_rail_height,
            const int p_start_sample, const int p_end_sample) {
        if (p_start_section.size() < 2 || p_start_section.size() != p_end_section.size()) {
            return;
        }
        if (p_sampled_points.size() < 2) {
            return;
        }

        const int last_index = static_cast<int>(p_sampled_points.size()) - 1;
        const int first_sample = CLAMP(p_start_sample, 0, last_index);
        const int last_sample = p_end_sample < 0 ? last_index : CLAMP(p_end_sample, 0, last_index);
        if (last_sample <= first_sample) {
            return;
        }

        const double safe_tex_length = MAX(Math::abs(p_tex_length), MIN_LENGTH);
        const int section_size = static_cast<int>(p_start_section.size());
        const int base_vertex = static_cast<int>(p_geometry.vertices.size());
        double distance = 0.0;
        const double total_length =
                MAX(_get_baked_segment_length(p_sampled_points, first_sample, last_sample), MIN_LENGTH);

        for (int sample_index = first_sample; sample_index <= last_sample; sample_index++) {
            const double t = distance / total_length;
            Transform3D frame = _build_curve_frame(p_sampled_points, sample_index);
            frame.origin.y += static_cast<real_t>(p_rail_height);

            for (int point_index = 0; point_index < section_size; point_index++) {
                const SectionPoint &start_point = p_start_section[point_index];
                const SectionPoint &end_point = p_end_section[point_index];

                const Vector3 local_position = start_point.position.lerp(end_point.position, static_cast<real_t>(t));
                const Vector3 local_normal =
                        start_point.normal.lerp(end_point.normal, static_cast<real_t>(t)).normalized();
                const double uv_x = Math::lerp(start_point.u, end_point.u, t);

                p_geometry.vertices.push_back(frame.xform(local_position));
                p_geometry.normals.push_back(frame.basis.xform(local_normal).normalized());
                p_geometry.uvs.push_back(
                        Vector2(static_cast<real_t>(uv_x), static_cast<real_t>(distance / safe_tex_length)));
            }

            if (sample_index < last_sample) {
                distance += p_sampled_points[sample_index].distance_to(p_sampled_points[sample_index + 1]);
            }
        }

        _append_loft_strip_indices(p_geometry.indices, last_sample - first_sample + 1, section_size, base_vertex);
    }

    void MaszynaTrackRenderingBackend::_append_loft_strip_indices(
            PackedInt32Array &p_indices, const int p_step_count, const int p_section_size, const int p_base_vertex) {
        for (int step = 0; step < p_step_count - 1; step++) {
            const int current_base = p_base_vertex + (step * p_section_size);
            const int next_base = p_base_vertex + ((step + 1) * p_section_size);
            for (int point_index = 0; point_index < p_section_size - 1; point_index++) {
                const int current = current_base + point_index;
                const int current_next = current_base + point_index + 1;
                const int next_current = next_base + point_index;
                const int next_next = next_base + point_index + 1;

                p_indices.push_back(current);
                p_indices.push_back(next_current);
                p_indices.push_back(current_next);
                p_indices.push_back(current_next);
                p_indices.push_back(next_current);
                p_indices.push_back(next_next);
            }
        }
    }

    Vector3 MaszynaTrackRenderingBackend::_get_section_world_center(
            const PackedVector3Array &p_sampled_points, const int p_sample_index, const Section &p_section,
            const double p_rail_height) {
        if (p_sampled_points.is_empty()) {
            return {};
        }
        const int safe_sample_index = CLAMP(p_sample_index, 0, static_cast<int>(p_sampled_points.size()) - 1);
        Transform3D frame = _build_curve_frame(p_sampled_points, safe_sample_index);
        frame.origin.y += static_cast<real_t>(p_rail_height);
        Vector3 center;
        if (!p_section.is_empty()) {
            for (const SectionPoint &point: p_section) {
                center += point.position;
            }
            center /= static_cast<real_t>(p_section.size());
        }
        return frame.xform(center);
    }

    Vector3 MaszynaTrackRenderingBackend::_get_switch_blade_pivot(
            const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_regular_profile,
            const double p_width, const double p_roll_start, const double p_roll_end, const bool p_blade_mirrored,
            const int p_blade_sample_count, const double p_rail_height) {
        const BladeSections sections = _get_switch_blade_sections(
                p_sampled_points, p_regular_profile, p_regular_profile, p_width, p_roll_start, p_roll_end,
                p_blade_mirrored, p_blade_sample_count);
        return _get_section_world_center(p_sampled_points, p_blade_sample_count, sections.reg_blade, p_rail_height);
    }

    double MaszynaTrackRenderingBackend::_get_blade_angle_factor(
            const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_blade_profile,
            const double p_width, const double p_roll_start, const bool p_blade_mirrored, const double p_rail_height,
            const Vector3 &p_move_direction, const Vector3 &p_pivot) {
        if (p_sampled_points.is_empty()) {
            return 0.0;
        }
        const Section blade_start_section =
                _build_rail_section(p_blade_profile, p_width, p_roll_start, p_blade_mirrored);
        const Vector3 blade_start = _get_section_world_center(p_sampled_points, 0, blade_start_section, p_rail_height);
        const Vector3 per_radian_motion = UP.cross(blade_start - p_pivot);
        return p_move_direction.dot(per_radian_motion);
    }

    Transform3D MaszynaTrackRenderingBackend::_build_blade_transform(
            const Vector3 &p_pivot, const double p_angle_factor, const double p_offset) {
        if (Math::is_zero_approx(p_angle_factor)) {
            return {};
        }
        const double angle = p_offset / p_angle_factor;
        return Transform3D(Basis(UP, static_cast<real_t>(angle)), p_pivot) * Transform3D(Basis(), -p_pivot);
    }

    // Fixed rail parts are generated once; moving blades are separate mesh instances.
    // MaSzyna Track.cpp:1372-1443 separates blades from fixed rails.
    void MaszynaTrackRenderingBackend::_append_switch_fixed_rail_geometry(
            Geometry &p_geometry, const PackedVector3Array &p_sampled_points,
            const Vector<ProfilePoint> &p_regular_profile, const double p_width, const double p_roll_start,
            const double p_roll_end, const double p_tex_length, const double p_rail_height, const bool p_other_mirrored,
            const bool p_blade_mirrored, const int p_blade_sample_count) {
        const Section other_start = _build_rail_section(p_regular_profile, p_width, p_roll_start, p_other_mirrored);
        const Section other_end = _build_rail_section(p_regular_profile, p_width, p_roll_end, p_other_mirrored);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, other_start, other_end, p_tex_length, p_rail_height, 0, CURVE_END);

        const BladeSections sections = _get_switch_blade_sections(
                p_sampled_points, p_regular_profile, p_regular_profile, p_width, p_roll_start, p_roll_end,
                p_blade_mirrored, p_blade_sample_count);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, sections.reg_blade, sections.reg_end, p_tex_length, p_rail_height,
                p_blade_sample_count, CURVE_END);
    }

    // The blade mesh is built at zero offset; domain fOffsetX is applied as instance transform.
    // MaSzyna Track.cpp:1975-2007 updates blade geometry only.
    // MaSzyna Segment.cpp:376-524 applies fOffsetX laterally.
    void MaszynaTrackRenderingBackend::_append_switch_blade_geometry(
            Geometry &p_geometry, const PackedVector3Array &p_sampled_points,
            const Vector<ProfilePoint> &p_regular_profile, const Vector<ProfilePoint> &p_blade_profile,
            const double p_width, const double p_roll_start, const double p_roll_end, const double p_tex_length,
            const double p_rail_height, const bool p_blade_mirrored, const int p_blade_sample_count) {
        const BladeSections sections = _get_switch_blade_sections(
                p_sampled_points, p_regular_profile, p_blade_profile, p_width, p_roll_start, p_roll_end,
                p_blade_mirrored, p_blade_sample_count);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, sections.blade_start, sections.reg_trans, p_tex_length, p_rail_height, 0,
                sections.transition_sample_count);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, sections.reg_trans, sections.reg_blade, p_tex_length, p_rail_height,
                sections.transition_sample_count, p_blade_sample_count);
    }

    // Port of the transition branch of TSegment::RenderLoft from Segment.cpp:376-524.
    Vector<MaszynaTrackRenderingBackend::StripChunk> MaszynaTrackRenderingBackend::_build_transition_loft_strip_chunks(
            const Ref<Curve3D> &p_curve, const Section &p_start_section, const Section &p_end_section,
            const double p_tex_length, const double p_rail_height) {
        Vector<StripChunk> chunks;
        if (p_start_section.size() < TRACKBED_SECTION_POINT_COUNT || p_start_section.size() != p_end_section.size()) {
            return chunks;
        }

        // Port of fixed switch segmentation from Segment.cpp:128-140.
        const PackedVector3Array sampled_points =
                p_curve->get_baked_length() > 0.0
                        ? _sample_curve_points_range(p_curve, 0.0, p_curve->get_baked_length(), SWITCH_SEGMENT_COUNT)
                        : PackedVector3Array();
        if (sampled_points.size() < 2) {
            return chunks;
        }

        const double safe_tex_length = MAX(Math::abs(p_tex_length), MIN_LENGTH);
        const int segment_count = static_cast<int>(sampled_points.size()) - 1;
        double tv1 = TEXTURE_V_RANGE;
        const Vector3 lift(0.0, static_cast<real_t>(p_rail_height), 0.0);

        for (int segment_index = 0; segment_index < segment_count; segment_index++) {
            // Port note for Segment.cpp:398-399 and Track.cpp:644-646 / 708-710:
            // original switch segments used by RenderLoft are already lifted by railheight.
            const Vector3 pos1 = sampled_points[segment_index] + lift;
            const Vector3 pos2 = sampled_points[segment_index + 1] + lift;
            // to the left, as every loft here maps the profile's x (_build_rail_section())
            const Vector3 parallel1 = -_build_planar_parallel(sampled_points, segment_index);
            const Vector3 parallel2 = -_build_planar_parallel(sampled_points, segment_index + 1);
            const double m1 = static_cast<double>(segment_index) / static_cast<double>(segment_count);
            const double m2 = static_cast<double>(segment_index + 1) / static_cast<double>(segment_count);
            const double jmm1 = 1.0 - m1;
            const double jmm2 = 1.0 - m2;
            const double segment_length = pos1.distance_to(pos2);
            // clamp_circular() of utilities.h:277
            tv1 = Math::fmod(tv1, TEXTURE_V_RANGE);
            if (tv1 < 0.0) {
                tv1 += TEXTURE_V_RANGE;
            }
            const double tv2 = tv1 - (segment_length / safe_tex_length);
            StripChunk chunk;

            // Port of per-segment strip vertex generation from Segment.cpp:452-492.
            for (int point_index = 0; point_index < TRACKBED_SECTION_POINT_COUNT; point_index++) {
                const SectionPoint &start_point = p_start_section[point_index];
                const SectionPoint &end_point = p_end_section[point_index];
                const double start_x = (jmm1 * start_point.position.x) + (m1 * end_point.position.x);
                const double start_y = (jmm1 * start_point.position.y) + (m1 * end_point.position.y);
                const double end_x = (jmm2 * start_point.position.x) + (m2 * end_point.position.x);
                const double end_y = (jmm2 * start_point.position.y) + (m2 * end_point.position.y);
                const double start_normal_x = (jmm1 * start_point.normal.x) + (m1 * end_point.normal.x);
                const double start_normal_y = (jmm1 * start_point.normal.y) + (m1 * end_point.normal.y);
                const double end_normal_x = (jmm2 * start_point.normal.x) + (m2 * end_point.normal.x);
                const double end_normal_y = (jmm2 * start_point.normal.y) + (m2 * end_point.normal.y);
                const double uv_x_start = (jmm1 * start_point.u) + (m1 * end_point.u);
                const double uv_x_end = (jmm2 * start_point.u) + (m2 * end_point.u);

                chunk.push_back(
                        {pos1 + parallel1 * static_cast<real_t>(start_x) +
                                 Vector3(0.0, static_cast<real_t>(start_y), 0.0),
                         (parallel1 * static_cast<real_t>(start_normal_x) +
                          Vector3(0.0, static_cast<real_t>(start_normal_y), 0.0))
                                 .normalized(),
                         Vector2(static_cast<real_t>(uv_x_start), static_cast<real_t>(tv1))});
                chunk.push_back(
                        {pos2 + parallel2 * static_cast<real_t>(end_x) + Vector3(0.0, static_cast<real_t>(end_y), 0.0),
                         (parallel2 * static_cast<real_t>(end_normal_x) +
                          Vector3(0.0, static_cast<real_t>(end_normal_y), 0.0))
                                 .normalized(),
                         Vector2(static_cast<real_t>(uv_x_end), static_cast<real_t>(tv2))});
            }

            chunks.push_back(chunk);
            tv1 = tv2;
        }
        return chunks;
    }

    double MaszynaTrackRenderingBackend::_find_switch_frog_distance(
            const Ref<Curve3D> &p_curve1, const Ref<Curve3D> &p_curve2, const double p_width) {
        const double length = MIN(p_curve1->get_baked_length(), p_curve2->get_baked_length());
        double dist = 0.0;
        while (dist < length) {
            const Vector3 p1 = p_curve1->sample_baked(static_cast<real_t>(dist));
            const Vector3 p2 = p_curve2->sample_baked(static_cast<real_t>(dist));
            if (p1.distance_to(p2) >= p_width) {
                return dist;
            }
            dist += FROG_SEARCH_STEP;
        }
        return length;
    }

    PackedVector3Array MaszynaTrackRenderingBackend::_sample_curve_points_range(
            const Ref<Curve3D> &p_curve, const double p_start_dist, const double p_end_dist,
            const int p_segment_count) {
        PackedVector3Array points;
        const double length = p_end_dist - p_start_dist;
        if (length <= 0.0) {
            return points;
        }
        for (int i = 0; i <= p_segment_count; i++) {
            const double d = p_start_dist + (length * static_cast<double>(i) / static_cast<double>(p_segment_count));
            points.push_back(p_curve->sample_baked(static_cast<real_t>(d), true));
        }
        return points;
    }

    PackedVector3Array
    MaszynaTrackRenderingBackend::_sample_switch_curve_points(const Ref<Curve3D> &p_curve, const double p_frog_dist) {
        PackedVector3Array points = _sample_curve_points_range(p_curve, 0.0, p_frog_dist, SWITCH_SEGMENT_COUNT);
        const double total_length = p_curve->get_baked_length();
        if (total_length > p_frog_dist + SWITCH_TAIL_MIN_LENGTH) {
            const int rem_segments =
                    MAX(1, static_cast<int>((total_length - p_frog_dist) / SWITCH_TAIL_SEGMENT_LENGTH));
            const PackedVector3Array rem_points =
                    _sample_curve_points_range(p_curve, p_frog_dist, total_length, rem_segments);
            for (int i = 1; i < rem_points.size(); i++) {
                points.push_back(rem_points[i]);
            }
        }
        return points;
    }

    // Port of planar cross-track vector construction from Segment.cpp:398-405 and Segment.cpp:444-450.
    Vector3
    MaszynaTrackRenderingBackend::_build_planar_parallel(const PackedVector3Array &p_points, const int p_sample_index) {
        const int last = static_cast<int>(p_points.size()) - 1;
        Vector3 tangent;
        if (p_sample_index <= 0) {
            tangent = p_points[1] - p_points[0];
        } else if (p_sample_index >= last) {
            tangent = p_points[last] - p_points[last - 1];
        } else {
            tangent = p_points[p_sample_index + 1] - p_points[p_sample_index - 1];
        }

        Vector3 parallel(-tangent.z, 0.0, tangent.x);
        if (parallel.length_squared() <= MIN_DIRECTION_LENGTH_SQUARED) {
            parallel = p_points[last] - p_points[0];
            parallel = Vector3(-parallel.z, 0.0, parallel.x);
        }
        if (parallel.length_squared() <= MIN_DIRECTION_LENGTH_SQUARED) {
            return RIGHT;
        }
        return parallel.normalized();
    }

    double MaszynaTrackRenderingBackend::_get_baked_segment_length(
            const PackedVector3Array &p_points, const int p_start_sample, const int p_end_sample) {
        double length = 0.0;
        const int last = p_end_sample < 0 ? static_cast<int>(p_points.size()) - 1 : p_end_sample;
        for (int sample_index = p_start_sample; sample_index < last; sample_index++) {
            length += p_points[sample_index].distance_to(p_points[sample_index + 1]);
        }
        return length;
    }

    Transform3D
    MaszynaTrackRenderingBackend::_build_curve_frame(const PackedVector3Array &p_points, const int p_sample_index) {
        const Vector3 origin = p_points[p_sample_index];
        Vector3 tangent;
        if (p_sample_index == 0) {
            tangent = p_points[1] - p_points[0];
        } else if (p_sample_index == p_points.size() - 1) {
            tangent = p_points[p_sample_index] - p_points[p_sample_index - 1];
        } else {
            tangent = p_points[p_sample_index + 1] - p_points[p_sample_index - 1];
        }

        if (tangent.length_squared() <= MIN_DIRECTION_LENGTH_SQUARED) {
            tangent = FORWARD;
        } else {
            tangent = tangent.normalized();
        }

        Vector3 reference_up = UP;
        if (Math::abs(tangent.dot(reference_up)) > VERTICAL_TANGENT_DOT) {
            reference_up = RIGHT;
        }

        const Vector3 right = reference_up.cross(tangent).normalized();
        const Vector3 up = tangent.cross(right).normalized();
        return {Basis(right, up, tangent).orthonormalized(), origin};
    }

    Ref<ArrayMesh> MaszynaTrackRenderingBackend::_build_array_mesh(const Geometry &p_geometry) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = p_geometry.vertices;
        arrays[Mesh::ARRAY_NORMAL] = p_geometry.normals;
        arrays[Mesh::ARRAY_TEX_UV] = p_geometry.uvs;
        arrays[Mesh::ARRAY_INDEX] = p_geometry.indices;

        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
        // regen_normal_maps() computes tangents for normal-mapped rail/ballast textures (the arrays
        // above never set ARRAY_TANGENT), but costs ~7-9ms PER CALL almost independent of mesh size -
        // for a real scenery with hundreds of track segments (2-3 calls each: rail, trackbed, and
        // switch secondary rail), that alone was ~2.1-2.3s of a ~3s scenery load, confirmed by
        // profiling. Skipped here; the tradeoff is flatter-looking (untangented) normal-map shading
        // on rails/ballast specifically, not missing geometry or broken base-color/roughness texturing.
        return mesh;
    }

    /// Port of TTrack::create_road_profile() (Track.cpp:3008-3067) at one end of the road: its left
    /// and right edges in the original's frame (x to the right of the road's direction), the texture
    /// cut to the road's width - a texture is as wide as its length times its width/height ratio
    MaszynaTrackRenderingBackend::Section MaszynaTrackRenderingBackend::_build_road_section(
            const double p_width, const double p_roll, const double p_texture_length, const double p_ratio) {
        const double half_width = 0.5 * Math::abs(p_width);
        const double texture_width = p_ratio * p_texture_length;
        const double map = texture_width > 0.0 ? half_width / texture_width : ROAD_DEFAULT_MAP;
        const Vector3 left(
                static_cast<real_t>(half_width * Math::cos(p_roll)),
                static_cast<real_t>(-half_width * Math::sin(p_roll)), 0.0);
        Section section;
        section.push_back({left, UP, 0.5 - map});
        section.push_back({-left, UP, 0.5 + map});
        return section;
    }

    /// Port of TTrack::create_road_side_profile() (Track.cpp:3069-3256) at one end of the road: right
    /// and left, each a section in the original's frame. A non-negative texture height is a shoulder:
    /// flat for tex_width, then down to -tex_height over tex_slope. A negative one is a sidewalk: a curb
    /// as high, its face sloped 1:3.75, then a sidewalk tex_slope wide on the right and tex_width wide
    /// on the left, mapped in proportion to its width.
    void MaszynaTrackRenderingBackend::_build_road_side_sections(
            const RoadLook &p_road, const Section &p_surface, const double p_texture_length, const double p_ratio,
            Section &p_r_right, Section &p_r_left) {
        const double half_width = 0.5 * Math::abs(p_road.width);
        const double side = Math::abs(p_road.tex_width);
        const double slope = Math::abs(p_road.tex_slope);
        const Vector3 road_left = p_surface[0].position;
        const Vector3 road_right = p_surface[1].position;
        p_r_right.clear();
        p_r_left.clear();
        if (p_road.tex_height >= 0.0) {
            const double outer = half_width + side + slope;
            p_r_right.push_back(
                    {Vector3(static_cast<real_t>(outer), static_cast<real_t>(-p_road.tex_height), 0.0), RIGHT, 0.0});
            p_r_right.push_back({Vector3(road_left.x + static_cast<real_t>(side), road_left.y, 0.0), UP, 0.5});
            p_r_right.push_back({road_left, UP, 1.0});
            p_r_left.push_back({road_right, UP, 1.0});
            p_r_left.push_back({Vector3(road_right.x - static_cast<real_t>(side), road_right.y, 0.0), UP, 0.5});
            p_r_left.push_back(
                    {Vector3(static_cast<real_t>(-outer), static_cast<real_t>(-p_road.tex_height), 0.0), LEFT, 0.0});
            return;
        }
        // a curb 150 mm high leans 40 mm; the curbs are mapped from 31/64 to 32/64 and 32/64 to 33/64
        const double lean = -p_road.tex_height / CURB_SLOPE;
        const double texture_width = p_ratio * p_texture_length;
        const double map_left = texture_width > 0.0 ? side / texture_width : CURB_LEFT_U;
        const double map_right = texture_width > 0.0 ? slope / texture_width : CURB_LEFT_U;
        const double height_right = slope > lean ? -p_road.tex_height : 0.0;
        const double height_left = side > lean ? -p_road.tex_height : 0.0;
        p_r_right.push_back(
                {Vector3(road_left.x + static_cast<real_t>(slope), road_left.y + static_cast<real_t>(height_right),
                         0.0),
                 UP, CURB_RIGHT_U + map_right});
        p_r_right.push_back(
                {Vector3(road_left.x + static_cast<real_t>(lean), road_left.y + static_cast<real_t>(height_right), 0.0),
                 UP, CURB_RIGHT_U});
        p_r_right.push_back({road_left, LEFT, CURB_RIGHT_U - (lean / CURB_LEAN_PER_U)});
        p_r_left.push_back({road_right, RIGHT, CURB_LEFT_U + (lean / CURB_LEAN_PER_U)});
        p_r_left.push_back(
                {Vector3(road_right.x - static_cast<real_t>(lean), road_right.y + static_cast<real_t>(height_left),
                         0.0),
                 UP, CURB_LEFT_U});
        p_r_left.push_back(
                {Vector3(road_right.x - static_cast<real_t>(side), road_right.y + static_cast<real_t>(height_left),
                         0.0),
                 UP, CURB_LEFT_U - map_left});
    }

    /// A section of the original's frame (x to the right of the road's direction) in this backend's
    /// loft frame, which has x to the left (_build_rail_section()): mirrored, and reversed so its faces
    /// still face up
    MaszynaTrackRenderingBackend::Section MaszynaTrackRenderingBackend::_mirror_road_section(const Section &p_section) {
        Section mirrored;
        for (const SectionPoint &point: p_section) {
            mirrored.insert(
                    0, {Vector3(-point.position.x, point.position.y, 0.0),
                        Vector3(-point.normal.x, point.normal.y, 0.0), point.u});
        }
        return mirrored;
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

    /// The track's materials by the names it has, as it is built - a streamed track holds them, and
    /// their textures, only while it is built
    void MaszynaTrackRenderingBackend::_resolve_materials(Track &p_track) const {
        ReloadedMaterialServer *materials = ReloadedMaterialServer::get_instance();
        ERR_FAIL_NULL(materials);
        // An unnamed slot has no material at all - ReloadedMaterialServer would hand back the
        // missing-texture placeholder instead (Track.cpp:485-491 keeps a null handle here).
        p_track.material1 = p_track.material1_name.is_empty()
                                    ? Ref<Material>()
                                    : materials->material_get(p_track.material1_name, track_material_options);
        p_track.material2 = p_track.material2_name.is_empty()
                                    ? Ref<Material>()
                                    : materials->material_get(p_track.material2_name, track_material_options);
        p_track.material_trackbed =
                p_track.material_trackbed_name.is_empty()
                        ? Ref<Material>()
                        : materials->material_get(p_track.material_trackbed_name, track_material_options);
        p_track.resolved_trackbed_material_valid = false;
    }

    Ref<Material> MaszynaTrackRenderingBackend::_resolve_trackbed_material(Track &p_track) {
        if (!p_track.resolved_trackbed_material_valid) {
            RIDSet visited;
            const Track *owner = _copy_adjacent_trackbed_material(&p_track, visited);
            String material_name;
            p_track.resolved_trackbed_material.unref();
            if (owner != nullptr) {
                material_name = owner->material_trackbed_name.is_empty() ? owner->material2_name
                                                                         : owner->material_trackbed_name;
                // the neighbour's own material, which it holds only while it is built itself
                ReloadedMaterialServer *materials = ReloadedMaterialServer::get_instance();
                ERR_FAIL_NULL_V(materials, Ref<Material>());
                p_track.resolved_trackbed_material = materials->material_get(material_name, track_material_options);
            }
            // TTrack::texture_length() (Track.cpp:2481-2493): the material's size, else the track's own
            const double size_length = material_name.is_empty() ? -1.0 : _load_texture_length(material_name);
            p_track.resolved_trackbed_texture_length = size_length >= 0.0 ? size_length : p_track.tex_length;
            p_track.resolved_trackbed_material_valid = true;
        }
        return p_track.resolved_trackbed_material;
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
            const double p_start_roll, const double p_end_roll, const double p_rail_height, Section &p_r_start,
            Section &p_r_end) {
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

        const double start_tex_height = source_state->tex_height + _get_roll_fix_height(p_start_roll);
        const double start_tex_width = source_state->tex_width;
        const double start_tex_slope = source_state->tex_slope;
        double end_width = p_geometry.width;
        double end_tex_height = start_tex_height;
        double end_tex_width = start_tex_width;
        double end_tex_slope = start_tex_slope;

        if (next_track.is_valid() && _has_trackbed_profile_options(next_state) && next_track.curve2.is_null()) {
            end_width = next_track.width;
            end_tex_height = next_state->tex_height +
                             _get_roll_fix_height(_get_track_roll(next_track, neighbors->get_next_endpoint_index()));
            end_tex_width = next_state->tex_width;
            end_tex_slope = next_state->tex_slope;
        }

        p_r_start = _build_trackbed_section(
                p_geometry.width, start_tex_height, start_tex_width, start_tex_slope, p_start_roll, p_rail_height,
                p_track.resolved_trackbed_texture_length);
        p_r_end = _build_trackbed_section(
                end_width, end_tex_height, end_tex_width, end_tex_slope, p_end_roll, p_rail_height,
                p_track.resolved_trackbed_texture_length);
    }

    // Port of switch trackbed generation and assembly from Track.cpp:3236-3302.
    Ref<ArrayMesh> MaszynaTrackRenderingBackend::_build_switch_trackbed_mesh(
            const Track &p_track, const TrackGeometry &p_geometry, const Ref<Curve3D> &p_primary_curve,
            const Ref<Curve3D> &p_secondary_curve, const double p_rail_height, const bool p_is_right_switch) {
        Geometry trackbed;
        Section primary_start;
        Section primary_end;
        Section secondary_start;
        Section secondary_end;
        _build_switch_trackbed_sections(
                p_track, p_geometry, TrackServer::TRACK_COMMON, p_geometry.curve1->get_roll1(),
                p_geometry.curve1->get_roll2(), p_rail_height, primary_start, primary_end);
        _build_switch_trackbed_sections(
                p_track, p_geometry, TrackServer::TRACK_DIVERGING, p_geometry.curve2->get_roll1(),
                p_geometry.curve2->get_roll2(), p_rail_height, secondary_start, secondary_end);

        // Port of per-branch loft generation from Track.cpp:3250-3259.
        const Vector<StripChunk> primary_chunks = _build_transition_loft_strip_chunks(
                p_primary_curve, primary_start, primary_end, p_track.resolved_trackbed_texture_length, p_rail_height);
        const Vector<StripChunk> secondary_chunks = _build_transition_loft_strip_chunks(
                p_secondary_curve, secondary_start, secondary_end, p_track.resolved_trackbed_texture_length,
                p_rail_height);

        const int segment_count = static_cast<int>(MIN(primary_chunks.size(), secondary_chunks.size()));

        for (int segment_index = 0; segment_index < segment_count; segment_index++) {
            StripChunk primary_chunk = primary_chunks[segment_index];
            StripChunk secondary_chunk = secondary_chunks[segment_index];

            // Port of samplersoffset-based edge lowering from Track.cpp:3265-3299.
            // The left edge is the profile's +x edge, to the left of the track
            // (_build_transition_loft_strip_chunks())
            const int primary_fallback_edge = p_is_right_switch ? TRACKBED_RIGHT_EDGE_POINT : TRACKBED_LEFT_EDGE_POINT;
            const int secondary_fallback_edge =
                    p_is_right_switch ? TRACKBED_LEFT_EDGE_POINT : TRACKBED_RIGHT_EDGE_POINT;
            const int primary_lower_edge =
                    _nearest_strip_inner_edge(primary_chunk, secondary_chunk, primary_fallback_edge);
            const int secondary_lower_edge =
                    _nearest_strip_inner_edge(secondary_chunk, primary_chunk, secondary_fallback_edge);
            _lower_strip_edge(primary_chunk, primary_lower_edge);
            _lower_strip_edge(secondary_chunk, secondary_lower_edge);

            // Port of the per-branch output order from Track.cpp:3280-3297.
            // Segment.cpp:452-492 emits one 10-vertex strip chunk at a time; only that chunk
            // describes loft faces. Triangulating across chunk boundaries exposes strip
            // connector triangles as visible geometry in Godot.
            _append_strip_chunk(trackbed, primary_chunk);
            _append_strip_chunk(trackbed, secondary_chunk);
        }

        return _build_array_mesh(trackbed);
    }

    // Port of vertex lowering index selection used during switch trackbed assembly from
    // Track.cpp:3271-3293: both vertices of the section point p_edge_point go down.
    void MaszynaTrackRenderingBackend::_lower_strip_edge(StripChunk &p_chunk, const int p_edge_point) {
        for (int vertex = 0; vertex < STRIP_VERTICES_PER_POINT; vertex++) {
            const int vertex_index = (p_edge_point * STRIP_VERTICES_PER_POINT) + vertex;
            if (vertex_index < 0 || vertex_index >= p_chunk.size()) {
                continue;
            }
            p_chunk.write[vertex_index].position.y -= static_cast<real_t>(SWITCH_TRACKBED_Z_FIGHT_OFFSET);
        }
    }

    /// The edge of p_chunk (a section point of its upper slope edges) nearer to the middle of
    /// p_other_chunk, across the ground; p_fallback_edge when the two are as near
    int MaszynaTrackRenderingBackend::_nearest_strip_inner_edge(
            const StripChunk &p_chunk, const StripChunk &p_other_chunk, const int p_fallback_edge) {
        constexpr int RIGHT_EDGE_END = (TRACKBED_RIGHT_EDGE_POINT + 1) * STRIP_VERTICES_PER_POINT;
        constexpr int CENTRE_END = (TRACKBED_CENTRE_POINT + 1) * STRIP_VERTICES_PER_POINT;
        if (p_chunk.size() < RIGHT_EDGE_END || p_other_chunk.size() < CENTRE_END) {
            return p_fallback_edge;
        }
        const auto edge_center = [](const StripChunk &p_strip, const int p_point) {
            const int first = p_point * STRIP_VERTICES_PER_POINT;
            return (p_strip[first].position + p_strip[first + 1].position) * 0.5;
        };
        const Vector3 other_center = edge_center(p_other_chunk, TRACKBED_CENTRE_POINT);
        Vector3 left_delta = edge_center(p_chunk, TRACKBED_LEFT_EDGE_POINT) - other_center;
        Vector3 right_delta = edge_center(p_chunk, TRACKBED_RIGHT_EDGE_POINT) - other_center;
        left_delta.y = 0.0;
        right_delta.y = 0.0;

        const double left_distance = left_delta.length_squared();
        const double right_distance = right_delta.length_squared();
        if (Math::is_equal_approx(left_distance, right_distance)) {
            return p_fallback_edge;
        }
        return left_distance < right_distance ? TRACKBED_LEFT_EDGE_POINT : TRACKBED_RIGHT_EDGE_POINT;
    }

    // Port of strip vertex copying from Track.cpp:3280-3297, and of the visible strip faces emitted per
    // loft chunk by Segment.cpp:452-492. Wound like _append_loft_strip_indices(): the chunk maps the
    // profile's x to the left, so the original's order faces up here without reversing it.
    void MaszynaTrackRenderingBackend::_append_strip_chunk(Geometry &p_geometry, const StripChunk &p_chunk) {
        const int base_vertex = static_cast<int>(p_geometry.vertices.size());
        for (const StripPoint &point: p_chunk) {
            p_geometry.vertices.push_back(point.position);
            p_geometry.normals.push_back(point.normal);
            p_geometry.uvs.push_back(point.uv);
        }
        for (int point_index = 0; point_index < p_chunk.size() - 2; point_index++) {
            const int a = base_vertex + point_index;
            const int b = base_vertex + point_index + 1;
            const int c = base_vertex + point_index + 2;
            if (point_index % 2 == 0) {
                p_geometry.indices.push_back(a);
                p_geometry.indices.push_back(b);
            } else {
                p_geometry.indices.push_back(b);
                p_geometry.indices.push_back(a);
            }
            p_geometry.indices.push_back(c);
        }
    }

    void MaszynaTrackRenderingBackend::_build_road(
            Track &p_track, const TrackGeometry &p_geometry, const Ref<Curve3D> &p_curve) {
        Geometry surface;
        Geometry sides;
        _append_road_geometry(p_track, p_geometry, p_curve, surface, sides);
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (p_track.material1.is_valid() && !surface.vertices.is_empty()) {
            p_track.meshes[SLOT_PRIMARY_RAIL] = _build_array_mesh(surface);
            _slot_draw(p_track, SLOT_PRIMARY_RAIL);
            rendering->mesh_surface_set_material(
                    p_track.meshes[SLOT_PRIMARY_RAIL]->get_rid(), 0, p_track.material1->get_rid());
        }
        if (p_track.material2.is_valid() && !sides.vertices.is_empty()) {
            p_track.meshes[SLOT_TRACKBED] = _build_array_mesh(sides);
            _slot_draw(p_track, SLOT_TRACKBED);
            rendering->mesh_surface_set_material(
                    p_track.meshes[SLOT_TRACKBED]->get_rid(), 0, p_track.material2->get_rid());
        }
    }

    /// Port of the road drawing of TTrack::create_geometry() (Track.cpp:1479-1517): the surface of
    /// material1 across the track's width into p_surface, then the shoulders - or the curbs and
    /// sidewalks - of material2 on both sides into p_sides; a slot without a material name gets
    /// nothing. A road has no rails: its points are its surface (Track.cpp:552-559 lifts rail tracks
    /// only).
    void MaszynaTrackRenderingBackend::_append_road_geometry(
            const Track &p_track, const TrackGeometry &p_geometry, const Ref<Curve3D> &p_curve, Geometry &p_surface,
            Geometry &p_sides) {
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
        const Section surface_start = _build_road_section(road.width, roll1, surface_length, surface_ratio);
        const Section surface_end =
                _build_road_section(next_road.width, transition ? roll2 : roll1, surface_length, surface_ratio);
        const PackedVector3Array points = p_curve->get_baked_points();

        if (!p_track.material1_name.is_empty()) {
            _append_loft_geometry_range_points(
                    p_surface, points, _mirror_road_section(surface_start), _mirror_road_section(surface_end),
                    surface_length, 0.0, 0, CURVE_END);
        }

        if (!p_track.material2_name.is_empty()) {
            const double side_length = _load_road_texture_length(p_track, p_track.material2_name);
            const double side_ratio = _load_texture_ratio(p_track.material2_name);
            Section right_start;
            Section left_start;
            Section right_end;
            Section left_end;
            _build_road_side_sections(road, surface_start, side_length, side_ratio, right_start, left_start);
            _build_road_side_sections(next_road, surface_end, side_length, side_ratio, right_end, left_end);
            // Track.cpp:1503-1514: a shoulder on both sides, a sidewalk only where it has a width
            if (road.tex_height >= 0.0 || !Math::is_zero_approx(road.tex_slope)) {
                _append_loft_geometry_range_points(
                        p_sides, points, _mirror_road_section(right_start), _mirror_road_section(right_end),
                        side_length, 0.0, 0, CURVE_END);
            }
            if (road.tex_height >= 0.0 || !Math::is_zero_approx(road.tex_width)) {
                _append_loft_geometry_range_points(
                        p_sides, points, _mirror_road_section(left_start), _mirror_road_section(left_end), side_length,
                        0.0, 0, CURVE_END);
            }
        }
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
        for (Blade *blade: {&p_track.primary_blade, &p_track.secondary_blade}) {
            blade->move_direction = Vector3();
            blade->pivot = Vector3();
            blade->angle_factor = 0.0;
        }
    }

    /// Puts a streamed road into the cell of its scenario holding its start, the cell made and
    /// registered with the streaming as its first road comes
    void MaszynaTrackRenderingBackend::_road_cell_add(const RID &p_track_render, Track &p_track) {
        const TrackGeometry geometry = _read_track_geometry(p_track.track);
        ERR_FAIL_COND(!geometry.is_valid());
        const Vector3 start = geometry.curve1->get_p1();
        const Vector2i coords(
                static_cast<int32_t>(Math::floor(start.x / ROAD_CELL_SIZE)),
                static_cast<int32_t>(Math::floor(start.z / ROAD_CELL_SIZE)));
        const RoadCellKey key{p_track.scenario, coords};
        RID cell_rid;
        if (const RID *found = road_cells_by_key.getptr(key); found != nullptr) {
            cell_rid = *found;
        } else {
            SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
            TrackRenderingServer *track_rendering = TrackRenderingServer::get_instance();
            ERR_FAIL_NULL(streaming);
            ERR_FAIL_NULL(track_rendering);
            RoadCell cell;
            cell.scenario = p_track.scenario;
            cell.coords = coords;
            cell_rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
            road_cells.insert(cell_rid, cell);
            road_cells_by_key.insert(key, cell_rid);
            const Vector2 centre = (Vector2(coords) + Vector2(0.5, 0.5)) * static_cast<real_t>(ROAD_CELL_SIZE);
            road_cells[cell_rid].stream_rid = streaming->stream_register(
                    road_stream_owner, cell_rid, Vector3(centre.x, start.y, centre.y),
                    static_cast<float>(track_rendering->get_tracks_distance()));
        }
        RoadCell &cell = road_cells[cell_rid];
        if (!cell.tracks.has(p_track_render)) {
            cell.tracks.push_back(p_track_render);
        }
        p_track.road_cell = cell_rid;
        _queue_road_cell_rebuild(cell);
    }

    void MaszynaTrackRenderingBackend::_road_cell_remove(const RID &p_track_render, Track &p_track) {
        const RID cell_rid = p_track.road_cell;
        p_track.road_cell = RID();
        RoadCell *cell = road_cells.getptr(cell_rid);
        if (cell == nullptr) {
            return;
        }
        cell->tracks.erase(p_track_render);
        if (!cell->tracks.is_empty()) {
            _queue_road_cell_rebuild(*cell);
            return;
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->stream_free(cell->stream_rid);
        }
        _road_cell_clear(cell_rid);
        if (cell->instance.is_valid()) {
            RenderingServer::get_singleton()->free_rid(cell->instance);
        }
        road_cells_by_key.erase(RoadCellKey{cell->scenario, cell->coords});
        road_cells.erase(cell_rid);
    }

    /// A built cell is built again once, at the end of the frame, however many of its roads changed
    /// (a scenery puts hundreds into one cell as it loads)
    void MaszynaTrackRenderingBackend::_queue_road_cell_rebuild(RoadCell &p_cell) {
        if (!p_cell.streamed || p_cell.rebuild_queued) {
            return;
        }
        p_cell.rebuild_queued = true;
        if (!road_rebuild_pending) {
            road_rebuild_pending = true;
            callable_mp(this, &MaszynaTrackRenderingBackend::_rebuild_queued_road_cells).call_deferred();
        }
    }

    void MaszynaTrackRenderingBackend::_rebuild_queued_road_cells() {
        road_rebuild_pending = false;
        for (KeyValue<RID, RoadCell> &item: road_cells) {
            RoadCell &cell = item.value;
            if (!cell.rebuild_queued) {
                continue;
            }
            cell.rebuild_queued = false;
            if (cell.streamed) {
                _build_road_cell(cell);
            }
        }
    }

    void MaszynaTrackRenderingBackend::_road_cell_build(const RID &p_cell, const Variant & /* p_preloaded */) {
        RoadCell *cell = road_cells.getptr(p_cell);
        if (cell == nullptr) {
            return;
        }
        cell->streamed = true;
        _build_road_cell(*cell);
    }

    void MaszynaTrackRenderingBackend::_road_cell_clear(const RID &p_cell) {
        RoadCell *cell = road_cells.getptr(p_cell);
        if (cell == nullptr) {
            return;
        }
        cell->streamed = false;
        cell->rebuild_queued = false;
        if (cell->instance.is_valid()) {
            RenderingServer::get_singleton()->instance_set_base(cell->instance, RID());
        }
        cell->mesh.unref();
    }

    /// The cell's visible roads as one mesh, a surface per material
    void MaszynaTrackRenderingBackend::_build_road_cell(RoadCell &p_cell) {
        HashMap<String, Geometry> geometries;
        for (const RID &track_render: p_cell.tracks) {
            const Track *track = tracks.getptr(track_render);
            if (track == nullptr || !track->visible) {
                continue;
            }
            const TrackGeometry geometry = _read_track_geometry(track->track);
            if (!geometry.is_valid()) {
                continue;
            }
            const Ref<Curve3D> curve = _build_curve(geometry.curve1);
            if (curve->get_baked_length() == 0.0) {
                continue;
            }
            if (!track->material1_name.is_empty() && !geometries.has(track->material1_name)) {
                geometries.insert(track->material1_name, Geometry());
            }
            if (!track->material2_name.is_empty() && !geometries.has(track->material2_name)) {
                geometries.insert(track->material2_name, Geometry());
            }
            Geometry unnamed_surface;
            Geometry unnamed_sides;
            _append_road_geometry(
                    *track, geometry, curve,
                    track->material1_name.is_empty() ? unnamed_surface : geometries[track->material1_name],
                    track->material2_name.is_empty() ? unnamed_sides : geometries[track->material2_name]);
        }

        ReloadedMaterialServer *materials = ReloadedMaterialServer::get_instance();
        ERR_FAIL_NULL(materials);
        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        for (const KeyValue<String, Geometry> &item: geometries) {
            const Geometry &geometry = item.value;
            if (geometry.vertices.is_empty()) {
                continue;
            }
            Array arrays;
            arrays.resize(Mesh::ARRAY_MAX);
            arrays[Mesh::ARRAY_VERTEX] = geometry.vertices;
            arrays[Mesh::ARRAY_NORMAL] = geometry.normals;
            arrays[Mesh::ARRAY_TEX_UV] = geometry.uvs;
            arrays[Mesh::ARRAY_INDEX] = geometry.indices;
            mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
            mesh->surface_set_material(
                    mesh->get_surface_count() - 1, materials->material_get(item.key, track_material_options));
        }
        p_cell.mesh = mesh;
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (!p_cell.instance.is_valid()) {
            p_cell.instance = rendering->instance_create();
            rendering->instance_set_scenario(p_cell.instance, p_cell.scenario);
            // flat on the ground: a road's shadow is its shoulder's, and those shaded themselves as the
            // trackbed's slopes did (_slot_draw())
            rendering->instance_geometry_set_cast_shadows_setting(
                    p_cell.instance, RenderingServer::SHADOW_CASTING_SETTING_OFF);
        }
        rendering->instance_set_base(p_cell.instance, mesh->get_surface_count() > 0 ? mesh->get_rid() : RID());
    }

    /// The rail profiles and the materials' sizes are the old data's
    void MaszynaTrackRenderingBackend::_on_data_unload_requested() {
        rail_profiles.clear();
        material_texture_lengths.clear();
        material_texture_ratios.clear();
    }

    /// The roads' cells are built again, taking their materials again (TrackRenderingServer builds the
    /// tracks again itself)
    void MaszynaTrackRenderingBackend::_on_data_reload_requested() {
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->owner_rebuild(road_stream_owner);
        }
    }
} // namespace godot
