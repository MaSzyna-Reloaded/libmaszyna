#pragma once
#include "maszyna/scenario/MaszynaScenarioEventImplementation.hpp"
#include "scenario/ScenarioEventCondition.hpp"
#include "tracks/TrackServer.hpp"
#include "utils/RIDHash.hpp"
#include "vehicles/rail/RailVehicleRadio.hpp"
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's event system on ScenarioEventServer's dispatcher: its memory cells, its
    /// launchers (simulation::Memory, TEventLauncher, MemCell.h, EvLaunch.h), the events of its
    /// tracks and isolated sections, and what an event of it is besides its delay, condition and
    /// action - waiting for its train's departure, or passive. Everything belongs to an event system
    /// (ScenarioEventServer::system_create()) whose implementation is this server's
    /// (implementation_get()), and goes when the implementation is detached from the system.
    class MaszynaScenarioEventServer : public Object {
            GDCLASS(MaszynaScenarioEventServer, Object)

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

            static const char *memory_values_changed_signal;

        private:
            static MaszynaScenarioEventServer *singleton;

            struct MemoryData {
                    RID system;
                    StringName name;
                    String text;
                    double value1 = 0.0;
                    double value2 = 0.0;
                    RID track;
                    Vector3 position;
                    /* The tracks whose friction it modifies (a track's `friction <memcell>`) */
                    Vector<RID> friction_tracks;
            };

            struct LauncherData {
                    RID system;
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
                    /// The ticket of its scheduled firing (ScenarioEventServer::system_schedule()),
                    /// 0 while none is
                    int64_t scheduled_ticket = 0;
            };

            struct TrackEvents {
                    Vector<RID> events[TRACK_EVENT_MAX];
            };

            struct IsolatedEvents {
                    Vector<RID> events[ISOLATED_EVENT_MAX];
            };

            /// Where a vehicle is, as TrackServer's track signals said
            struct VehicleOnTrack {
                    RID track;
                    bool standing = false;
                    /// The track the other axle of the standing vehicle is on, where it is another
                    /// one: its event0 goes on too (TrkFoll.cpp:122-133 runs for every axle)
                    RID secondary_track;
                    /// The event1 and event2 of the track were fired since the vehicle came on it -
                    /// once per entry (TTrackFollower::iEventFlag, TrkFoll.cpp:136-161, 199)
                    bool fired_to_start = false;
                    bool fired_to_end = false;
            };

            /// What the systems own besides their events, to let go of with them
            struct SystemData {
                    Vector<RID> memories;
                    Vector<RID> launchers;
                    Vector<RID> tracks;
                    Vector<RID> isolated;
            };

            Ref<MaszynaScenarioEventImplementation> implementation;
            RIDMap<SystemData> systems;
            RIDMap<MemoryData> memories;
            RIDMap<LauncherData> launchers;
            /// The launchers that fire at a time of day
            Vector<RID> timed_launchers;
            /// ...and the ones a radio call fires (m_radiodrivenlaunchers, Event.h)
            Vector<RID> radio_launchers;
            RIDMap<TrackEvents> track_events;
            RIDMap<IsolatedEvents> isolated_events;
            RIDMap<VehicleOnTrack> vehicles_on_tracks;
            HashMap<StringName, RID> memories_by_name;
            HashMap<StringName, RID> launchers_by_name;
            /// The events waiting for their train's departure, by the seconds after it
            RIDMap<double> departure_delays;
            RIDSet passive_events;

            void _on_time_of_day_changed();
            /* The launchers of the clock's HH:MM fire, those of another hour are armed again */
            void _check_timed_launchers();
            /* A launcher of a radius is in reach of the camera (basic_cell::update_events(),
             * scene.cpp:126-139); a negative radius is everywhere */
            static bool _in_reach(const LauncherData &p_launcher);
            void _on_radio_transmitted(
                    int64_t p_kind, int64_t p_channel, const Variant &p_payload, const Vector3 &p_position,
                    double p_reach);
            /// A vehicle came onto the track, or started off on it, travelling as p_direction says:
            /// the track's event1 towards its start, event2 towards its end - a moving vehicle's only
            /// (TrkFoll.cpp:104, 136-161)
            void _on_track_vehicle_travelling(
                    const RID &p_track, const RID &p_vehicle, TrackServer::TravelDirection p_direction);
            /// A vehicle stopped on the track: its event0, of the vehicle's own track - the one it
            /// is listed on - or, reported after it, of the track its other axle stands on
            /// (TrkFoll.cpp:122-133)
            void _on_track_vehicle_stopped(const RID &p_track, const RID &p_vehicle);
            void _on_vehicle_freed(const RID &p_vehicle);
            void _on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_freed(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_axle_entered(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_axle_left(const RID &p_isolated, const RID &p_vehicle);
            void _on_event_freed(const RID &p_event);
            void _queue_isolated_events(const RID &p_isolated, IsolatedEvent p_slot, const RID &p_vehicle);
            /// The vehicle on the track, its flags cleared when the track is a new one
            VehicleOnTrack &_place_vehicle(const RID &p_vehicle, const RID &p_track);
            /// Queues the track's events of the slot - the crew slot only for a vehicle with a driver
            void _queue_track_events(
                    const RID &p_track, TrackEvent p_crew_slot, TrackEvent p_all_slot, const RID &p_vehicle);
            /* Queues the event when the condition passes; whether it did */
            static bool _fire(Ref<ScenarioEventCondition> p_condition, RID p_event);
            /* The system an event belongs to, its own data made on first use */
            SystemData *_get_event_system(const RID &p_event);

        protected:
            static void _bind_methods();

        public:
            static MaszynaScenarioEventServer *get_instance();

            MaszynaScenarioEventServer();
            ~MaszynaScenarioEventServer() override;

            /// The implementation to attach to a MaSzyna scenery's event system
            /// (ScenarioEventServer::system_attach_implementation())
            Ref<MaszynaScenarioEventImplementation> implementation_get() const;

            // What MaszynaScenarioEventImplementation forwards of a system

            /// Everything of the system goes: its memories, launchers, track and isolated events
            void system_release(const RID &p_system);
            /// A passive event is never queued
            bool event_can_queue(const RID &p_event) const;
            /// An event that waits for its train's departure runs that long after it
            double
            event_get_run_time(const RID &p_event, const RID &p_activator, double p_now, double p_run_time) const;
            /// A track's event0 goes on while its vehicle stands there
            void event_ran(const RID &p_event, const RID &p_activator);
            /// A launcher's interval came
            void scheduled(const RID &p_owner, int64_t p_ticket);

            /// Queued by a vehicle, the event runs this many seconds from the departure of the
            /// vehicle's train by its timetable instead of from now (`departuredelay`,
            /// Event.cpp:2431-2441); NAN for an event that does not wait for a departure
            void event_set_departure_delay(const RID &p_event, double p_seconds);
            double event_get_departure_delay(const RID &p_event) const;
            /// A passive event is never queued - nothing runs it; drivers read it on the tracks
            /// ahead (the original's m_passive, Event.cpp:2380-2384)
            void event_set_passive(const RID &p_event, bool p_passive);
            bool event_is_passive(const RID &p_event) const;

            /// A memory of the event system
            RID memory_create(const RID &p_system);
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
            /* The memory modifies the track's friction by its values from now on: friction * value1 +
             * value2 (a track's `friction <memcell>`, Track.cpp:924, 2527-2533, 3526) */
            void memory_add_friction_track(const RID &p_memory, const RID &p_track);
            RID memory_get_track(const RID &p_memory) const;
            /// Where the memory stands - sent with its command to the drivers it reaches
            void memory_set_position(const RID &p_memory, const Vector3 &p_position);
            Vector3 memory_get_position(const RID &p_memory) const;

            /// Fires the event when a vehicle does what the slot says on the track; the binding
            /// goes with the event's system
            void track_add_event(const RID &p_track, TrackEvent p_slot, const RID &p_event);
            TypedArray<RID> track_get_events(const RID &p_track, TrackEvent p_slot) const;

            /// Fires the event when the isolated section (TrackServer) does what the slot says
            void isolated_add_event(const RID &p_isolated, IsolatedEvent p_slot, const RID &p_event);

            /// A launcher of the event system
            RID launcher_create(const RID &p_system);
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

VARIANT_BITFIELD_CAST(MaszynaScenarioEventServer::MemoryField)
VARIANT_ENUM_CAST(MaszynaScenarioEventServer::TrackEvent)
VARIANT_ENUM_CAST(MaszynaScenarioEventServer::IsolatedEvent)
