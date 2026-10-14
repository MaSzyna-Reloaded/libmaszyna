#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* One endpoint of one registered track. Was TrackServer's own EndpointRef, an inner class of the
     * GDScript autoload - GDExtension has no equivalent, so it is a class of its own. */
    class TrackEndpointRef : public RefCounted {
            GDCLASS(TrackEndpointRef, RefCounted)

        private:
            RID track_rid;
            int endpoint_index = 0;

        protected:
            static void _bind_methods();

        public:
            void set_track_rid(const RID &p_track_rid);
            RID get_track_rid() const;
            void set_endpoint_index(int p_endpoint_index);
            int get_endpoint_index() const;
    };

    /* The tracks connected before and after one branch of a switch. Was TrackServer's own
     * BranchNeighbors. */
    class TrackBranchNeighbors : public RefCounted {
            GDCLASS(TrackBranchNeighbors, RefCounted)

        private:
            RID previous_track_rid;
            int previous_endpoint_index = 0;
            RID next_track_rid;
            int next_endpoint_index = 0;

        protected:
            static void _bind_methods();

        public:
            void set_previous_track_rid(const RID &p_track_rid);
            RID get_previous_track_rid() const;
            void set_previous_endpoint_index(int p_endpoint_index);
            int get_previous_endpoint_index() const;
            void set_next_track_rid(const RID &p_track_rid);
            RID get_next_track_rid() const;
            void set_next_endpoint_index(int p_endpoint_index);
            int get_next_endpoint_index() const;
    };

    /* One track of a traced route (TrackServer::track_trace_route()). */
    class TrackRouteSegment : public RefCounted {
            GDCLASS(TrackRouteSegment, RefCounted)

        private:
            /* the track */
            RID track_rid = RID();
            /* from the vehicle to where the track is entered [m], negative for the one it stands on */
            double distance = 0.0;
            /* [m] */
            double length = 0.0;
            /* its speed limit [km/h], -1 for none */
            double velocity = -1.0;
            /* a switch, passed as it is set */
            bool track_switch = false;
            /* the switch's branch the route takes */
            int branch = 0;
            /* the branch is the switch's setting (entered at its common end), not the one entered on:
             * throwing the switch changes the route from here */
            bool branch_from_setting = false;
            /* driven towards its end (event2), not its start (event1) */
            bool toward_end = false;
            /* nothing follows it */
            bool line_end = false;

        protected:
            static void _bind_methods();

        public:
            void set_track_rid(const RID &p_track_rid);
            RID get_track_rid() const;
            void set_distance(double p_distance);
            double get_distance() const;
            void set_length(double p_length);
            double get_length() const;
            void set_velocity(double p_velocity);
            double get_velocity() const;
            void set_track_switch(bool p_track_switch);
            bool get_track_switch() const;
            void set_branch(int p_branch);
            int get_branch() const;
            void set_branch_from_setting(bool p_branch_from_setting);
            bool get_branch_from_setting() const;
            void set_toward_end(bool p_toward_end);
            bool get_toward_end() const;
            void set_line_end(bool p_line_end);
            bool get_line_end() const;
    };
} // namespace godot
