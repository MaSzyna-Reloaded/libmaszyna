#pragma once

#include "SpatialIndex.hpp"
#include "TrackEndpointRef.hpp"

#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* Stores track geometry, switch state and track topology, indexed by RID.
     *
     * Ported from addons/libmaszyna/tracks/track_server.gd. The curve resource
     * (TrackCurve) stays a GDScript Resource because scenes serialize it, so its fields are
     * read through the Variant property API once, when the curves are set, and cached as plain
     * values from then on. */
    class TrackServer : public Object {
            GDCLASS(TrackServer, Object)

        public:
            /* Identifies branch of a switch track. */
            enum SwitchTrack {
                TRACK_COMMON,
                TRACK_DIVERGING,
            };

            /* Identifies which endpoint is shared by both switch curves. */
            enum SwitchCommonPoint {
                POINT_NONE = -1,
                POINT_P1 = 0,
                POINT_P2 = 1,
            };

            /* Identifies one of the stored track curve endpoints. */
            /* track_find_next(): entering the switch does not force a branch */
            static constexpr int NO_FORCED_SWITCH_TRACK = -1;

            enum EndpointIndex {
                CURVE1_P1 = 0,
                CURVE1_P2 = 1,
                CURVE2_P1 = 2,
                CURVE2_P2 = 3,
            };

            /* Describes the logical type of a track segment. */
            enum TrackType {
                TRACK_UNKNOWN = -1,
                TRACK_NORMAL = 0,
                TRACK_SWITCH,
                TRACK_ROAD,
                TRACK_CROSS,
                TRACK_RIVER,
                TRACK_TRIBUTARY,
                TRACK_TURN,
                TRACK_TABLE,
            };

            /* Group of compatible track types for topology connections. */
            enum TrackTypeGroup {
                GROUP_NONE,
                GROUP_RAIL,
                GROUP_ROAD,
                GROUP_WATER,
            };

            /* Movement direction along the current track. */
            enum Direction {
                DIRECTION_NORMAL = 0,
                DIRECTION_REVERSED = 1,
            };

            /* Invalid node index used by topology storage. */
            static constexpr int INVALID_NODE_ID = -1;

            static TrackServer *get_instance() {
                return Object::cast_to<TrackServer>(Engine::get_singleton()->get_singleton("TrackServer"));
            }

        private:
            static constexpr const char *CURVE_BAKE_INTERVAL_SETTING = "maszyna/scenery/track_curve_bake_interval";
            static constexpr double DEFAULT_CURVE_BAKE_INTERVAL = 10.0;
            /* Maximum switch blade offset (MaSzyna Track.cpp:33 fMaxOffset). */
            static constexpr double SWITCH_MAX_OFFSET = 0.1;
            /* Delay applied before switch blade movement starts (Track.h:68 fOffsetDelay). */
            static constexpr double SWITCH_OFFSET_DELAY = 0.05;
            /* Default rail height used by track sampling (Track.cpp:555 railheight). */
            static constexpr double RAIL_HEIGHT = 0.180;

            /* Tolerance for "these two endpoints are the same physical point", per axis - the
             * original engine's own Equal() (Track.cpp:2121). It has to stay this tight: inside a
             * double slip (rozjazd krzyzowy) the two branch ends of one switch are only ~0.2 m
             * apart, so a rounder tolerance merges them into one topology node and the crossing
             * becomes impassable. */
            static constexpr double ENDPOINT_EPSILON = 0.02;
            /* Cell size of the endpoint hash used while pairing endpoints - unrelated to the
             * tolerance above, it only has to be at least as large as it so the 3x3 neighbourhood
             * scan is complete. */
            static constexpr double ENDPOINT_CELL_SIZE = 0.5;
            static constexpr double GRID_CELL_SIZE = 500.0;
            /* Track.h:67 fOffsetSpeed - the blade's linear speed (m/s) */
            static constexpr double SWITCH_OFFSET_SPEED = 0.1;
            static constexpr int SWITCH_BLADE_SEGMENT_COUNT = 6;
            /* Track.cpp:1371 bladelength */
            static constexpr double SWITCH_BLADE_RATIO = 0.65;
            /* Step along both branches while looking for the frog, where they part (m). */
            static constexpr double FROG_SEARCH_STEP = 0.5;
            /* Margin added around a track's own extent before it is filed in the spatial index (m). */
            static constexpr double AABB_MARGIN = 5.0;
            /* A roll of this many degrees lifts the outer rail by sin(roll) * this (Segment.cpp:84). */
            static constexpr double ROLL_FIX_FACTOR = 0.75;
            /* Track.h:205 fTrackWidth */
            static constexpr double DEFAULT_TRACK_WIDTH = 1.435;

            /* Both endpoints of one curve, read out of the GDScript resource once. */
            struct CurvePoints {
                    Vector3 p1;
                    Vector3 c1;
                    Vector3 c2;
                    Vector3 p2;
                    double roll1 = 0.0;
                    double roll2 = 0.0;
            };

            struct TrackSegment {
                    RID track_rid;
                    int type = TRACK_NORMAL;
                    Ref<Resource> curve1;
                    Ref<Resource> curve2;
                    CurvePoints points1;
                    CurvePoints points2;
                    Ref<Curve3D> domain_curve1;
                    Ref<Curve3D> domain_curve2;
                    double width = DEFAULT_TRACK_WIDTH;
                    int quality_flag = 0;
                    int environment = 0;
                    double sound_distance = -1.0;
                    /* Speed limit in km/h, negative for none (TTrack::fVelocity, Track.cpp:851-858) */
                    double velocity = -1.0;
                    /* Vehicles on it, as RailVehicleServer reports them (TTrack::Dynamics) */
                    Vector<RID> vehicles;
                    /* The isolated sections it belongs to (TTrack::Isolated) */
                    Vector<RID> isolated;
                    double length = 0.0;
                    double length1 = 0.0;
                    double length2 = 0.0;
                    int graph_id = -1;
                    int active_track = TRACK_COMMON;
                    double switch_f_offset_delay = SWITCH_OFFSET_DELAY;
                    double switch_desired_offset = -SWITCH_OFFSET_DELAY;
                    double switch_f_offset = -SWITCH_OFFSET_DELAY;
                    double switch_f_offset1 = -SWITCH_OFFSET_DELAY; // Track.cpp:55
                    double switch_f_offset2 = 0.0;
                    int switch_common_endpoint_index = POINT_NONE;
                    PackedInt32Array switch_common_endpoints;
                    /* Indexed by SwitchTrack; -1 where the branch does not exist. */
                    int switch_branch_start_endpoints[2] = {-1, -1};
                    int switch_branch_end_endpoints[2] = {-1, -1};
                    /* Indexed by EndpointIndex; -1 where the endpoint has no branch. */
                    int switch_endpoint_branches[4] = {-1, -1, -1, -1};
                    double switch_blade_boundary_offsets[2] = {0.0, 0.0};
                    bool switch_is_right = false;
                    Rect2 aabb;
                    int node_ids[4] = {INVALID_NODE_ID, INVALID_NODE_ID, INVALID_NODE_ID, INVALID_NODE_ID};
                    PackedVector3Array cached_endpoints;

                    double get_length(int p_switch_track) const;
            };

            /* One (track, endpoint) pair. The Godot-visible TrackEndpointRef is built from this
             * only where GDScript asks for the connections. */
            struct EndpointPair {
                    RID track_rid;
                    int endpoint_index = 0;
            };

            struct TrackNode {
                    int id = INVALID_NODE_ID;
                    /* Merged into another node, so it answers for nothing any more */
                    bool merged_away = false;
                    Vector3 world_position;
                    Vector<EndpointPair> endpoint_refs;
            };

            /* One endpoint of one track, while the topology is being rebuilt. */
            struct EndpointEntry {
                    RID track_rid;
                    int endpoint_index = 0;
                    Vector3 position;
                    int node_id = INVALID_NODE_ID;
            };

            /* An isolated track section: a group of tracks whose occupancy is reported as one
             * (TIsolated, Track.cpp:90-170), inside an optional parent section (`area`) */
            struct IsolatedData {
                    StringName name;
                    RID parent;
                    Vector<RID> tracks;
                    int vehicle_count = 0;
            };

            HashMap<RID, TrackSegment> tracks;
            HashMap<String, RID> named_tracks;
            HashMap<RID, IsolatedData> isolated_sections;
            HashMap<StringName, RID> isolated_by_name;
            Ref<SpatialIndex> spatial_index;
            int64_t next_track_id = 0;
            int next_graph_id = 0;
            HashMap<int, Vector<RID>> graph_members;
            Vector<TrackNode> nodes;
            /* The switches whose blade is moving - the tick is connected only while this is not
             * empty, and disconnected again when the last one arrives. */
            Vector<RID> moving_switches;

            /* Adds p_delta vehicles to the section and its parents, reporting the change */
            void _count_isolated(const RID &p_isolated, int p_delta, const RID &p_vehicle);
            bool switch_processing = false;
            /* Timestamp of the previous blade step, the way E3DRenderingServer's smoke tick
             * measures its own delta - a SceneTree gives none. */
            uint64_t last_switch_step_usec = 0;
            double curve_bake_interval = DEFAULT_CURVE_BAKE_INTERVAL;
            bool topology_changed_flag = false;

            static bool _endpoints_equal(const Vector3 &p_first, const Vector3 &p_second);
            static int _track_type_group(int p_type);

            void _read_curve_points(const Ref<Resource> &p_curve, CurvePoints &p_points) const;
            Ref<Curve3D> _build_domain_curve(const CurvePoints &p_points) const;
            void _set_curves(TrackSegment &p_track, const Ref<Resource> &p_curve1, const Ref<Resource> &p_curve2);
            /// The tracks' curves are baked again at the interval the setting says now
            void _on_project_settings_changed();
            void _update_switch_blade_boundary_offsets(TrackSegment &p_track) const;
            double _switch_blade_boundary_offset(const Ref<Curve3D> &p_branch_curve, double p_frog_distance) const;
            void _append_common_switch_endpoint(
                    TrackSegment &p_track, const Vector3 &p_first, const Vector3 &p_second, int p_first_endpoint,
                    int p_second_endpoint) const;
            const PackedVector3Array &_endpoints(TrackSegment &p_track) const;

            void _set_switch_f_offset(TrackSegment &p_track, double p_value);
            void _set_switch_processing(bool p_processing);
            void _process_switches();

            int _get_or_create_node(const Vector3 &p_world_position, const RID &p_track_rid, int p_endpoint_index);
            TrackNode *_get_node(int p_node_id);
            void _clear_topology();
            void _connect_all_tracks();
            void _merge_endpoint_nodes(
                    const RID &p_first_track, int p_first_endpoint, const RID &p_second_track, int p_second_endpoint);
            void _rebuild_graph_ids();
            void _mark_topology_changed();

        protected:
            static void _bind_methods();

        public:
            TrackServer();
            ~TrackServer() override;

            static const char *switch_active_track_changed_signal;
            static const char *switch_offset_changed_signal;
            static const char *switch_movement_started_signal;
            static const char *switch_movement_finished_signal;
            static const char *tracks_changed_signal;
            static const char *topology_rebuilt_signal;
            static const char *topology_changed_signal;
            /* The first vehicle came onto the isolated section, the last one left it, a vehicle
             * came onto it or left it (TIsolated::Modify(), `:busy`, `:free`, `:inc`, `:dec`) */
            static const char *isolated_occupied_signal;
            static const char *isolated_freed_signal;
            static const char *isolated_vehicle_entered_signal;
            static const char *isolated_vehicle_left_signal;

            /* The three values the callers used to read as GDScript constants. A C++ class cannot
             * expose a float constant, so they are read-only properties. */
            double get_switch_max_offset() const;
            double get_switch_offset_delay() const;
            double get_rail_height() const;

            RID track_create();
            void track_free(const RID &p_track);
            RID track_get_rid_by_name(const String &p_name) const;
            double track_get_length(const RID &p_track, int p_switch_track = TRACK_COMMON) const;
            double switch_track_get_length(const RID &p_track, int p_switch_track) const;
            void track_update_curves(const RID &p_track, const Ref<Resource> &p_curve1, const Ref<Resource> &p_curve2);
            void track_update(const RID &p_track, int p_type, const String &p_name, double p_width);
            void
            track_update_properties(const RID &p_track, int p_quality_flag, int p_environment, double p_sound_distance);
            bool track_exists(const RID &p_track) const;
            TypedArray<RID> track_get_rids() const;
            double track_get_width(const RID &p_track) const;
            int track_get_quality_flag(const RID &p_track) const;
            int track_get_environment(const RID &p_track) const;
            double track_get_sound_distance(const RID &p_track) const;
            /* Speed limit in km/h, negative for none (TTrack::VelocitySet()) */
            void track_set_velocity(const RID &p_track, double p_velocity);
            /* A vehicle came onto the track or left it - reported by RailVehicleServer, the new
             * track first (TTrackFollower::SetCurrentTrack(), TrkFoll.cpp:88-91) */
            void track_vehicle_entered(const RID &p_track, const RID &p_vehicle);
            void track_vehicle_left(const RID &p_track, const RID &p_vehicle);
            bool track_is_occupied(const RID &p_track) const;
            TypedArray<RID> track_get_vehicles(const RID &p_track) const;

            RID isolated_create();
            void isolated_free(const RID &p_isolated);
            void isolated_set_name(const RID &p_isolated, const StringName &p_name);
            StringName isolated_get_name(const RID &p_isolated) const;
            RID isolated_get_rid_by_name(const StringName &p_name) const;
            void isolated_add_track(const RID &p_isolated, const RID &p_track);
            /* The section whose occupancy includes this one's (`area`) */
            void isolated_set_parent(const RID &p_isolated, const RID &p_parent);
            bool isolated_is_occupied(const RID &p_isolated) const;
            double track_get_velocity(const RID &p_track) const;
            PackedVector3Array track_get_endpoints(const RID &p_track);
            int track_get_common_endpoint_index(const RID &p_track) const;
            Ref<Resource> track_get_curve(const RID &p_track, int p_branch = TRACK_COMMON) const;
            /* The roll of the branch's curve at its start (x) and end (y) [deg], as read out of the
             * curve resource when it was set */
            Vector2 track_get_roll(const RID &p_track, int p_branch = TRACK_COMMON) const;
            Ref<Curve3D> track_get_domain_curve(const RID &p_track, int p_branch = TRACK_COMMON) const;
            PackedInt32Array switch_get_common_endpoints(const RID &p_track) const;
            int switch_get_branch_start_endpoint(const RID &p_track, int p_branch) const;
            int switch_get_branch_end_endpoint(const RID &p_track, int p_branch) const;
            int switch_get_endpoint_branch(const RID &p_track, int p_endpoint_index) const;
            double switch_get_blade_boundary_offset(const RID &p_track, int p_branch) const;
            double switch_get_f_offset1(const RID &p_track) const;
            double switch_get_f_offset2(const RID &p_track) const;
            TypedArray<RID> tracks_find_in_aabb(const Rect2 &p_aabb) const;
            TypedArray<TrackEndpointRef> track_get_endpoint_connections(const RID &p_track, int p_endpoint_index);
            Ref<TrackBranchNeighbors> switch_track_get_neighbors(const RID &p_track, int p_switch_track);
            /* The track a movement leaving p_track at p_endpoint_index continues onto, and the
             * switch branch that entering it forces; false when the node is open or ambiguous */
            bool track_find_next(
                    const RID &p_track, int p_endpoint_index, RID &p_r_track, int &p_r_endpoint,
                    int &p_r_forced_switch_track);
            /* The tracks a movement goes over from p_track (on p_branch, toward its end or its start),
             * entered p_start [m] from where it is measured, until p_distance [m] from there; the
             * first is p_track itself, the last marked line_end when nothing follows it */
            TypedArray<TrackRouteSegment> track_trace_route(
                    const RID &p_track, int p_branch, bool p_branch_from_setting, bool p_toward_end, double p_start,
                    double p_distance);
            /* The endpoint of the track a movement continues onto, null when there is none */
            Ref<TrackEndpointRef> track_get_next(const RID &p_track, int p_endpoint_index);
            bool track_is_switch(const RID &p_track) const;
            bool switch_is_right(const RID &p_track) const;
            int switch_get_active_track(const RID &p_track) const;
            String track_get_name(const RID &p_track) const;
            void switch_set_active_track(const RID &p_track, int p_active_track);
            Dictionary topology_get_summary();
            void topology_rebuild();

            bool get_is_topology_changed() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(TrackServer::SwitchTrack);
VARIANT_ENUM_CAST(TrackServer::SwitchCommonPoint);
VARIANT_ENUM_CAST(TrackServer::EndpointIndex);
VARIANT_ENUM_CAST(TrackServer::TrackType);
VARIANT_ENUM_CAST(TrackServer::TrackTypeGroup);
VARIANT_ENUM_CAST(TrackServer::Direction);
