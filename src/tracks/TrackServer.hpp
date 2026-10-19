#pragma once

#include "SpatialIndex.hpp"
#include "TrackEndpointRef.hpp"
#include "utils/RIDHash.hpp"

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
    /* Stores track geometry, switches and track topology, indexed by RID. A track is geometry;
     * a track of TRACK_SWITCH type gets a switch of its own - a RID of its own holding where the
     * switch lies and its blade movement - created and freed with the type (TTrack::SwitchExtension,
     * Track.cpp).
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

            /* track_find_next(): entering the switch does not force a branch */
            static constexpr int NO_FORCED_SWITCH_TRACK = -1;

            /* Identifies one of the stored track curve endpoints. */
            enum EndpointIndex {
                CURVE1_P1 = 0,
                CURVE1_P2 = 1,
                CURVE2_P1 = 2,
                CURVE2_P2 = 3,
            };

            /* A track's gauge [m] when it gives none */
            static constexpr double DEFAULT_TRACK_WIDTH = 1.435;
            /* TTrack::fFriction's default (Track.h:207) */
            static constexpr double DEFAULT_TRACK_FRICTION = 0.15;

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

            /* What surrounds a track, for its running sounds (TEnvironmentType, Track.h:36-44) */
            enum TrackEnvironment {
                ENVIRONMENT_UNKNOWN = -1,
                ENVIRONMENT_FLAT = 0,
                ENVIRONMENT_MOUNTAINS,
                ENVIRONMENT_CANYON,
                ENVIRONMENT_TUNNEL,
                ENVIRONMENT_BRIDGE,
                ENVIRONMENT_BANK,
            };

            /* Group of compatible track types for topology connections. */
            enum TrackTypeGroup {
                GROUP_NONE,
                GROUP_RAIL,
                GROUP_ROAD,
                GROUP_WATER,
            };

            /* A track's damage (TTrack::iDamageFlag, Track.cpp:452-458; dtrack_*, MOVER.h) - the bits a
             * vehicle on it takes notice of */
            enum TrackDamage {
                TRACK_DAMAGE_NONE = 0,
                /* rails taken away: a vehicle running onto it derails (dtrack_norail) */
                TRACK_DAMAGE_NO_RAIL = 128,
            };

            /* Movement direction along the current track. */
            enum Direction {
                DIRECTION_NORMAL = 0,
                DIRECTION_REVERSED = 1,
            };

            /* Invalid node index used by topology storage. */
            static constexpr int INVALID_NODE_ID = -1;

            static TrackServer *get_instance() {
                return singleton;
            }

        private:
            static TrackServer *singleton;

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
                    TrackType type = TRACK_NORMAL;
                    /* Its own name, also where named_tracks points it is found by (a duplicate
                     * keeps its name but is not found by it) */
                    String name;
                    /* The switch of a TRACK_SWITCH track, none for any other */
                    RID switch_rid;
                    Ref<Resource> curve1;
                    Ref<Resource> curve2;
                    CurvePoints points1;
                    CurvePoints points2;
                    Ref<Curve3D> domain_curve1;
                    Ref<Curve3D> domain_curve2;
                    double width = DEFAULT_TRACK_WIDTH;
                    int quality_flag = 0;
                    BitField<TrackDamage> damage_flag = TRACK_DAMAGE_NONE;
                    double friction = DEFAULT_TRACK_FRICTION;
                    /* A memory cell's modifiers of the friction (TTrack::m_friction, Track.cpp:2527-2533):
                     * friction * x + y, clamped to 0-1 - none while unset */
                    bool friction_modified = false;
                    Vector2 friction_modifier = Vector2(1.0, 0.0);
                    TrackEnvironment environment = ENVIRONMENT_FLAT;
                    double sound_distance = -1.0;
                    /* Speed limit in km/h, negative for none (TTrack::fVelocity, Track.cpp:851-858) */
                    double velocity = -1.0;
                    /* Vehicles on it by their primary axle, as RailVehicleServer reports them
                     * (TTrack::Dynamics - only the primary axle moves a vehicle between the lists,
                     * TrkFoll.cpp:194-241) */
                    Vector<RID> vehicles;
                    /* The axles on it - every axle of every vehicle, primary or not (TTrack::AxleCounter(),
                     * Track.h:302). What its isolated sections count; the vehicle list above is not. */
                    int axle_count = 0;
                    /* The isolated sections it belongs to (TTrack::Isolated) */
                    Vector<RID> isolated;
                    double length = 0.0;
                    double length1 = 0.0;
                    double length2 = 0.0;
                    int graph_id = -1;
                    SwitchCommonPoint switch_common_endpoint_index = POINT_NONE;
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

                    double get_length(SwitchTrack p_switch_track) const;
            };

            /* Where a switch lies and how far its blades have moved (TTrack::SwitchExtension) */
            struct SwitchData {
                    RID track;
                    SwitchTrack active_track = TRACK_COMMON;
                    double f_offset_delay = SWITCH_OFFSET_DELAY;
                    double desired_offset = -SWITCH_OFFSET_DELAY;
                    double f_offset = -SWITCH_OFFSET_DELAY;
                    double f_offset1 = -SWITCH_OFFSET_DELAY; // Track.cpp:55
                    double f_offset2 = 0.0;
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
                    /* TIsolated::iAxles - axles, not vehicles: two per vehicle */
                    int axle_count = 0;
            };

            RIDMap<TrackSegment> tracks;
            RIDMap<SwitchData> switches;
            /* The index of TrackSegment::name; a duplicate name points at the last track given it
             * (basic_table::insert(), Names.h:40-49) */
            HashMap<String, RID> named_tracks;
            RIDMap<IsolatedData> isolated_sections;
            HashMap<StringName, RID> isolated_by_name;
            Ref<SpatialIndex> spatial_index;
            int64_t next_track_id = 0;
            int next_graph_id = 0;
            HashMap<int, Vector<RID>> graph_members;
            Vector<TrackNode> nodes;
            /* The switches whose blade is moving - the tick is connected only while this is not
             * empty, and disconnected again when the last one arrives. */
            Vector<RID> moving_switches;

            /* An axle came onto a track or left it: the count TTrack::AxleCounter(+1/-1) adds
             * (TrkFoll.cpp:89-91) */
            static constexpr int AXLE_ENTERED = 1;
            static constexpr int AXLE_LEFT = -1;
            /* Adds p_delta axles to the track and to its sections */
            void _count_axle(const RID &p_track, int p_delta, const RID &p_vehicle);
            /* Adds p_delta axles to the section and its parents, reporting the change */
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

            void _set_switch_f_offset(const RID &p_switch, SwitchData &p_data, double p_value);
            /* Frees the track's switch, reporting it under the track's name */
            void _free_switch(TrackSegment &p_track);
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
            /* A track became a switch, a switch track became another type or was freed */
            static const char *switch_created_signal;
            static const char *switch_freed_signal;
            static const char *tracks_changed_signal;
            static const char *topology_rebuilt_signal;
            static const char *topology_changed_signal;
            /* The first axle came onto the isolated section, the last one left it, an axle came
             * onto it or left it (TIsolated::Modify(), `:busy`, `:free`, `:inc`, `:dec`) - with the
             * vehicle of that axle */
            static const char *isolated_occupied_signal;
            static const char *isolated_freed_signal;
            static const char *isolated_axle_entered_signal;
            static const char *isolated_axle_left_signal;

            /* The three values the callers used to read as GDScript constants. A C++ class cannot
             * expose a float constant, so they are read-only properties. */
            double get_switch_max_offset() const;
            double get_switch_offset_delay() const;
            double get_rail_height() const;

            RID track_create();
            void track_free(const RID &p_track);
            RID track_get_rid_by_name(const String &p_name) const;
            double track_get_length(const RID &p_track, SwitchTrack p_branch = TRACK_COMMON) const;
            void track_update_curves(const RID &p_track, const Ref<Resource> &p_curve1, const Ref<Resource> &p_curve2);
            /* Becoming TRACK_SWITCH creates the track's switch, leaving it frees the switch. An
             * empty name takes the track's name away. */
            void track_update(const RID &p_track, TrackType p_type, const String &p_name, double p_width);
            void track_update_properties(
                    const RID &p_track, int p_quality_flag, BitField<TrackDamage> p_damage_flag, double p_friction,
                    TrackEnvironment p_environment, double p_sound_distance);
            bool track_exists(const RID &p_track) const;
            TypedArray<RID> track_get_rids() const;
            double track_get_width(const RID &p_track) const;
            int track_get_quality_flag(const RID &p_track) const;
            BitField<TrackDamage> track_get_damage_flag(const RID &p_track) const;
            /* A derailer worked: the damage a scenario's switch event leaves on a plain track
             * (TTrack::Switch(), Track.cpp:1846-1847) */
            void track_set_damage_flag(const RID &p_track, BitField<TrackDamage> p_damage_flag);
            /* The friction, as its memory cell modifies it (TTrack::Friction(), Track.cpp:2527-2533) */
            double track_get_friction(const RID &p_track) const;
            /* The friction times p_multiplier plus p_offset, clamped to 0-1 - what the scenery's
             * `friction <memcell>` of a track makes of the cell's two values */
            void track_set_friction_modifier(const RID &p_track, double p_multiplier, double p_offset);
            /* What runs on it - rails, a road, water (TTrack::iCategoryFlag, Track.cpp:396-432) */
            TrackTypeGroup track_get_group(const RID &p_track) const;
            TrackEnvironment track_get_environment(const RID &p_track) const;
            double track_get_sound_distance(const RID &p_track) const;
            /* Speed limit in km/h, negative for none (TTrack::VelocitySet()) */
            void track_set_velocity(const RID &p_track, double p_velocity);
            /* An axle of the vehicle came onto the track or left it - reported by RailVehicleServer
             * for each of a vehicle's two axles, the new track first, so a section both tracks
             * belong to never reads empty (TTrackFollower::SetCurrentTrack(), TrkFoll.cpp:88-91).
             * The isolated sections count these. */
            void track_axle_entered(const RID &p_track, const RID &p_vehicle);
            void track_axle_left(const RID &p_track, const RID &p_vehicle);
            /* The vehicle's primary axle came onto the track or left it: the vehicle is listed on
             * the track it is on (TTrack::AddDynamicObject()/RemoveDynamicObject(),
             * TrkFoll.cpp:194-241) - what track_is_occupied() and track_get_vehicles() answer */
            void track_vehicle_entered(const RID &p_track, const RID &p_vehicle);
            void track_vehicle_left(const RID &p_track, const RID &p_vehicle);
            bool track_is_occupied(const RID &p_track) const;
            TypedArray<RID> track_get_vehicles(const RID &p_track) const;
            /* The same list for C++ only, without a copy - read by the neighbour scan of every
             * vehicle on every sub-step; empty for a track that is gone */
            const Vector<RID> &track_get_vehicle_list(const RID &p_track) const;

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
            SwitchCommonPoint track_get_common_endpoint_index(const RID &p_track) const;
            Ref<Resource> track_get_curve(const RID &p_track, SwitchTrack p_branch = TRACK_COMMON) const;
            /* The roll of the branch's curve at its start (x) and end (y) [deg], as read out of the
             * curve resource when it was set */
            Vector2 track_get_roll(const RID &p_track, SwitchTrack p_branch = TRACK_COMMON) const;
            Ref<Curve3D> track_get_domain_curve(const RID &p_track, SwitchTrack p_branch = TRACK_COMMON) const;
            /* The endpoints the two branches of a two-curve track share */
            PackedInt32Array track_get_common_endpoints(const RID &p_track) const;
            EndpointIndex track_get_branch_start_endpoint(const RID &p_track, SwitchTrack p_branch) const;
            EndpointIndex track_get_branch_end_endpoint(const RID &p_track, SwitchTrack p_branch) const;
            SwitchTrack track_get_endpoint_branch(const RID &p_track, EndpointIndex p_endpoint_index) const;
            /* How far along the branch its blade reaches (Track.cpp:1371) */
            double track_get_blade_boundary_offset(const RID &p_track, SwitchTrack p_branch) const;
            TypedArray<RID> tracks_find_in_aabb(const Rect2 &p_aabb) const;
            TypedArray<TrackEndpointRef>
            track_get_endpoint_connections(const RID &p_track, EndpointIndex p_endpoint_index);
            /* The tracks each end of the branch connects to */
            Ref<TrackBranchNeighbors> track_get_branch_neighbors(const RID &p_track, SwitchTrack p_branch);
            /* The track a movement leaving p_track at p_endpoint_index continues onto, and the
             * switch branch that entering it forces; false when the node is open or ambiguous */
            bool track_find_next(
                    const RID &p_track, int p_endpoint_index, RID &p_r_track, int &p_r_endpoint,
                    int &p_r_forced_switch_track);
            /* The tracks a movement goes over from p_track (on p_branch, toward its end or its start),
             * entered p_start [m] from where it is measured, until p_distance [m] from there; the
             * first is p_track itself, the last marked line_end when nothing follows it */
            TypedArray<TrackRouteSegment> track_trace_route(
                    const RID &p_track, SwitchTrack p_branch, bool p_branch_from_setting, bool p_toward_end,
                    double p_start, double p_distance);
            /* The endpoint of the track a movement continues onto, null when there is none */
            Ref<TrackEndpointRef> track_get_next(const RID &p_track, EndpointIndex p_endpoint_index);
            bool track_is_switch(const RID &p_track) const;
            /* The diverging branch of the switch track turns right */
            bool track_is_switch_right(const RID &p_track) const;
            String track_get_name(const RID &p_track) const;
            /* The switch of a TRACK_SWITCH track, none for any other */
            RID track_get_switch(const RID &p_track) const;

            RID switch_get_track(const RID &p_switch) const;
            /* The switch of the track of the name, none when there is no such switch track */
            RID switch_get_rid_by_name(const String &p_name) const;
            /* Its track's name */
            String switch_get_name(const RID &p_switch) const;
            SwitchTrack switch_get_active_track(const RID &p_switch) const;
            /* The blades move towards the track (Track.cpp:1743-1755) */
            void switch_set_active_track(const RID &p_switch, SwitchTrack p_active_track);
            double switch_get_f_offset1(const RID &p_switch) const;
            double switch_get_f_offset2(const RID &p_switch) const;
            Dictionary topology_get_summary();
            void topology_rebuild();

            bool get_is_topology_changed() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(TrackServer::SwitchTrack);
VARIANT_ENUM_CAST(TrackServer::SwitchCommonPoint);
VARIANT_ENUM_CAST(TrackServer::EndpointIndex);
VARIANT_ENUM_CAST(TrackServer::TrackType);
VARIANT_ENUM_CAST(TrackServer::TrackEnvironment);
VARIANT_BITFIELD_CAST(TrackServer::TrackDamage);
VARIANT_ENUM_CAST(TrackServer::TrackTypeGroup);
VARIANT_ENUM_CAST(TrackServer::Direction);
