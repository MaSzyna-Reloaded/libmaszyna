#pragma once
#include "ScenarioEventAction.hpp"
#include "ScenarioEventCondition.hpp"
#include "vehicles/rail/RailVehicleRadio.hpp"
#include <functional>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <queue>
#include <vector>

namespace godot {
    /// RID based registry and queue of the scenario's events, its memory and its launchers (the
    /// original's event_manager, simulation::Memory and TEventLauncher, Event.h:638, MemCell.h,
    /// EvLaunch.h).
    ///
    /// An event is a delay, an optional condition and an action (ScenarioEventAction); the server
    /// knows no event types - what `updatevalues` or `lights` means is the business of an action.
    /// A queued event runs once its time comes: its condition is tested, then the action's run()
    /// or run_else() is called. The time is SimulationServer's simulation time, in seconds, which
    /// the physics and the drivers read too.
    class ScenarioEventServer : public Object {
            GDCLASS(ScenarioEventServer, Object)

        public:
            /// The fields of a memory, as a mask (the original's `text`, `value1`, `value2`
            /// flags, Event.h:29-31)
            enum MemoryField {
                MEMORY_FIELD_TEXT = 1,
                MEMORY_FIELD_VALUE1 = 2,
                MEMORY_FIELD_VALUE2 = 4,
            };

            /// A track's events (TTrack m_events0..m_events2all, Track.cpp:952-954): 0 while a
            /// vehicle stands on it, 1 when one moves along it towards its start, 2 towards its
            /// end; the plain ones only for a vehicle with a driver, the `all` ones for any
            enum TrackEvent {
                TRACK_EVENT0,
                TRACK_EVENT1,
                TRACK_EVENT2,
                TRACK_EVENTALL0,
                TRACK_EVENTALL1,
                TRACK_EVENTALL2,
                TRACK_EVENT_MAX,
            };

            /// An isolated section's events (TIsolated::AssignEvents(), Track.cpp:110-116): the first
            /// vehicle came onto it, the last one left, a vehicle came onto it, a vehicle left it
            enum IsolatedEvent {
                ISOLATED_BUSY,
                ISOLATED_FREE,
                ISOLATED_INC,
                ISOLATED_DEC,
                ISOLATED_EVENT_MAX,
            };

            static const char *event_queued_signal;
            static const char *event_launched_signal;
            static const char *event_dequeued_signal;
            static const char *memory_values_changed_signal;

            static ScenarioEventServer *get_instance() {
                return Object::cast_to<ScenarioEventServer>(
                        Engine::get_singleton()->get_singleton("ScenarioEventServer"));
            }

        private:
            struct EventData {
                    StringName name;
                    double delay = 0.0;
                    double random_delay = 0.0;
                    /// NAN for none
                    double departure_delay = Math::NaN;
                    Ref<ScenarioEventAction> action;
                    Ref<ScenarioEventCondition> condition;
                    /// The sequence of its entry in the queue, 0 while it is not queued
                    uint64_t queued_sequence = 0;
                    /// ...and the time the entry runs at
                    double run_time = 0.0;
                    /// Never queued, only read (by drivers scanning the track ahead)
                    bool passive = false;
            };

            struct MemoryData {
                    StringName name;
                    String text;
                    double value1 = 0.0;
                    double value2 = 0.0;
                    RID track;
                    Vector3 position;
            };

            struct LauncherData {
                    StringName name;
                    RID event;
                    RID shift_event;
                    Ref<ScenarioEventCondition> condition;
                    Vector3 position;
                    double radius = -1.0;
                    Key key = KEY_NONE;
                    double interval = 0.0;
                    /// HH:MM it fires at, hour -1 for none; armed again once the hour is another
                    /// (iHour, iMinute, UpdatedTime - EvLaunch.cpp:197-211)
                    int hour = -1;
                    /// The radio call it answers, when it is on radio_launchers
                    RailVehicleRadio::RadioCall radio_call = RailVehicleRadio::RADIO_CALL1;
                    int minute = 0;
                    bool armed = true;
                    /// The sequence of its scheduled firing in the queue, 0 while none is
                    uint64_t scheduled_sequence = 0;
            };

            /// An event to run, or a launcher to fire, at a time; the sequence keeps entries of one
            /// time in the order they were queued
            struct QueueEntry {
                    double time = 0.0;
                    uint64_t sequence = 0;
                    RID owner;
                    RID activator;

                    bool operator>(const QueueEntry &p_other) const {
                        return time == p_other.time ? sequence > p_other.sequence : time > p_other.time;
                    }
            };

            struct TrackEvents {
                    Vector<RID> events[TRACK_EVENT_MAX];
            };

            struct IsolatedEvents {
                    Vector<RID> events[ISOLATED_EVENT_MAX];
            };

            /// Where a vehicle is, as the track signals of RailVehicleServer said
            struct VehicleOnTrack {
                    RID track;
                    bool standing = false;
                    /// The event1 and event2 of the track were fired since the vehicle came on it -
                    /// once per entry (TTrackFollower::iEventFlag, TrkFoll.cpp:136-161, 199)
                    bool fired_to_start = false;
                    bool fired_to_end = false;
            };

            HashMap<RID, EventData> events;
            HashMap<RID, MemoryData> memories;
            HashMap<RID, LauncherData> launchers;
            /// The launchers that fire at a time of day
            Vector<RID> timed_launchers;
            /// ...and the ones a radio call fires (m_radiodrivenlaunchers, Event.h)
            Vector<RID> radio_launchers;
            HashMap<RID, TrackEvents> track_events;
            HashMap<RID, IsolatedEvents> isolated_events;
            /// The `<model>.<submodel>:done` events, by E3DRenderingServer instance and submodel
            HashMap<RID, HashMap<String, RID>> animation_done_events;
            HashMap<RID, VehicleOnTrack> vehicles_on_tracks;
            HashMap<StringName, RID> events_by_name;
            HashMap<StringName, RID> memories_by_name;
            HashMap<StringName, RID> launchers_by_name;
            std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
            uint64_t next_sequence = 1;
            /// While something is queued it holds the runtime's clock and runs as it advances
            bool processing = false;

            void _on_time_of_day_changed();
            /* The launchers of the clock's HH:MM fire, those of another hour are armed again */
            void _check_timed_launchers();
            /* A launcher of a radius is in reach of the camera (basic_cell::update_events(),
             * scene.cpp:126-139); a negative radius is everywhere */
            static bool _in_reach(const LauncherData &p_launcher);
            void _on_vehicle_radio_called(
                    const RID &p_vehicle, RailVehicleRadio::RadioCall p_call, const Vector3 &p_position);
            void _on_vehicle_heading_to_track_start(const RID &p_vehicle, const RID &p_track);
            void _on_vehicle_heading_to_track_end(const RID &p_vehicle, const RID &p_track);
            void _on_vehicle_stopped_on_track(const RID &p_vehicle, const RID &p_track);
            void _on_vehicle_freed(const RID &p_vehicle);
            void _on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_freed(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_vehicle_entered(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_vehicle_left(const RID &p_isolated, const RID &p_vehicle);
            void _on_instance_submodel_animation_finished(const RID &p_instance, const String &p_submodel);
            void _on_instance_freed(const RID &p_instance);
            void _queue_isolated_events(const RID &p_isolated, IsolatedEvent p_slot, const RID &p_vehicle);
            /// The vehicle on the track, its flags cleared when the track is a new one
            VehicleOnTrack &_place_vehicle(const RID &p_vehicle, const RID &p_track);
            /// Queues the track's events of the slot - the crew slot only for a vehicle with a driver
            void _queue_track_events(
                    const RID &p_track, TrackEvent p_crew_slot, TrackEvent p_all_slot, const RID &p_vehicle);
            void _set_processing(bool p_processing);
            void _process_queue(double p_seconds);
            /// Puts the owner (an event or a launcher) in the queue, returns the entry's sequence
            uint64_t _schedule(const RID &p_owner, double p_time, const RID &p_activator);
            /* Queues the event when the condition passes; whether it did */
            bool _fire(Ref<ScenarioEventCondition> p_condition, RID p_event);

        protected:
            static void _bind_methods();

        public:
            ScenarioEventServer();
            ~ScenarioEventServer() override;

            RID event_create();
            void event_free(const RID &p_event);
            void event_set_name(const RID &p_event, const StringName &p_name);
            StringName event_get_name(const RID &p_event) const;
            RID event_get_rid_by_name(const StringName &p_name) const;
            void event_set_delay(const RID &p_event, double p_seconds);
            double event_get_delay(const RID &p_event) const;
            /// Up to this many seconds more, drawn each time the event is queued
            void event_set_random_delay(const RID &p_event, double p_seconds);
            double event_get_random_delay(const RID &p_event) const;
            /// Queued by a vehicle, the event runs this many seconds from the departure of the
            /// vehicle's train by its timetable instead of from now (`departuredelay`,
            /// Event.cpp:2431-2441); NAN for an event that does not wait for a departure
            void event_set_departure_delay(const RID &p_event, double p_seconds);
            double event_get_departure_delay(const RID &p_event) const;
            /// A passive event is never queued - nothing runs it; drivers read it on the tracks
            /// ahead (the original's m_passive, Event.cpp:2380-2384)
            void event_set_passive(const RID &p_event, bool p_passive);
            bool event_is_passive(const RID &p_event) const;
            void event_attach_action(const RID &p_event, const Ref<ScenarioEventAction> &p_action);
            Ref<ScenarioEventAction> event_get_action(const RID &p_event) const;
            void event_attach_condition(const RID &p_event, const Ref<ScenarioEventCondition> &p_condition);
            Ref<ScenarioEventCondition> event_get_condition(const RID &p_event) const;
            /// Queues the event to run after its delay plus p_extra_delay. An event is queued once:
            /// while it waits, another request is refused and its activator is dropped
            /// (event_manager::AddToQuery, Event.cpp:2380-2462). A passive event is refused.
            bool event_queue(const RID &p_event, const RID &p_activator = RID(), double p_extra_delay = 0.0);
            bool event_is_queued(const RID &p_event) const;
            /// The simulation time (SimulationServer.simulation_get_time()) a queued event runs at,
            /// negative when it is not queued
            double event_get_run_time(const RID &p_event) const;
            /// The queued events, in the order they run
            TypedArray<RID> queue_get_events() const;

            RID memory_create();
            void memory_free(const RID &p_memory);
            void memory_set_name(const RID &p_memory, const StringName &p_name);
            StringName memory_get_name(const RID &p_memory) const;
            RID memory_get_rid_by_name(const StringName &p_name) const;
            void memory_set_values(const RID &p_memory, const String &p_text, double p_value1, double p_value2);
            String memory_get_text(const RID &p_memory) const;
            double memory_get_value1(const RID &p_memory) const;
            double memory_get_value2(const RID &p_memory) const;
            /// The track the memory stands at (TMemCell::Track, MemCell.h:76)
            void memory_set_track(const RID &p_memory, const RID &p_track);
            RID memory_get_track(const RID &p_memory) const;
            /// Where the memory stands - sent with its command to the drivers it reaches
            void memory_set_position(const RID &p_memory, const Vector3 &p_position);
            Vector3 memory_get_position(const RID &p_memory) const;

            /// Fires the event when a vehicle does what the slot says on the track
            void track_add_event(const RID &p_track, TrackEvent p_slot, const RID &p_event);
            TypedArray<RID> track_get_events(const RID &p_track, TrackEvent p_slot) const;
            void track_clear_events(const RID &p_track);

            /// Fires the event when the isolated section (TrackServer) does what the slot says
            void isolated_add_event(const RID &p_isolated, IsolatedEvent p_slot, const RID &p_event);
            void isolated_clear_events(const RID &p_isolated);

            /// Fires the event when the submodel of the E3DRenderingServer instance finishes an
            /// animation (TAnimContainer::EventAssign(), Event.cpp:1588-1592)
            void animation_set_done_event(const RID &p_instance, const String &p_submodel, const RID &p_event);

            RID launcher_create();
            void launcher_free(const RID &p_launcher);
            void launcher_set_name(const RID &p_launcher, const StringName &p_name);
            StringName launcher_get_name(const RID &p_launcher) const;
            RID launcher_get_rid_by_name(const StringName &p_name) const;
            /// The event it fires, and the one it fires with Shift (Event1, Event2, EvLaunch.h)
            void launcher_set_events(const RID &p_launcher, const RID &p_event, const RID &p_shift_event);
            RID launcher_get_event(const RID &p_launcher) const;
            RID launcher_get_shift_event(const RID &p_launcher) const;
            void launcher_attach_condition(const RID &p_launcher, const Ref<ScenarioEventCondition> &p_condition);
            void launcher_set_position(const RID &p_launcher, const Vector3 &p_position);
            Vector3 launcher_get_position(const RID &p_launcher) const;
            /// How close its user has to be, negative for anywhere
            void launcher_set_radius(const RID &p_launcher, double p_radius);
            double launcher_get_radius(const RID &p_launcher) const;
            void launcher_set_key(const RID &p_launcher, Key p_key);
            Key launcher_get_key(const RID &p_launcher) const;
            /// Fires every p_seconds of simulated time, 0 for never
            void launcher_set_interval(const RID &p_launcher, double p_seconds);
            /// Fires when the clock shows p_hour:p_minute, every day; p_hour -1 for never
            void launcher_set_time_of_day(const RID &p_launcher, int p_hour, int p_minute);
            /// Fires when a vehicle's radio sends the call within the radius
            /// (event_manager::queue_receivers(), Event.cpp:2255-2268)
            void launcher_set_radio_call(const RID &p_launcher, RailVehicleRadio::RadioCall p_call);
            TypedArray<RID> launcher_get_rids() const;
            /// Queues the launcher's event if its condition passes
            void launcher_fire(const RID &p_launcher);
            /// Queues the launcher's Shift event if its condition passes
            void launcher_fire_shift(const RID &p_launcher);
    };
} // namespace godot

VARIANT_BITFIELD_CAST(ScenarioEventServer::MemoryField)
VARIANT_ENUM_CAST(ScenarioEventServer::TrackEvent)
VARIANT_ENUM_CAST(ScenarioEventServer::IsolatedEvent)
