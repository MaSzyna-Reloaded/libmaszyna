#include "TrackServer.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "utils/Names.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *TrackServer::switch_active_track_changed_signal = "switch_active_track_changed";
    const char *TrackServer::switch_offset_changed_signal = "switch_offset_changed";
    const char *TrackServer::switch_movement_started_signal = "switch_movement_started";
    const char *TrackServer::switch_movement_finished_signal = "switch_movement_finished";
    const char *TrackServer::tracks_changed_signal = "tracks_changed";
    const char *TrackServer::topology_rebuilt_signal = "topology_rebuilt";
    const char *TrackServer::topology_changed_signal = "topology_changed";
    const char *TrackServer::isolated_occupied_signal = "isolated_occupied";
    const char *TrackServer::isolated_freed_signal = "isolated_freed";
    const char *TrackServer::isolated_vehicle_entered_signal = "isolated_vehicle_entered";
    const char *TrackServer::isolated_vehicle_left_signal = "isolated_vehicle_left";

    double TrackServer::TrackSegment::get_length(const int p_switch_track) const {
        if (p_switch_track == TRACK_DIVERGING && curve2.is_valid()) {
            return length2;
        }
        return length1;
    }


    TrackServer::TrackServer() {
        spatial_index.instantiate();
        spatial_index->set_cell_size(GRID_CELL_SIZE);
        curve_bake_interval =
                ProjectSettings::get_singleton()->get_setting(CURVE_BAKE_INTERVAL_SETTING, DEFAULT_CURVE_BAKE_INTERVAL);
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &TrackServer::_on_project_settings_changed));
    }

    /// The control points stay, so do the endpoints and the topology - only the baked curves and
    /// the lengths measured on them change
    void TrackServer::_on_project_settings_changed() {
        const double interval =
                ProjectSettings::get_singleton()->get_setting(CURVE_BAKE_INTERVAL_SETTING, DEFAULT_CURVE_BAKE_INTERVAL);
        if (interval == curve_bake_interval) {
            return;
        }
        curve_bake_interval = interval;
        for (KeyValue<RID, TrackSegment> &item: tracks) {
            _set_curves(item.value, item.value.curve1, item.value.curve2);
        }
    }

    TrackServer::~TrackServer() {
        _set_switch_processing(false);
    }

    void TrackServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("track_create"), &TrackServer::track_create);
        ClassDB::bind_method(D_METHOD("track_free", "track"), &TrackServer::track_free);
        ClassDB::bind_method(D_METHOD("track_get_rid_by_name", "name"), &TrackServer::track_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("track_get_length", "track", "switch_track"), &TrackServer::track_get_length,
                DEFVAL(TRACK_COMMON));
        ClassDB::bind_method(
                D_METHOD("switch_track_get_length", "track", "switch_track"), &TrackServer::switch_track_get_length);
        ClassDB::bind_method(
                D_METHOD("track_update_curves", "track", "curve1", "curve2"), &TrackServer::track_update_curves);
        ClassDB::bind_method(D_METHOD("track_update", "track", "type", "name", "width"), &TrackServer::track_update);
        ClassDB::bind_method(
                D_METHOD("track_update_properties", "track", "quality_flag", "environment", "sound_distance"),
                &TrackServer::track_update_properties);
        ClassDB::bind_method(D_METHOD("track_exists", "track"), &TrackServer::track_exists);
        ClassDB::bind_method(D_METHOD("track_get_rids"), &TrackServer::track_get_rids);
        ClassDB::bind_method(D_METHOD("track_get_width", "track"), &TrackServer::track_get_width);
        ClassDB::bind_method(D_METHOD("track_get_quality_flag", "track"), &TrackServer::track_get_quality_flag);
        ClassDB::bind_method(D_METHOD("track_get_environment", "track"), &TrackServer::track_get_environment);
        ClassDB::bind_method(D_METHOD("track_get_sound_distance", "track"), &TrackServer::track_get_sound_distance);
        ClassDB::bind_method(D_METHOD("track_set_velocity", "track", "velocity"), &TrackServer::track_set_velocity);
        ClassDB::bind_method(D_METHOD("track_get_velocity", "track"), &TrackServer::track_get_velocity);
        ClassDB::bind_method(
                D_METHOD("track_vehicle_entered", "track", "vehicle"), &TrackServer::track_vehicle_entered);
        ClassDB::bind_method(D_METHOD("track_vehicle_left", "track", "vehicle"), &TrackServer::track_vehicle_left);
        ClassDB::bind_method(D_METHOD("track_is_occupied", "track"), &TrackServer::track_is_occupied);
        ClassDB::bind_method(D_METHOD("track_get_vehicles", "track"), &TrackServer::track_get_vehicles);
        ClassDB::bind_method(D_METHOD("isolated_create"), &TrackServer::isolated_create);
        ClassDB::bind_method(D_METHOD("isolated_free", "isolated"), &TrackServer::isolated_free);
        ClassDB::bind_method(D_METHOD("isolated_set_name", "isolated", "name"), &TrackServer::isolated_set_name);
        ClassDB::bind_method(D_METHOD("isolated_get_name", "isolated"), &TrackServer::isolated_get_name);
        ClassDB::bind_method(D_METHOD("isolated_get_rid_by_name", "name"), &TrackServer::isolated_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("isolated_add_track", "isolated", "track"), &TrackServer::isolated_add_track);
        ClassDB::bind_method(D_METHOD("isolated_set_parent", "isolated", "parent"), &TrackServer::isolated_set_parent);
        ClassDB::bind_method(D_METHOD("isolated_is_occupied", "isolated"), &TrackServer::isolated_is_occupied);
        ClassDB::bind_method(D_METHOD("track_get_endpoints", "track"), &TrackServer::track_get_endpoints);
        ClassDB::bind_method(
                D_METHOD("track_get_common_endpoint_index", "track"), &TrackServer::track_get_common_endpoint_index);
        ClassDB::bind_method(
                D_METHOD("track_get_curve", "track", "branch"), &TrackServer::track_get_curve, DEFVAL(TRACK_COMMON));
        ClassDB::bind_method(
                D_METHOD("track_get_roll", "track", "branch"), &TrackServer::track_get_roll, DEFVAL(TRACK_COMMON));
        ClassDB::bind_method(
                D_METHOD("track_get_domain_curve", "track", "branch"), &TrackServer::track_get_domain_curve,
                DEFVAL(TRACK_COMMON));
        ClassDB::bind_method(
                D_METHOD("switch_get_common_endpoints", "track"), &TrackServer::switch_get_common_endpoints);
        ClassDB::bind_method(
                D_METHOD("switch_get_branch_start_endpoint", "track", "branch"),
                &TrackServer::switch_get_branch_start_endpoint);
        ClassDB::bind_method(
                D_METHOD("switch_get_branch_end_endpoint", "track", "branch"),
                &TrackServer::switch_get_branch_end_endpoint);
        ClassDB::bind_method(
                D_METHOD("switch_get_endpoint_branch", "track", "endpoint_index"),
                &TrackServer::switch_get_endpoint_branch);
        ClassDB::bind_method(
                D_METHOD("switch_get_blade_boundary_offset", "track", "branch"),
                &TrackServer::switch_get_blade_boundary_offset);
        ClassDB::bind_method(D_METHOD("switch_get_f_offset1", "track"), &TrackServer::switch_get_f_offset1);
        ClassDB::bind_method(D_METHOD("switch_get_f_offset2", "track"), &TrackServer::switch_get_f_offset2);
        ClassDB::bind_method(D_METHOD("tracks_find_in_aabb", "aabb"), &TrackServer::tracks_find_in_aabb);
        ClassDB::bind_method(
                D_METHOD("track_get_endpoint_connections", "track", "endpoint_index"),
                &TrackServer::track_get_endpoint_connections);
        ClassDB::bind_method(
                D_METHOD("switch_track_get_neighbors", "track", "switch_track"),
                &TrackServer::switch_track_get_neighbors);
        ClassDB::bind_method(D_METHOD("track_get_next", "track", "endpoint_index"), &TrackServer::track_get_next);
        ClassDB::bind_method(
                D_METHOD(
                        "track_trace_route", "track", "branch", "branch_from_setting", "toward_end", "start",
                        "distance"),
                &TrackServer::track_trace_route);
        ClassDB::bind_method(D_METHOD("track_is_switch", "track"), &TrackServer::track_is_switch);
        ClassDB::bind_method(D_METHOD("switch_is_right", "track"), &TrackServer::switch_is_right);
        ClassDB::bind_method(D_METHOD("switch_get_active_track", "track"), &TrackServer::switch_get_active_track);
        ClassDB::bind_method(D_METHOD("track_get_name", "track"), &TrackServer::track_get_name);
        ClassDB::bind_method(
                D_METHOD("switch_set_active_track", "track", "active_track"), &TrackServer::switch_set_active_track);
        ClassDB::bind_method(D_METHOD("topology_get_summary"), &TrackServer::topology_get_summary);
        ClassDB::bind_method(D_METHOD("topology_rebuild"), &TrackServer::topology_rebuild);

        ClassDB::bind_method(D_METHOD("get_switch_max_offset"), &TrackServer::get_switch_max_offset);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "switch_max_offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "",
                "get_switch_max_offset");
        ClassDB::bind_method(D_METHOD("get_switch_offset_delay"), &TrackServer::get_switch_offset_delay);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "switch_offset_delay", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY),
                "", "get_switch_offset_delay");
        ClassDB::bind_method(D_METHOD("get_rail_height"), &TrackServer::get_rail_height);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "rail_height", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "",
                "get_rail_height");
        ClassDB::bind_method(D_METHOD("get_is_topology_changed"), &TrackServer::get_is_topology_changed);
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "is_topology_changed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY),
                "", "get_is_topology_changed");

        BIND_ENUM_CONSTANT(TRACK_COMMON);
        BIND_ENUM_CONSTANT(TRACK_DIVERGING);
        BIND_ENUM_CONSTANT(POINT_NONE);
        BIND_ENUM_CONSTANT(POINT_P1);
        BIND_ENUM_CONSTANT(POINT_P2);
        BIND_ENUM_CONSTANT(CURVE1_P1);
        BIND_ENUM_CONSTANT(CURVE1_P2);
        BIND_ENUM_CONSTANT(CURVE2_P1);
        BIND_ENUM_CONSTANT(CURVE2_P2);
        BIND_ENUM_CONSTANT(TRACK_UNKNOWN);
        BIND_ENUM_CONSTANT(TRACK_NORMAL);
        BIND_ENUM_CONSTANT(TRACK_SWITCH);
        BIND_ENUM_CONSTANT(TRACK_ROAD);
        BIND_ENUM_CONSTANT(TRACK_CROSS);
        BIND_ENUM_CONSTANT(TRACK_RIVER);
        BIND_ENUM_CONSTANT(TRACK_TRIBUTARY);
        BIND_ENUM_CONSTANT(TRACK_TURN);
        BIND_ENUM_CONSTANT(TRACK_TABLE);
        BIND_ENUM_CONSTANT(GROUP_NONE);
        BIND_ENUM_CONSTANT(GROUP_RAIL);
        BIND_ENUM_CONSTANT(GROUP_ROAD);
        BIND_ENUM_CONSTANT(GROUP_WATER);
        BIND_ENUM_CONSTANT(DIRECTION_NORMAL);
        BIND_ENUM_CONSTANT(DIRECTION_REVERSED);
        BIND_CONSTANT(INVALID_NODE_ID);

        ADD_SIGNAL(MethodInfo(
                switch_active_track_changed_signal, PropertyInfo(Variant::RID, "track_rid"),
                PropertyInfo(Variant::INT, "active_track")));
        ADD_SIGNAL(MethodInfo(
                switch_offset_changed_signal, PropertyInfo(Variant::RID, "track_rid"),
                PropertyInfo(Variant::FLOAT, "offset")));
        ADD_SIGNAL(MethodInfo(
                switch_movement_started_signal, PropertyInfo(Variant::RID, "track_rid"),
                PropertyInfo(Variant::INT, "from_track"), PropertyInfo(Variant::INT, "to_track")));
        ADD_SIGNAL(MethodInfo(
                switch_movement_finished_signal, PropertyInfo(Variant::RID, "track_rid"),
                PropertyInfo(Variant::INT, "active_track")));
        ADD_SIGNAL(MethodInfo(tracks_changed_signal));
        ADD_SIGNAL(MethodInfo(topology_rebuilt_signal));
        ADD_SIGNAL(MethodInfo(topology_changed_signal));
        for (const char *signal:
             {isolated_occupied_signal, isolated_freed_signal, isolated_vehicle_entered_signal,
              isolated_vehicle_left_signal}) {
            ADD_SIGNAL(
                    MethodInfo(signal, PropertyInfo(Variant::RID, "isolated"), PropertyInfo(Variant::RID, "vehicle")));
        }
    }

    double TrackServer::get_switch_max_offset() const {
        return SWITCH_MAX_OFFSET;
    }

    double TrackServer::get_switch_offset_delay() const {
        return SWITCH_OFFSET_DELAY;
    }

    double TrackServer::get_rail_height() const {
        return RAIL_HEIGHT;
    }

    /* Port of the original engine's Equal() (Track.cpp:2121) - a per-axis cube test, not a sphere,
     * used everywhere two track endpoints are checked for being the same point. */
    bool TrackServer::_endpoints_equal(const Vector3 &p_first, const Vector3 &p_second) {
        return Math::abs(p_first.x - p_second.x) <= ENDPOINT_EPSILON &&
               Math::abs(p_first.y - p_second.y) <= ENDPOINT_EPSILON &&
               Math::abs(p_first.z - p_second.z) <= ENDPOINT_EPSILON;
    }

    int TrackServer::_track_type_group(const int p_type) {
        switch (p_type) {
            case TRACK_NORMAL:
            case TRACK_SWITCH:
            case TRACK_TURN:
            case TRACK_TABLE:
                return GROUP_RAIL;
            case TRACK_ROAD:
            case TRACK_CROSS:
                return GROUP_ROAD;
            case TRACK_RIVER:
            case TRACK_TRIBUTARY:
                return GROUP_WATER;
            default:
                return GROUP_NONE;
        }
    }

    /* TrackCurve is a GDScript Resource (scenes serialize it), so its class is not knowable
     * at build time and its fields come through the Variant property API. Read once, here, and
     * cached as plain values - nothing downstream touches the resource again. */
    void TrackServer::_read_curve_points(const Ref<Resource> &p_curve, CurvePoints &p_points) const {
        p_points = CurvePoints();
        if (p_curve.is_null()) {
            return;
        }
        p_points.p1 = p_curve->get("p1");
        p_points.c1 = p_curve->get("c1");
        p_points.c2 = p_curve->get("c2");
        p_points.p2 = p_curve->get("p2");
        p_points.roll1 = p_curve->get("roll1");
        p_points.roll2 = p_curve->get("roll2");
    }

    Ref<Curve3D> TrackServer::_build_domain_curve(const CurvePoints &p_points) const {
        Ref<Curve3D> curve;
        curve.instantiate();
        curve->set_closed(false);
        curve->set_bake_interval(static_cast<real_t>(curve_bake_interval));
        const double roll_fix1 = Math::abs(Math::sin(Math::deg_to_rad(p_points.roll1)) * ROLL_FIX_FACTOR);
        const double roll_fix2 = Math::abs(Math::sin(Math::deg_to_rad(p_points.roll2)) * ROLL_FIX_FACTOR);
        curve->add_point(p_points.p1 + Vector3(0.0, static_cast<real_t>(roll_fix1), 0.0), Vector3(), p_points.c1);
        curve->add_point(p_points.p2 + Vector3(0.0, static_cast<real_t>(roll_fix2), 0.0), p_points.c2, Vector3());
        return curve;
    }

    const PackedVector3Array &TrackServer::_endpoints(TrackSegment &p_track) const {
        if (!p_track.cached_endpoints.is_empty()) {
            return p_track.cached_endpoints;
        }
        if (p_track.curve1.is_valid()) {
            p_track.cached_endpoints.push_back(p_track.points1.p1);
            p_track.cached_endpoints.push_back(p_track.points1.p2);
        }
        if (p_track.curve2.is_valid()) {
            p_track.cached_endpoints.push_back(p_track.points2.p1);
            p_track.cached_endpoints.push_back(p_track.points2.p2);
        }
        return p_track.cached_endpoints;
    }

    void TrackServer::_append_common_switch_endpoint(
            TrackSegment &p_track, const Vector3 &p_first, const Vector3 &p_second, const int p_first_endpoint,
            const int p_second_endpoint) const {
        if (!_endpoints_equal(p_first, p_second)) {
            return;
        }
        if (!p_track.switch_common_endpoints.has(p_first_endpoint)) {
            p_track.switch_common_endpoints.push_back(p_first_endpoint);
        }
        if (!p_track.switch_common_endpoints.has(p_second_endpoint)) {
            p_track.switch_common_endpoints.push_back(p_second_endpoint);
        }
    }

    double
    TrackServer::_switch_blade_boundary_offset(const Ref<Curve3D> &p_branch_curve, const double p_frog_distance) const {
        if (p_frog_distance <= 0.0) {
            return 0.0;
        }
        PackedVector3Array sampled;
        for (int index = 0; index <= SWITCH_BLADE_SEGMENT_COUNT; ++index) {
            const double distance = p_frog_distance * static_cast<double>(index) / SWITCH_BLADE_SEGMENT_COUNT;
            sampled.push_back(p_branch_curve->sample_baked(static_cast<real_t>(distance), true));
        }
        if (sampled.size() < 2) {
            return 0.0;
        }
        double boundary_offset = 0.0;
        const int blade_sample_count =
                MIN(static_cast<int>(Math::ceil(SWITCH_BLADE_SEGMENT_COUNT * SWITCH_BLADE_RATIO)),
                    static_cast<int>(sampled.size()) - 1);
        for (int index = 0; index < blade_sample_count; ++index) {
            boundary_offset += sampled[index].distance_to(sampled[index + 1]);
        }
        return boundary_offset;
    }

    void TrackServer::_update_switch_blade_boundary_offsets(TrackSegment &p_track) const {
        p_track.switch_blade_boundary_offsets[TRACK_COMMON] = p_track.get_length(TRACK_COMMON);
        p_track.switch_blade_boundary_offsets[TRACK_DIVERGING] = p_track.get_length(TRACK_DIVERGING);
        if (p_track.domain_curve1.is_null() || p_track.domain_curve2.is_null()) {
            return;
        }

        double frog_distance = MIN(p_track.length1, p_track.length2);
        double sample_distance = 0.0;
        while (sample_distance < frog_distance) {
            const Vector3 point1 = p_track.domain_curve1->sample_baked(static_cast<real_t>(sample_distance));
            const Vector3 point2 = p_track.domain_curve2->sample_baked(static_cast<real_t>(sample_distance));
            if (point1.distance_to(point2) >= p_track.width) {
                frog_distance = sample_distance;
                break;
            }
            sample_distance += FROG_SEARCH_STEP;
        }

        p_track.switch_blade_boundary_offsets[TRACK_COMMON] =
                _switch_blade_boundary_offset(p_track.domain_curve1, frog_distance);
        p_track.switch_blade_boundary_offsets[TRACK_DIVERGING] =
                _switch_blade_boundary_offset(p_track.domain_curve2, frog_distance);
    }

    void TrackServer::_set_curves(TrackSegment &p_track, const Ref<Resource> &p_curve1, const Ref<Resource> &p_curve2) {
        p_track.curve1 = p_curve1;
        p_track.curve2 = p_curve2;
        _read_curve_points(p_curve1, p_track.points1);
        _read_curve_points(p_curve2, p_track.points2);
        p_track.domain_curve1 = p_curve1.is_valid() ? _build_domain_curve(p_track.points1) : Ref<Curve3D>();
        p_track.domain_curve2 = p_curve2.is_valid() ? _build_domain_curve(p_track.points2) : Ref<Curve3D>();
        p_track.cached_endpoints.clear();
        p_track.switch_is_right = false;
        p_track.length = 0.0;
        p_track.length1 = 0.0;
        p_track.length2 = 0.0;
        if (p_track.domain_curve1.is_valid()) {
            p_track.length1 = p_track.domain_curve1->get_baked_length();
            p_track.length += p_track.length1;
        }
        if (p_track.domain_curve2.is_valid()) {
            p_track.length2 = p_track.domain_curve2->get_baked_length();
            p_track.length += p_track.length2;
        }

        // the endpoint of the common track the two curves share
        p_track.switch_common_endpoint_index = POINT_NONE;
        if (p_curve1.is_valid() && p_curve2.is_valid()) {
            if (_endpoints_equal(p_track.points1.p1, p_track.points2.p1) ||
                _endpoints_equal(p_track.points1.p1, p_track.points2.p2)) {
                p_track.switch_common_endpoint_index = POINT_P1;
            } else if (
                    _endpoints_equal(p_track.points1.p2, p_track.points2.p1) ||
                    _endpoints_equal(p_track.points1.p2, p_track.points2.p2)) {
                p_track.switch_common_endpoint_index = POINT_P2;
            }
        }

        // which branch each endpoint belongs to, and which endpoints the branches share
        p_track.switch_common_endpoints.clear();
        p_track.switch_branch_start_endpoints[TRACK_COMMON] = -1;
        p_track.switch_branch_start_endpoints[TRACK_DIVERGING] = -1;
        p_track.switch_branch_end_endpoints[TRACK_COMMON] = -1;
        p_track.switch_branch_end_endpoints[TRACK_DIVERGING] = -1;
        for (int &branch: p_track.switch_endpoint_branches) {
            branch = -1;
        }
        if (p_curve1.is_valid()) {
            p_track.switch_branch_start_endpoints[TRACK_COMMON] = CURVE1_P1;
            p_track.switch_branch_end_endpoints[TRACK_COMMON] = CURVE1_P2;
            p_track.switch_endpoint_branches[CURVE1_P1] = TRACK_COMMON;
            p_track.switch_endpoint_branches[CURVE1_P2] = TRACK_COMMON;
        }
        if (p_curve2.is_valid()) {
            p_track.switch_branch_start_endpoints[TRACK_DIVERGING] = CURVE2_P1;
            p_track.switch_branch_end_endpoints[TRACK_DIVERGING] = CURVE2_P2;
            p_track.switch_endpoint_branches[CURVE2_P1] = TRACK_DIVERGING;
            p_track.switch_endpoint_branches[CURVE2_P2] = TRACK_DIVERGING;
        }
        if (p_curve1.is_valid() && p_curve2.is_valid()) {
            _append_common_switch_endpoint(p_track, p_track.points1.p1, p_track.points2.p1, CURVE1_P1, CURVE2_P1);
            _append_common_switch_endpoint(p_track, p_track.points1.p1, p_track.points2.p2, CURVE1_P1, CURVE2_P2);
            _append_common_switch_endpoint(p_track, p_track.points1.p2, p_track.points2.p1, CURVE1_P2, CURVE2_P1);
            _append_common_switch_endpoint(p_track, p_track.points1.p2, p_track.points2.p2, CURVE1_P2, CURVE2_P2);
        }
        _update_switch_blade_boundary_offsets(p_track);

        if (p_curve1.is_null() || p_curve2.is_null() || p_track.switch_common_endpoint_index == POINT_NONE) {
            return;
        }
        const Vector3 common_position =
                p_track.switch_common_endpoint_index == POINT_P1 ? p_track.points1.p1 : p_track.points1.p2;
        const auto direction_from_common = [](const CurvePoints &p_points, const Vector3 &p_common) {
            if (_endpoints_equal(p_points.p1, p_common)) {
                return p_points.p2 - p_points.p1;
            }
            if (_endpoints_equal(p_points.p2, p_common)) {
                return p_points.p1 - p_points.p2;
            }
            return Vector3();
        };
        const Vector3 primary = direction_from_common(p_track.points1, common_position);
        const Vector3 secondary = direction_from_common(p_track.points2, common_position);
        const double primary_angle = Math::atan2(primary.x, primary.z);
        const double secondary_angle = Math::atan2(secondary.x, secondary.z);
        double angle_delta = secondary_angle - primary_angle;
        while (angle_delta > Math::PI) {
            angle_delta -= 2.0 * Math::PI;
        }
        while (angle_delta < -Math::PI) {
            angle_delta += 2.0 * Math::PI;
        }
        p_track.switch_is_right = angle_delta < 0.0;
    }

    // MaSzyna Track.cpp:1933-1941 clamps blade offsets.
    void TrackServer::_set_switch_f_offset(TrackSegment &p_track, const double p_value) {
        p_track.switch_f_offset = p_value;
        p_track.switch_f_offset1 = MIN(p_value, SWITCH_MAX_OFFSET);
        p_track.switch_f_offset2 = MAX(p_value, 0.0);
        emit_signal(switch_offset_changed_signal, p_track.track_rid, p_value);
    }

    /// The blade animation runs only while a switch is actually moving - the same shape as
    /// E3DRenderingServer's smoke tick, and the reason this needs no Tween of its own.
    void TrackServer::_set_switch_processing(const bool p_processing) {
        if (switch_processing == p_processing) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr) {
            return;
        }
        switch_processing = p_processing;
        if (p_processing) {
            tree->connect("process_frame", callable_mp(this, &TrackServer::_process_switches));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &TrackServer::_process_switches));
    }

    // MaSzyna Track.cpp:1933-1955 animates domain offset.
    void TrackServer::_process_switches() {
        const uint64_t now = Time::get_singleton()->get_ticks_usec();
        const double delta = static_cast<double>(now - last_switch_step_usec) / LibMaszynaUnits::USEC_PER_SECOND;
        last_switch_step_usec = now;
        for (int index = static_cast<int>(moving_switches.size()) - 1; index >= 0; --index) {
            const RID track_rid = moving_switches[index];
            TrackSegment *track = tracks.getptr(track_rid);
            if (track == nullptr) {
                moving_switches.remove_at(index);
                continue;
            }
            const double remaining = track->switch_desired_offset - track->switch_f_offset;
            // Track.cpp:1944 - the blade moves at a constant speed
            const double step = SWITCH_OFFSET_SPEED * delta;
            if (Math::abs(remaining) <= step) {
                _set_switch_f_offset(*track, track->switch_desired_offset);
                moving_switches.remove_at(index);
                emit_signal(switch_movement_finished_signal, track_rid, track->active_track);
                continue;
            }
            _set_switch_f_offset(*track, track->switch_f_offset + (remaining > 0.0 ? step : -step));
        }
        if (moving_switches.is_empty()) {
            _set_switch_processing(false);
        }
    }

    RID TrackServer::track_create() {
        ++next_track_id;
        const RID track_rid = UtilityFunctions::rid_from_int64(next_track_id);
        TrackSegment track;
        track.track_rid = track_rid;
        tracks.insert(track_rid, track);
        _mark_topology_changed();
        emit_signal(tracks_changed_signal);
        return track_rid;
    }

    void TrackServer::track_free(const RID &p_track) {
        if (!tracks.has(p_track)) {
            return;
        }
        moving_switches.erase(p_track);
        for (const KeyValue<String, RID> &named: named_tracks) {
            if (named.value == p_track) {
                named_tracks.erase(named.key);
                break;
            }
        }
        spatial_index->remove(p_track);
        for (const RID &isolated: tracks.getptr(p_track)->isolated) {
            if (IsolatedData *section = isolated_sections.getptr(isolated); section != nullptr) {
                section->tracks.erase(p_track);
            }
        }
        tracks.erase(p_track);
        _clear_topology();
        _mark_topology_changed();
        emit_signal(tracks_changed_signal);
    }

    RID TrackServer::track_get_rid_by_name(const String &p_name) const {
        const RID *track_rid = named_tracks.getptr(p_name);
        return track_rid != nullptr ? *track_rid : RID();
    }

    double TrackServer::track_get_length(const RID &p_track, const int p_switch_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return 0.0;
        }
        if (track->type == TRACK_SWITCH) {
            return track->get_length(p_switch_track);
        }
        return track->get_length(TRACK_COMMON);
    }

    double TrackServer::switch_track_get_length(const RID &p_track, const int p_switch_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->get_length(p_switch_track) : 0.0;
    }

    void
    TrackServer::track_update_curves(const RID &p_track, const Ref<Resource> &p_curve1, const Ref<Resource> &p_curve2) {
        TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return;
        }

        const PackedVector3Array previous_endpoints = _endpoints(*track);
        PackedVector3Array next_endpoints;
        CurvePoints next_points1;
        CurvePoints next_points2;
        _read_curve_points(p_curve1, next_points1);
        _read_curve_points(p_curve2, next_points2);
        if (p_curve1.is_valid()) {
            next_endpoints.push_back(next_points1.p1);
            next_endpoints.push_back(next_points1.p2);
        }
        if (p_curve2.is_valid()) {
            next_endpoints.push_back(next_points2.p1);
            next_endpoints.push_back(next_points2.p2);
        }

        bool changed_connected_endpoint = false;
        for (int endpoint_index = 0; endpoint_index < previous_endpoints.size(); ++endpoint_index) {
            const TrackNode *node = _get_node(track->node_ids[endpoint_index]);
            if (node == nullptr) {
                continue;
            }
            bool has_external_connection = false;
            for (const EndpointPair &ref: node->endpoint_refs) {
                if (ref.track_rid != track->track_rid) {
                    has_external_connection = true;
                    break;
                }
            }
            if (!has_external_connection) {
                continue;
            }
            if (endpoint_index >= next_endpoints.size()) {
                changed_connected_endpoint = true;
                break;
            }
            if (!_endpoints_equal(previous_endpoints[endpoint_index], next_endpoints[endpoint_index])) {
                changed_connected_endpoint = true;
                break;
            }
        }

        _set_curves(*track, p_curve1, p_curve2);

        if (changed_connected_endpoint) {
            _mark_topology_changed();
        }
    }

    void TrackServer::track_update(const RID &p_track, const int p_type, const String &p_name, const double p_width) {
        TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return;
        }
        // Looking a track up by its RID is a linear scan over every named track - only worth
        // paying for when this update actually carries a name that might replace a previous one.
        // Most real scenery tracks are unnamed, so the common case stays O(1).
        if (!p_name.is_empty()) {
            for (const KeyValue<String, RID> &named: named_tracks) {
                if (named.value == p_track && named.key != p_name) {
                    named_tracks.erase(named.key);
                    break;
                }
            }
            // A duplicate name only means by-name lookup resolves to whichever track claimed it
            // first - it must not stop this track from getting its own width/type/AABB configured
            // below.
            const RID *existing = named_tracks.getptr(p_name);
            if (existing != nullptr && *existing != p_track) {
                UtilityFunctions::push_error("Named track already exists: " + p_name);
            } else {
                named_tracks.insert(p_name, p_track);
            }
        }

        const double previous_width = track->width;
        track->type = p_type;
        track->width = p_width;
        if (!Math::is_equal_approx(previous_width, p_width)) {
            _update_switch_blade_boundary_offsets(*track);
        }
        const PackedVector3Array points = _endpoints(*track);
        if (points.is_empty()) {
            spatial_index->remove(p_track);
        } else {
            Rect2 rect(Vector2(points[0].x, points[0].z), Vector2());
            for (const Vector3 &point: points) {
                rect = rect.expand(Vector2(point.x, point.z));
            }
            track->aabb = rect.grow(static_cast<real_t>(AABB_MARGIN));
            spatial_index->remove(p_track);
            spatial_index->add(p_track, track->aabb);
        }
        emit_signal(tracks_changed_signal);
    }

    // Track properties read by the running sounds (Track.cpp:449).
    void TrackServer::track_update_properties(
            const RID &p_track, const int p_quality_flag, const int p_environment, const double p_sound_distance) {
        TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return;
        }
        track->quality_flag = p_quality_flag;
        track->environment = p_environment;
        track->sound_distance = p_sound_distance;
    }

    bool TrackServer::track_exists(const RID &p_track) const {
        return tracks.has(p_track);
    }

    TypedArray<RID> TrackServer::track_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, TrackSegment> &track: tracks) {
            result.push_back(track.key);
        }
        return result;
    }

    double TrackServer::track_get_width(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->width : 0.0;
    }

    int TrackServer::track_get_quality_flag(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->quality_flag : 0;
    }

    int TrackServer::track_get_environment(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->environment : 0;
    }

    double TrackServer::track_get_sound_distance(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->sound_distance : -1.0;
    }

    void TrackServer::track_set_velocity(const RID &p_track, const double p_velocity) {
        TrackSegment *track = tracks.getptr(p_track);
        ERR_FAIL_NULL(track);
        track->velocity = p_velocity;
    }

    double TrackServer::track_get_velocity(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->velocity : -1.0;
    }

    void TrackServer::track_vehicle_entered(const RID &p_track, const RID &p_vehicle) {
        TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return; // a vehicle off the tracks, or a track already gone
        }
        track->vehicles.push_back(p_vehicle);
        // copied: a listener may add the track to a section
        const Vector<RID> isolated = track->isolated;
        for (const RID &section: isolated) {
            _count_isolated(section, 1, p_vehicle);
        }
    }

    void TrackServer::track_vehicle_left(const RID &p_track, const RID &p_vehicle) {
        TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return; // the track went first
        }
        track->vehicles.erase(p_vehicle);
        const Vector<RID> isolated = track->isolated;
        for (const RID &section: isolated) {
            _count_isolated(section, -1, p_vehicle);
        }
    }

    bool TrackServer::track_is_occupied(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr && !track->vehicles.is_empty();
    }

    TypedArray<RID> TrackServer::track_get_vehicles(const RID &p_track) const {
        TypedArray<RID> result;
        if (const TrackSegment *track = tracks.getptr(p_track); track != nullptr) {
            for (const RID &vehicle: track->vehicles) {
                result.push_back(vehicle);
            }
        }
        return result;
    }

    /// TIsolated::Modify() (Track.cpp:118-170)
    void TrackServer::_count_isolated(const RID &p_isolated, const int p_delta, const RID &p_vehicle) {
        IsolatedData *section = isolated_sections.getptr(p_isolated);
        if (section == nullptr) {
            return;
        }
        const int previous = section->vehicle_count;
        section->vehicle_count += p_delta;
        const int current = section->vehicle_count;
        const RID parent = section->parent;
        if (previous == 0 && current > 0) {
            emit_signal(isolated_occupied_signal, p_isolated, p_vehicle);
        } else if (previous > 0 && current == 0) {
            emit_signal(isolated_freed_signal, p_isolated, p_vehicle);
        }
        emit_signal(
                p_delta > 0 ? isolated_vehicle_entered_signal : isolated_vehicle_left_signal, p_isolated, p_vehicle);
        if (parent.is_valid()) {
            _count_isolated(parent, p_delta, p_vehicle);
        }
    }

    RID TrackServer::isolated_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        isolated_sections.insert(rid, IsolatedData());
        return rid;
    }

    void TrackServer::isolated_free(const RID &p_isolated) {
        const IsolatedData *section = isolated_sections.getptr(p_isolated);
        ERR_FAIL_NULL(section);
        for (const RID &track_rid: section->tracks) {
            if (TrackSegment *track = tracks.getptr(track_rid); track != nullptr) {
                track->isolated.erase(p_isolated);
            }
        }
        names_rename(isolated_by_name, section->name, StringName(), p_isolated);
        isolated_sections.erase(p_isolated);
    }

    void TrackServer::isolated_set_name(const RID &p_isolated, const StringName &p_name) {
        IsolatedData *section = isolated_sections.getptr(p_isolated);
        ERR_FAIL_NULL(section);
        names_rename(isolated_by_name, section->name, p_name, p_isolated);
        section->name = p_name;
    }

    StringName TrackServer::isolated_get_name(const RID &p_isolated) const {
        const IsolatedData *section = isolated_sections.getptr(p_isolated);
        ERR_FAIL_NULL_V(section, StringName());
        return section->name;
    }

    RID TrackServer::isolated_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = isolated_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    /// The track's vehicles count for the section from now on
    void TrackServer::isolated_add_track(const RID &p_isolated, const RID &p_track) {
        IsolatedData *section = isolated_sections.getptr(p_isolated);
        ERR_FAIL_NULL(section);
        TrackSegment *track = tracks.getptr(p_track);
        ERR_FAIL_NULL(track);
        if (track->isolated.has(p_isolated)) {
            return; // named by the track and by an `isolated` block both
        }
        track->isolated.push_back(p_isolated);
        section->tracks.push_back(p_track);
        section->vehicle_count += static_cast<int>(track->vehicles.size());
    }

    void TrackServer::isolated_set_parent(const RID &p_isolated, const RID &p_parent) {
        IsolatedData *section = isolated_sections.getptr(p_isolated);
        ERR_FAIL_NULL(section);
        section->parent = p_parent;
    }

    bool TrackServer::isolated_is_occupied(const RID &p_isolated) const {
        const IsolatedData *section = isolated_sections.getptr(p_isolated);
        ERR_FAIL_NULL_V(section, false);
        return section->vehicle_count > 0;
    }

    PackedVector3Array TrackServer::track_get_endpoints(const RID &p_track) {
        TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? _endpoints(*track) : PackedVector3Array();
    }

    int TrackServer::track_get_common_endpoint_index(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->switch_common_endpoint_index : POINT_NONE;
    }

    Ref<Resource> TrackServer::track_get_curve(const RID &p_track, const int p_branch) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            UtilityFunctions::push_error("Track RID is not valid");
            return Ref<Resource>();
        }
        if (p_branch == TRACK_DIVERGING && track->curve2.is_valid()) {
            return track->curve2;
        }
        return track->curve1;
    }

    Vector2 TrackServer::track_get_roll(const RID &p_track, const int p_branch) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            return Vector2();
        }
        const CurvePoints &points =
                p_branch == TRACK_DIVERGING && track->curve2.is_valid() ? track->points2 : track->points1;
        return Vector2(static_cast<real_t>(points.roll1), static_cast<real_t>(points.roll2));
    }

    Ref<Curve3D> TrackServer::track_get_domain_curve(const RID &p_track, const int p_branch) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            UtilityFunctions::push_error("Track RID is not valid");
            return Ref<Curve3D>();
        }
        if (p_branch == TRACK_DIVERGING && track->domain_curve2.is_valid()) {
            return track->domain_curve2;
        }
        return track->domain_curve1;
    }

    PackedInt32Array TrackServer::switch_get_common_endpoints(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->switch_common_endpoints : PackedInt32Array();
    }

    int TrackServer::switch_get_branch_start_endpoint(const RID &p_track, const int p_branch) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track != nullptr && p_branch >= 0 && p_branch <= TRACK_DIVERGING &&
            track->switch_branch_start_endpoints[p_branch] >= 0) {
            return track->switch_branch_start_endpoints[p_branch];
        }
        return CURVE1_P1;
    }

    int TrackServer::switch_get_branch_end_endpoint(const RID &p_track, const int p_branch) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track != nullptr && p_branch >= 0 && p_branch <= TRACK_DIVERGING &&
            track->switch_branch_end_endpoints[p_branch] >= 0) {
            return track->switch_branch_end_endpoints[p_branch];
        }
        return CURVE1_P2;
    }

    int TrackServer::switch_get_endpoint_branch(const RID &p_track, const int p_endpoint_index) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track != nullptr && p_endpoint_index >= 0 && p_endpoint_index <= CURVE2_P2 &&
            track->switch_endpoint_branches[p_endpoint_index] >= 0) {
            return track->switch_endpoint_branches[p_endpoint_index];
        }
        return TRACK_COMMON;
    }

    double TrackServer::switch_get_blade_boundary_offset(const RID &p_track, const int p_branch) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track != nullptr && p_branch >= 0 && p_branch <= TRACK_DIVERGING) {
            return track->switch_blade_boundary_offsets[p_branch];
        }
        return 0.0;
    }

    double TrackServer::switch_get_f_offset1(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->switch_f_offset1 : 0.0;
    }

    double TrackServer::switch_get_f_offset2(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr ? track->switch_f_offset2 : 0.0;
    }

    TypedArray<RID> TrackServer::tracks_find_in_aabb(const Rect2 &p_aabb) const {
        return spatial_index->query(p_aabb);
    }

    TypedArray<TrackEndpointRef>
    TrackServer::track_get_endpoint_connections(const RID &p_track, const int p_endpoint_index) {
        TypedArray<TrackEndpointRef> result;
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr || p_endpoint_index < 0 || p_endpoint_index > CURVE2_P2) {
            return result;
        }
        const TrackNode *node = _get_node(track->node_ids[p_endpoint_index]);
        if (node == nullptr) {
            return result;
        }
        for (const EndpointPair &ref: node->endpoint_refs) {
            if (ref.track_rid == p_track) {
                continue;
            }
            bool is_duplicate = false;
            for (int index = 0; index < result.size(); ++index) {
                const Ref<TrackEndpointRef> connection = result[index];
                if (connection->get_track_rid() == ref.track_rid &&
                    connection->get_endpoint_index() == ref.endpoint_index) {
                    is_duplicate = true;
                    break;
                }
            }
            if (is_duplicate) {
                continue;
            }
            Ref<TrackEndpointRef> connection;
            connection.instantiate();
            connection->set_track_rid(ref.track_rid);
            connection->set_endpoint_index(ref.endpoint_index);
            result.push_back(connection);
        }
        return result;
    }

    Ref<TrackBranchNeighbors> TrackServer::switch_track_get_neighbors(const RID &p_track, const int p_switch_track) {
        Ref<TrackBranchNeighbors> neighbors;
        neighbors.instantiate();
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr || track->switch_common_endpoint_index == POINT_NONE) {
            return neighbors;
        }
        if (p_switch_track == TRACK_DIVERGING && track->curve2.is_null()) {
            return neighbors;
        }

        const int previous_endpoint_index = p_switch_track == TRACK_DIVERGING ? CURVE2_P1 : CURVE1_P1;
        const int next_endpoint_index = p_switch_track == TRACK_DIVERGING ? CURVE2_P2 : CURVE1_P2;

        const TypedArray<TrackEndpointRef> previous_connections =
                track_get_endpoint_connections(p_track, previous_endpoint_index);
        if (previous_connections.size() == 1) {
            const Ref<TrackEndpointRef> connection = previous_connections[0];
            neighbors->set_previous_track_rid(connection->get_track_rid());
            neighbors->set_previous_endpoint_index(connection->get_endpoint_index());
        }
        const TypedArray<TrackEndpointRef> next_connections =
                track_get_endpoint_connections(p_track, next_endpoint_index);
        if (next_connections.size() == 1) {
            const Ref<TrackEndpointRef> connection = next_connections[0];
            neighbors->set_next_track_rid(connection->get_track_rid());
            neighbors->set_next_endpoint_index(connection->get_endpoint_index());
        }
        return neighbors;
    }

    /* The single track a movement leaving p_track at p_endpoint_index continues onto, and the
     * switch branch it forces when it enters a switch from a branch side (NO_FORCED_SWITCH_TRACK
     * otherwise); false when the node is open or ambiguous. Nothing is changed. */
    TypedArray<TrackRouteSegment> TrackServer::track_trace_route(
            const RID &p_track, const int p_branch, const bool p_branch_from_setting, const bool p_toward_end,
            const double p_start, const double p_distance) {
        TypedArray<TrackRouteSegment> route;
        if (!track_exists(p_track)) {
            return route;
        }
        RID track = p_track;
        int branch = p_branch;
        bool branch_from_setting = p_branch_from_setting;
        bool is_switch = track_is_switch(track);
        double movement_sign = p_toward_end ? 1.0 : -1.0;
        double length = track_get_length(track, branch);
        double start = p_start;
        double covered = start + length;
        while (true) {
            Ref<TrackRouteSegment> segment;
            segment.instantiate();
            segment->set_track_rid(track);
            segment->set_distance(start);
            segment->set_length(length);
            segment->set_velocity(track_get_velocity(track));
            segment->set_track_switch(is_switch);
            segment->set_branch(branch);
            segment->set_branch_from_setting(branch_from_setting);
            segment->set_toward_end(movement_sign > 0.0);
            route.push_back(segment);
            if (covered >= p_distance) {
                break;
            }
            int endpoint_index = 0;
            if (is_switch) {
                endpoint_index = movement_sign > 0.0 ? switch_get_branch_end_endpoint(track, branch)
                                                     : switch_get_branch_start_endpoint(track, branch);
            } else {
                endpoint_index = movement_sign > 0.0 ? CURVE1_P2 : CURVE1_P1;
            }
            RID next_track;
            int next_endpoint = 0;
            int forced_switch_track = NO_FORCED_SWITCH_TRACK;
            if (!track_find_next(track, endpoint_index, next_track, next_endpoint, forced_switch_track)) {
                segment->set_line_end(true);
                break;
            }
            track = next_track;
            is_switch = track_is_switch(track);
            branch = TRACK_COMMON;
            branch_from_setting = false;
            if (is_switch) {
                // the branch it would be forced onto, or the one the node leads to - the setting
                branch_from_setting = forced_switch_track == NO_FORCED_SWITCH_TRACK;
                branch = branch_from_setting ? switch_get_endpoint_branch(track, next_endpoint) : forced_switch_track;
                movement_sign = next_endpoint == switch_get_branch_end_endpoint(track, branch) ? -1.0 : 1.0;
            } else {
                movement_sign = next_endpoint == CURVE1_P2 ? -1.0 : 1.0;
            }
            length = track_get_length(track, branch);
            start = covered;
            covered += length;
        }
        return route;
    }

    bool TrackServer::track_find_next(
            const RID &p_track, const int p_endpoint_index, RID &p_r_track, int &p_r_endpoint,
            int &p_r_forced_switch_track) {
        if (!track_exists(p_track)) {
            return false;
        }
        const TypedArray<TrackEndpointRef> connections = track_get_endpoint_connections(p_track, p_endpoint_index);

        bool has_unique = false;
        RID unique_track;
        int unique_endpoint = 0;
        int unique_forced_switch_track = TrackServer::TRACK_COMMON;
        bool has_unique_forced_switch_track = false;

        for (int index = 0; index < connections.size(); ++index) {
            const Ref<TrackEndpointRef> raw = connections[index];
            RID candidate_track = raw->get_track_rid();
            int candidate_endpoint = raw->get_endpoint_index();
            int candidate_forced_switch_track = TrackServer::TRACK_COMMON;
            bool has_candidate_forced_switch_track = false;

            if (track_is_switch(candidate_track)) {
                const PackedInt32Array common_endpoints = switch_get_common_endpoints(candidate_track);
                if (common_endpoints.has(raw->get_endpoint_index())) {
                    // Entering from the common point follows whichever branch is active; remap the
                    // shared endpoint to the active branch endpoint at the same physical point.
                    const int active_track = switch_get_active_track(candidate_track);
                    const int branch_start = switch_get_branch_start_endpoint(candidate_track, active_track);
                    candidate_endpoint = common_endpoints.has(branch_start)
                                                 ? branch_start
                                                 : switch_get_branch_end_endpoint(candidate_track, active_track);
                } else {
                    candidate_forced_switch_track =
                            switch_get_endpoint_branch(candidate_track, raw->get_endpoint_index());
                    has_candidate_forced_switch_track = true;
                }
            }

            // Discard endpoints that cannot be used for motion on the selected route. Branch-side
            // switch entry is allowed because it will force that branch.
            bool is_motion_accessible = false;
            if (track_is_switch(candidate_track)) {
                const int active_track = switch_get_active_track(candidate_track);
                is_motion_accessible =
                        candidate_endpoint == switch_get_branch_start_endpoint(candidate_track, active_track) ||
                        candidate_endpoint == switch_get_branch_end_endpoint(candidate_track, active_track);
            } else {
                is_motion_accessible =
                        candidate_endpoint == TrackServer::CURVE1_P1 || candidate_endpoint == TrackServer::CURVE1_P2;
            }
            if (!is_motion_accessible && !has_candidate_forced_switch_track) {
                continue;
            }

            if (!has_unique) {
                has_unique = true;
                unique_track = candidate_track;
                unique_endpoint = candidate_endpoint;
                unique_forced_switch_track = candidate_forced_switch_track;
                has_unique_forced_switch_track = has_candidate_forced_switch_track;
                continue;
            }

            // More than one different usable target means the node is ambiguous. Identical
            // candidates can happen at switch common points and still count as one route.
            const bool is_same_candidate =
                    unique_track == candidate_track && unique_endpoint == candidate_endpoint &&
                    has_unique_forced_switch_track == has_candidate_forced_switch_track &&
                    (!has_unique_forced_switch_track || unique_forced_switch_track == candidate_forced_switch_track);
            if (!is_same_candidate) {
                return false;
            }
        }

        if (!has_unique) {
            return false;
        }
        p_r_track = unique_track;
        p_r_endpoint = unique_endpoint;
        p_r_forced_switch_track = has_unique_forced_switch_track ? unique_forced_switch_track : NO_FORCED_SWITCH_TRACK;
        return true;
    }

    Ref<TrackEndpointRef> TrackServer::track_get_next(const RID &p_track, const int p_endpoint_index) {
        RID next_track;
        int next_endpoint = 0;
        int forced_switch_track = NO_FORCED_SWITCH_TRACK;
        if (!track_find_next(p_track, p_endpoint_index, next_track, next_endpoint, forced_switch_track)) {
            return Ref<TrackEndpointRef>();
        }
        Ref<TrackEndpointRef> next;
        next.instantiate();
        next->set_track_rid(next_track);
        next->set_endpoint_index(next_endpoint);
        return next;
    }

    bool TrackServer::track_is_switch(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr && track->type == TRACK_SWITCH;
    }

    bool TrackServer::switch_is_right(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        return track != nullptr && track->type == TRACK_SWITCH && track->switch_is_right;
    }

    int TrackServer::switch_get_active_track(const RID &p_track) const {
        const TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr) {
            UtilityFunctions::push_error("Track RID is not valid");
            return TRACK_COMMON;
        }
        return track->active_track;
    }

    String TrackServer::track_get_name(const RID &p_track) const {
        if (!tracks.has(p_track)) {
            UtilityFunctions::push_error("Track RID is not valid");
            return String();
        }
        for (const KeyValue<String, RID> &named: named_tracks) {
            if (named.value == p_track) {
                return named.key;
            }
        }
        return String();
    }

    // MaSzyna Track.cpp:1743-1755 sets desired switch offset.
    void TrackServer::switch_set_active_track(const RID &p_track, const int p_active_track) {
        TrackSegment *track = tracks.getptr(p_track);
        if (track == nullptr || track->active_track == p_active_track) {
            return;
        }
        const int from_track = track->active_track;
        track->active_track = p_active_track;
        track->switch_desired_offset = p_active_track == TRACK_DIVERGING
                                               ? SWITCH_MAX_OFFSET + track->switch_f_offset_delay
                                               : -track->switch_f_offset_delay;

        emit_signal(switch_active_track_changed_signal, p_track, track->active_track);
        emit_signal(switch_movement_started_signal, p_track, from_track, p_active_track);

        moving_switches.erase(p_track);

        if (track->switch_desired_offset == track->switch_f_offset) {
            _set_switch_f_offset(*track, track->switch_desired_offset);
            emit_signal(switch_movement_finished_signal, p_track, p_active_track);
            return;
        }
        moving_switches.push_back(p_track);
        last_switch_step_usec = Time::get_singleton()->get_ticks_usec();
        _set_switch_processing(true);
    }

    Dictionary TrackServer::topology_get_summary() {
        Dictionary stats;
        Array graphs;
        for (const KeyValue<int, Vector<RID>> &graph: graph_members) {
            int switch_count = 0;
            double total_length = 0.0;
            for (const RID &track_rid: graph.value) {
                const TrackSegment *track = tracks.getptr(track_rid);
                if (track == nullptr) {
                    continue;
                }
                if (track->type == TRACK_SWITCH) {
                    ++switch_count;
                }
                total_length += track_get_length(track_rid);
            }
            Dictionary entry;
            entry["id"] = graph.key;
            entry["num_tracks"] = static_cast<int>(graph.value.size());
            entry["num_switches"] = switch_count;
            entry["total_length"] = total_length;
            graphs.push_back(entry);
        }
        stats["graphs"] = graphs;

        int orphaned = 0;
        for (const KeyValue<RID, TrackSegment> &item: tracks) {
            bool is_orphaned = true;
            for (const int node_id: item.value.node_ids) {
                if (node_id == INVALID_NODE_ID) {
                    continue;
                }
                const TrackNode *node = _get_node(node_id);
                if (node != nullptr && node->endpoint_refs.size() > 1) {
                    is_orphaned = false;
                    break;
                }
            }
            if (is_orphaned) {
                ++orphaned;
            }
        }
        stats["orphaned_tracks_count"] = orphaned;
        return stats;
    }

    void TrackServer::topology_rebuild() {
        _clear_topology();
        _connect_all_tracks();
        _rebuild_graph_ids();
        topology_changed_flag = false;
        emit_signal(topology_rebuilt_signal);
        emit_signal(tracks_changed_signal);
    }

    void TrackServer::_rebuild_graph_ids() {
        graph_members.clear();
        HashMap<RID, bool> visited;
        for (const KeyValue<RID, TrackSegment> &item: tracks) {
            if (visited.has(item.key)) {
                continue;
            }
            const int graph_id = next_graph_id;
            ++next_graph_id;
            Vector<RID> queue;
            queue.push_back(item.key);
            while (!queue.is_empty()) {
                const RID current_rid = queue[0];
                queue.remove_at(0);
                if (visited.has(current_rid)) {
                    continue;
                }
                visited.insert(current_rid, true);
                TrackSegment *current = tracks.getptr(current_rid);
                if (current == nullptr) {
                    continue;
                }
                current->graph_id = graph_id;
                graph_members[graph_id].push_back(current_rid);

                for (const int node_id: current->node_ids) {
                    if (node_id == INVALID_NODE_ID) {
                        continue;
                    }
                    const TrackNode *node = _get_node(node_id);
                    if (node == nullptr) {
                        continue;
                    }
                    for (const EndpointPair &ref: node->endpoint_refs) {
                        if (ref.track_rid == current_rid || visited.has(ref.track_rid)) {
                            continue;
                        }
                        queue.push_back(ref.track_rid);
                    }
                }
            }
        }
    }

    int TrackServer::_get_or_create_node(
            const Vector3 &p_world_position, const RID &p_track_rid, const int p_endpoint_index) {
        const TrackSegment *track = tracks.getptr(p_track_rid);
        if (track == nullptr) {
            return INVALID_NODE_ID;
        }
        int node_id = INVALID_NODE_ID;
        for (const int existing_node_id: track->node_ids) {
            const TrackNode *existing = _get_node(existing_node_id);
            if (existing == nullptr) {
                continue;
            }
            if (_endpoints_equal(existing->world_position, p_world_position)) {
                node_id = existing->id;
                break;
            }
        }
        if (node_id == INVALID_NODE_ID) {
            node_id = static_cast<int>(nodes.size());
            TrackNode new_node;
            new_node.id = node_id;
            new_node.world_position = p_world_position;
            nodes.push_back(new_node);
        }
        EndpointPair pair;
        pair.track_rid = p_track_rid;
        pair.endpoint_index = p_endpoint_index;
        nodes.write[node_id].endpoint_refs.push_back(pair);
        return node_id;
    }

    TrackServer::TrackNode *TrackServer::_get_node(const int p_node_id) {
        if (p_node_id < 0 || p_node_id >= nodes.size() || nodes[p_node_id].merged_away) {
            return nullptr;
        }
        return &nodes.write[p_node_id];
    }

    void TrackServer::_clear_topology() {
        nodes.clear();
        graph_members.clear();
        for (KeyValue<RID, TrackSegment> &item: tracks) {
            item.value.graph_id = -1;
            for (int &node_id: item.value.node_ids) {
                node_id = INVALID_NODE_ID;
            }
        }
    }

    void TrackServer::_connect_all_tracks() {
        // 1. a node per endpoint of every track
        for (KeyValue<RID, TrackSegment> &item: tracks) {
            TrackSegment &track = item.value;
            for (int &node_id: track.node_ids) {
                node_id = INVALID_NODE_ID;
            }
            if (track.curve1.is_valid()) {
                track.node_ids[CURVE1_P1] = _get_or_create_node(track.points1.p1, track.track_rid, CURVE1_P1);
                track.node_ids[CURVE1_P2] = _get_or_create_node(track.points1.p2, track.track_rid, CURVE1_P2);
            }
            if (track.curve2.is_valid()) {
                track.node_ids[CURVE2_P1] = _get_or_create_node(track.points2.p1, track.track_rid, CURVE2_P1);
                track.node_ids[CURVE2_P2] = _get_or_create_node(track.points2.p2, track.track_rid, CURVE2_P2);
            }
        }

        // 2. collect the endpoints
        Vector<EndpointEntry> endpoints;
        for (const KeyValue<RID, TrackSegment> &item: tracks) {
            const TrackSegment &track = item.value;
            for (int endpoint_index = CURVE1_P1; endpoint_index <= CURVE2_P2; ++endpoint_index) {
                if (track.node_ids[endpoint_index] == INVALID_NODE_ID) {
                    continue;
                }
                EndpointEntry entry;
                entry.track_rid = track.track_rid;
                entry.endpoint_index = endpoint_index;
                entry.node_id = track.node_ids[endpoint_index];
                switch (endpoint_index) {
                    case CURVE1_P1:
                        entry.position = track.points1.p1;
                        break;
                    case CURVE1_P2:
                        entry.position = track.points1.p2;
                        break;
                    case CURVE2_P1:
                        entry.position = track.points2.p1;
                        break;
                    default:
                        entry.position = track.points2.p2;
                        break;
                }
                endpoints.push_back(entry);
            }
        }

        // Only endpoints in neighbouring cells can be within the connection tolerance.
        HashMap<Vector2i, Vector<int>> endpoint_cells;
        for (int index = 0; index < endpoints.size(); ++index) {
            const Vector3 &position = endpoints[index].position;
            const Vector2i cell(
                    static_cast<int>(Math::floor(position.x / ENDPOINT_CELL_SIZE)),
                    static_cast<int>(Math::floor(position.z / ENDPOINT_CELL_SIZE)));
            endpoint_cells[cell].push_back(index);
        }

        for (int i = 0; i < endpoints.size(); ++i) {
            const Vector3 position = endpoints[i].position;
            const Vector2i cell(
                    static_cast<int>(Math::floor(position.x / ENDPOINT_CELL_SIZE)),
                    static_cast<int>(Math::floor(position.z / ENDPOINT_CELL_SIZE)));
            Vector<int> candidates;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    const Vector<int> *cell_entries = endpoint_cells.getptr(cell + Vector2i(dx, dz));
                    if (cell_entries == nullptr) {
                        continue;
                    }
                    for (const int candidate: *cell_entries) {
                        if (candidate > i) {
                            candidates.push_back(candidate);
                        }
                    }
                }
            }
            // Preserve the merge order and node identifiers of the original pairwise traversal.
            candidates.sort();

            for (const int j: candidates) {
                if (endpoints[i].node_id == endpoints[j].node_id) {
                    continue;
                }
                const TrackSegment *first_track = tracks.getptr(endpoints[i].track_rid);
                const TrackSegment *second_track = tracks.getptr(endpoints[j].track_rid);
                if (first_track == nullptr || second_track == nullptr) {
                    continue;
                }
                const int first_group = _track_type_group(first_track->type);
                const int second_group = _track_type_group(second_track->type);
                if (first_group == GROUP_NONE || first_group != second_group) {
                    continue;
                }
                if (!_endpoints_equal(endpoints[i].position, endpoints[j].position)) {
                    continue;
                }
                _merge_endpoint_nodes(
                        endpoints[i].track_rid, endpoints[i].endpoint_index, endpoints[j].track_rid,
                        endpoints[j].endpoint_index);
                const TrackSegment *merged = tracks.getptr(endpoints[i].track_rid);
                if (merged != nullptr) {
                    endpoints.write[j].node_id = merged->node_ids[endpoints[i].endpoint_index];
                }
            }
        }
    }

    void TrackServer::_merge_endpoint_nodes(
            const RID &p_first_track, const int p_first_endpoint, const RID &p_second_track,
            const int p_second_endpoint) {
        const TrackSegment *first_track = tracks.getptr(p_first_track);
        const TrackSegment *second_track = tracks.getptr(p_second_track);
        if (first_track == nullptr || second_track == nullptr) {
            return;
        }
        const int first_node_id = first_track->node_ids[p_first_endpoint];
        const int second_node_id = second_track->node_ids[p_second_endpoint];
        if (first_node_id == second_node_id) {
            return;
        }
        if (_get_node(first_node_id) == nullptr || _get_node(second_node_id) == nullptr) {
            return;
        }
        const Vector<EndpointPair> moved = nodes[second_node_id].endpoint_refs;
        for (const EndpointPair &ref: moved) {
            nodes.write[first_node_id].endpoint_refs.push_back(ref);
            if (TrackSegment *ref_track = tracks.getptr(ref.track_rid); ref_track != nullptr) {
                ref_track->node_ids[ref.endpoint_index] = first_node_id;
            }
        }
        nodes.write[second_node_id].merged_away = true;
        nodes.write[second_node_id].endpoint_refs.clear();
    }

    void TrackServer::_mark_topology_changed() {
        topology_changed_flag = true;
        emit_signal(topology_changed_signal);
    }

    bool TrackServer::get_is_topology_changed() const {
        return topology_changed_flag;
    }
} // namespace godot
