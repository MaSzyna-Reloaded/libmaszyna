#include "TrackEndpointRef.hpp"

namespace godot {
    void TrackEndpointRef::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_track_rid", "track_rid"), &TrackEndpointRef::set_track_rid);
        ClassDB::bind_method(D_METHOD("get_track_rid"), &TrackEndpointRef::get_track_rid);
        ADD_PROPERTY(PropertyInfo(Variant::RID, "track_rid"), "set_track_rid", "get_track_rid");
        ClassDB::bind_method(D_METHOD("set_endpoint_index", "endpoint_index"), &TrackEndpointRef::set_endpoint_index);
        ClassDB::bind_method(D_METHOD("get_endpoint_index"), &TrackEndpointRef::get_endpoint_index);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "endpoint_index"), "set_endpoint_index", "get_endpoint_index");
    }

    void TrackEndpointRef::set_track_rid(const RID &p_track_rid) {
        track_rid = p_track_rid;
    }

    RID TrackEndpointRef::get_track_rid() const {
        return track_rid;
    }

    void TrackEndpointRef::set_endpoint_index(const int p_endpoint_index) {
        endpoint_index = p_endpoint_index;
    }

    int TrackEndpointRef::get_endpoint_index() const {
        return endpoint_index;
    }

    void TrackBranchNeighbors::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_previous_track_rid", "track_rid"), &TrackBranchNeighbors::set_previous_track_rid);
        ClassDB::bind_method(D_METHOD("get_previous_track_rid"), &TrackBranchNeighbors::get_previous_track_rid);
        ADD_PROPERTY(
                PropertyInfo(Variant::RID, "previous_track_rid"), "set_previous_track_rid", "get_previous_track_rid");
        ClassDB::bind_method(
                D_METHOD("set_previous_endpoint_index", "endpoint_index"),
                &TrackBranchNeighbors::set_previous_endpoint_index);
        ClassDB::bind_method(
                D_METHOD("get_previous_endpoint_index"), &TrackBranchNeighbors::get_previous_endpoint_index);
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "previous_endpoint_index"), "set_previous_endpoint_index",
                "get_previous_endpoint_index");
        ClassDB::bind_method(D_METHOD("set_next_track_rid", "track_rid"), &TrackBranchNeighbors::set_next_track_rid);
        ClassDB::bind_method(D_METHOD("get_next_track_rid"), &TrackBranchNeighbors::get_next_track_rid);
        ADD_PROPERTY(PropertyInfo(Variant::RID, "next_track_rid"), "set_next_track_rid", "get_next_track_rid");
        ClassDB::bind_method(
                D_METHOD("set_next_endpoint_index", "endpoint_index"), &TrackBranchNeighbors::set_next_endpoint_index);
        ClassDB::bind_method(D_METHOD("get_next_endpoint_index"), &TrackBranchNeighbors::get_next_endpoint_index);
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "next_endpoint_index"), "set_next_endpoint_index",
                "get_next_endpoint_index");
    }

    void TrackBranchNeighbors::set_previous_track_rid(const RID &p_track_rid) {
        previous_track_rid = p_track_rid;
    }

    RID TrackBranchNeighbors::get_previous_track_rid() const {
        return previous_track_rid;
    }

    void TrackBranchNeighbors::set_previous_endpoint_index(const int p_endpoint_index) {
        previous_endpoint_index = p_endpoint_index;
    }

    int TrackBranchNeighbors::get_previous_endpoint_index() const {
        return previous_endpoint_index;
    }

    void TrackBranchNeighbors::set_next_track_rid(const RID &p_track_rid) {
        next_track_rid = p_track_rid;
    }

    RID TrackBranchNeighbors::get_next_track_rid() const {
        return next_track_rid;
    }

    void TrackBranchNeighbors::set_next_endpoint_index(const int p_endpoint_index) {
        next_endpoint_index = p_endpoint_index;
    }

    int TrackBranchNeighbors::get_next_endpoint_index() const {
        return next_endpoint_index;
    }

    void TrackRouteSegment::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_track_rid", "track_rid"), &TrackRouteSegment::set_track_rid);
        ClassDB::bind_method(D_METHOD("get_track_rid"), &TrackRouteSegment::get_track_rid);
        ADD_PROPERTY(PropertyInfo(Variant::RID, "track_rid"), "set_track_rid", "get_track_rid");
        ClassDB::bind_method(D_METHOD("set_distance", "distance"), &TrackRouteSegment::set_distance);
        ClassDB::bind_method(D_METHOD("get_distance"), &TrackRouteSegment::get_distance);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance"), "set_distance", "get_distance");
        ClassDB::bind_method(D_METHOD("set_length", "length"), &TrackRouteSegment::set_length);
        ClassDB::bind_method(D_METHOD("get_length"), &TrackRouteSegment::get_length);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "length"), "set_length", "get_length");
        ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &TrackRouteSegment::set_velocity);
        ClassDB::bind_method(D_METHOD("get_velocity"), &TrackRouteSegment::get_velocity);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity"), "set_velocity", "get_velocity");
        ClassDB::bind_method(D_METHOD("set_track_switch", "track_switch"), &TrackRouteSegment::set_track_switch);
        ClassDB::bind_method(D_METHOD("get_track_switch"), &TrackRouteSegment::get_track_switch);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "track_switch"), "set_track_switch", "get_track_switch");
        ClassDB::bind_method(D_METHOD("set_branch", "branch"), &TrackRouteSegment::set_branch);
        ClassDB::bind_method(D_METHOD("get_branch"), &TrackRouteSegment::get_branch);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "branch"), "set_branch", "get_branch");
        ClassDB::bind_method(
                D_METHOD("set_branch_from_setting", "branch_from_setting"),
                &TrackRouteSegment::set_branch_from_setting);
        ClassDB::bind_method(D_METHOD("get_branch_from_setting"), &TrackRouteSegment::get_branch_from_setting);
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "branch_from_setting"), "set_branch_from_setting",
                "get_branch_from_setting");
        ClassDB::bind_method(D_METHOD("set_toward_end", "toward_end"), &TrackRouteSegment::set_toward_end);
        ClassDB::bind_method(D_METHOD("get_toward_end"), &TrackRouteSegment::get_toward_end);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "toward_end"), "set_toward_end", "get_toward_end");
        ClassDB::bind_method(D_METHOD("set_line_end", "line_end"), &TrackRouteSegment::set_line_end);
        ClassDB::bind_method(D_METHOD("get_line_end"), &TrackRouteSegment::get_line_end);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "line_end"), "set_line_end", "get_line_end");
    }

    void TrackRouteSegment::set_track_rid(const RID &p_track_rid) {
        track_rid = p_track_rid;
    }

    RID TrackRouteSegment::get_track_rid() const {
        return track_rid;
    }

    void TrackRouteSegment::set_distance(const double p_distance) {
        distance = p_distance;
    }

    double TrackRouteSegment::get_distance() const {
        return distance;
    }

    void TrackRouteSegment::set_length(const double p_length) {
        length = p_length;
    }

    double TrackRouteSegment::get_length() const {
        return length;
    }

    void TrackRouteSegment::set_velocity(const double p_velocity) {
        velocity = p_velocity;
    }

    double TrackRouteSegment::get_velocity() const {
        return velocity;
    }

    void TrackRouteSegment::set_track_switch(const bool p_track_switch) {
        track_switch = p_track_switch;
    }

    bool TrackRouteSegment::get_track_switch() const {
        return track_switch;
    }

    void TrackRouteSegment::set_branch(const int p_branch) {
        branch = p_branch;
    }

    int TrackRouteSegment::get_branch() const {
        return branch;
    }

    void TrackRouteSegment::set_branch_from_setting(const bool p_branch_from_setting) {
        branch_from_setting = p_branch_from_setting;
    }

    bool TrackRouteSegment::get_branch_from_setting() const {
        return branch_from_setting;
    }

    void TrackRouteSegment::set_toward_end(const bool p_toward_end) {
        toward_end = p_toward_end;
    }

    bool TrackRouteSegment::get_toward_end() const {
        return toward_end;
    }

    void TrackRouteSegment::set_line_end(const bool p_line_end) {
        line_end = p_line_end;
    }

    bool TrackRouteSegment::get_line_end() const {
        return line_end;
    }
} // namespace godot
