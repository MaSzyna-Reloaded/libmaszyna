#include "driver/DriverServer.hpp"
#include "maszyna/scenario/MaszynaMemoryAction.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleController.hpp"
#include "player/PlayerServer.hpp"
#include "radio/RadioServer.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "station/StationServer.hpp"
#include "tracks/TrackServer.hpp"
#include "utils/Names.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /// What drivers read on the tracks, or nothing a memory carries for them: no command to
        /// send (TMemCell::CommandCheck(), MemCell.cpp:52-99), and a passenger stop by its prefix
        const char *const TEXTS_NOT_SENT[] = {"SetVelocity",          "ShuntVelocity",   "OutsideStation",
                                              "SetProximityVelocity", "Emergency_brake", "CabSignal"};
        const char *const PASSENGER_STOP_POINT = "PassengerStopPoint:";
    } // namespace

    const char *MaszynaScenarioEventServer::memory_values_changed_signal = "memory_values_changed";
    MaszynaScenarioEventServer *MaszynaScenarioEventServer::singleton = nullptr;

    void MaszynaScenarioEventServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("implementation_get"), &MaszynaScenarioEventServer::implementation_get);
        ClassDB::bind_method(
                D_METHOD("event_set_departure_delay", "event", "seconds"),
                &MaszynaScenarioEventServer::event_set_departure_delay);
        ClassDB::bind_method(
                D_METHOD("event_get_departure_delay", "event"), &MaszynaScenarioEventServer::event_get_departure_delay);
        ClassDB::bind_method(
                D_METHOD("event_set_passive", "event", "passive"), &MaszynaScenarioEventServer::event_set_passive);
        ClassDB::bind_method(
                D_METHOD("vehicle_put_command", "vehicle", "command", "value1", "value2", "position"),
                &MaszynaScenarioEventServer::vehicle_put_command);
        ClassDB::bind_method(
                D_METHOD("event_set_queued_on_every_move", "event", "every_move"),
                &MaszynaScenarioEventServer::event_set_queued_on_every_move);
        ClassDB::bind_method(
                D_METHOD("event_is_queued_on_every_move", "event"),
                &MaszynaScenarioEventServer::event_is_queued_on_every_move);
        ClassDB::bind_method(D_METHOD("event_is_passive", "event"), &MaszynaScenarioEventServer::event_is_passive);
        ClassDB::bind_method(
                D_METHOD("event_add_to_query", "event", "activator"), &MaszynaScenarioEventServer::event_add_to_query,
                DEFVAL(RID()));
        ClassDB::bind_method(D_METHOD("memory_create", "system"), &MaszynaScenarioEventServer::memory_create);
        ClassDB::bind_method(D_METHOD("memory_free", "memory"), &MaszynaScenarioEventServer::memory_free);
        ClassDB::bind_method(
                D_METHOD("memory_set_name", "memory", "name"), &MaszynaScenarioEventServer::memory_set_name);
        ClassDB::bind_method(D_METHOD("memory_get_name", "memory"), &MaszynaScenarioEventServer::memory_get_name);
        ClassDB::bind_method(
                D_METHOD("memory_get_rid_by_name", "name"), &MaszynaScenarioEventServer::memory_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("memory_set_values", "memory", "text", "value1", "value2"),
                &MaszynaScenarioEventServer::memory_set_values);
        ClassDB::bind_method(
                D_METHOD("memory_check_command", "memory"), &MaszynaScenarioEventServer::memory_check_command);
        ClassDB::bind_method(D_METHOD("memory_has_command", "memory"), &MaszynaScenarioEventServer::memory_has_command);
        ClassDB::bind_method(
                D_METHOD("memory_command_sent", "memory"), &MaszynaScenarioEventServer::memory_command_sent);
        ClassDB::bind_method(
                D_METHOD("memory_set_sent_event", "memory", "event"),
                &MaszynaScenarioEventServer::memory_set_sent_event);
        ClassDB::bind_method(D_METHOD("memory_get_text", "memory"), &MaszynaScenarioEventServer::memory_get_text);
        ClassDB::bind_method(D_METHOD("memory_get_value1", "memory"), &MaszynaScenarioEventServer::memory_get_value1);
        ClassDB::bind_method(D_METHOD("memory_get_value2", "memory"), &MaszynaScenarioEventServer::memory_get_value2);
        ClassDB::bind_method(
                D_METHOD("memory_add_friction_track", "memory", "track"),
                &MaszynaScenarioEventServer::memory_add_friction_track);
        ClassDB::bind_method(
                D_METHOD("memory_set_track", "memory", "track"), &MaszynaScenarioEventServer::memory_set_track);
        ClassDB::bind_method(D_METHOD("memory_get_track", "memory"), &MaszynaScenarioEventServer::memory_get_track);
        ClassDB::bind_method(
                D_METHOD("memory_set_position", "memory", "position"),
                &MaszynaScenarioEventServer::memory_set_position);
        ClassDB::bind_method(
                D_METHOD("memory_get_position", "memory"), &MaszynaScenarioEventServer::memory_get_position);
        ClassDB::bind_method(
                D_METHOD("track_add_event", "track", "slot", "event"), &MaszynaScenarioEventServer::track_add_event);
        ClassDB::bind_method(
                D_METHOD("track_get_events", "track", "slot"), &MaszynaScenarioEventServer::track_get_events);
        ClassDB::bind_method(
                D_METHOD("track_set_velocity_changing", "track", "event"),
                &MaszynaScenarioEventServer::track_set_velocity_changing);
        ClassDB::bind_method(
                D_METHOD("track_is_velocity_changing", "track"),
                &MaszynaScenarioEventServer::track_is_velocity_changing);
        ClassDB::bind_method(
                D_METHOD("isolated_add_event", "isolated", "slot", "event"),
                &MaszynaScenarioEventServer::isolated_add_event);
        ClassDB::bind_method(D_METHOD("launcher_create", "system"), &MaszynaScenarioEventServer::launcher_create);
        ClassDB::bind_method(D_METHOD("launcher_free", "launcher"), &MaszynaScenarioEventServer::launcher_free);
        ClassDB::bind_method(
                D_METHOD("launcher_set_name", "launcher", "name"), &MaszynaScenarioEventServer::launcher_set_name);
        ClassDB::bind_method(D_METHOD("launcher_get_name", "launcher"), &MaszynaScenarioEventServer::launcher_get_name);
        ClassDB::bind_method(
                D_METHOD("launcher_get_rid_by_name", "name"), &MaszynaScenarioEventServer::launcher_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("launcher_set_events", "launcher", "event", "shift_event"),
                &MaszynaScenarioEventServer::launcher_set_events);
        ClassDB::bind_method(
                D_METHOD("launcher_get_event", "launcher"), &MaszynaScenarioEventServer::launcher_get_event);
        ClassDB::bind_method(
                D_METHOD("launcher_get_shift_event", "launcher"),
                &MaszynaScenarioEventServer::launcher_get_shift_event);
        ClassDB::bind_method(
                D_METHOD("launcher_attach_condition", "launcher", "condition"),
                &MaszynaScenarioEventServer::launcher_attach_condition);
        ClassDB::bind_method(
                D_METHOD("launcher_set_position", "launcher", "position"),
                &MaszynaScenarioEventServer::launcher_set_position);
        ClassDB::bind_method(
                D_METHOD("launcher_get_position", "launcher"), &MaszynaScenarioEventServer::launcher_get_position);
        ClassDB::bind_method(
                D_METHOD("launcher_set_radius", "launcher", "radius"),
                &MaszynaScenarioEventServer::launcher_set_radius);
        ClassDB::bind_method(
                D_METHOD("launcher_get_radius", "launcher"), &MaszynaScenarioEventServer::launcher_get_radius);
        ClassDB::bind_method(
                D_METHOD("launcher_set_key", "launcher", "key"), &MaszynaScenarioEventServer::launcher_set_key);
        ClassDB::bind_method(D_METHOD("launcher_get_key", "launcher"), &MaszynaScenarioEventServer::launcher_get_key);
        ClassDB::bind_method(
                D_METHOD("launcher_set_key_needs_shift", "launcher", "needs_shift"),
                &MaszynaScenarioEventServer::launcher_set_key_needs_shift);
        ClassDB::bind_method(
                D_METHOD("launcher_get_key_needs_shift", "launcher"),
                &MaszynaScenarioEventServer::launcher_get_key_needs_shift);
        ClassDB::bind_method(
                D_METHOD("launcher_set_fire_once", "launcher", "fire_once"),
                &MaszynaScenarioEventServer::launcher_set_fire_once);
        ClassDB::bind_method(
                D_METHOD("launcher_set_train_triggered", "launcher", "train_triggered"),
                &MaszynaScenarioEventServer::launcher_set_train_triggered);
        ClassDB::bind_method(
                D_METHOD("launcher_set_interval", "launcher", "seconds"),
                &MaszynaScenarioEventServer::launcher_set_interval);
        ClassDB::bind_method(
                D_METHOD("launcher_set_time_of_day", "launcher", "hour", "minute"),
                &MaszynaScenarioEventServer::launcher_set_time_of_day);
        ClassDB::bind_method(D_METHOD("launcher_get_rids"), &MaszynaScenarioEventServer::launcher_get_rids);
        ClassDB::bind_method(
                D_METHOD("launcher_set_radio_call", "launcher", "call"),
                &MaszynaScenarioEventServer::launcher_set_radio_call);
        ClassDB::bind_method(D_METHOD("launcher_fire", "launcher"), &MaszynaScenarioEventServer::launcher_fire);
        ClassDB::bind_method(
                D_METHOD("launcher_fire_shift", "launcher"), &MaszynaScenarioEventServer::launcher_fire_shift);

        BIND_BITFIELD_FLAG(MEMORY_FIELD_TEXT);
        BIND_BITFIELD_FLAG(MEMORY_FIELD_VALUE1);
        BIND_BITFIELD_FLAG(MEMORY_FIELD_VALUE2);
        BIND_ENUM_CONSTANT(TRACK_EVENT0);
        BIND_ENUM_CONSTANT(TRACK_EVENT1);
        BIND_ENUM_CONSTANT(TRACK_EVENT2);
        BIND_ENUM_CONSTANT(TRACK_EVENTALL0);
        BIND_ENUM_CONSTANT(TRACK_EVENTALL1);
        BIND_ENUM_CONSTANT(TRACK_EVENTALL2);
        BIND_ENUM_CONSTANT(ISOLATED_BUSY);
        BIND_ENUM_CONSTANT(ISOLATED_FREE);
        BIND_ENUM_CONSTANT(ISOLATED_INC);
        BIND_ENUM_CONSTANT(ISOLATED_DEC);

        ADD_SIGNAL(MethodInfo(memory_values_changed_signal, PropertyInfo(Variant::RID, "memory")));
    }

    MaszynaScenarioEventServer *MaszynaScenarioEventServer::get_instance() {
        return singleton;
    }

    /// No explicit disconnect: callable_mp reports this instance as the callable's object, so the
    /// engine drops the connections when the instance dies.
    MaszynaScenarioEventServer::MaszynaScenarioEventServer() {
        singleton = this;
        implementation.instantiate();
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        // a launcher at an hour looks at the time of day, which the runtime's clock runs
        runtime->connect(
                SimulationServer::time_of_day_changed_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_time_of_day_changed));
        // ...and once the clock runs: a scenario's start minute is set before the camera is in
        // place and before its onstart events have run
        runtime->connect(
                SimulationServer::simulation_unpaused_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_check_timed_launchers));
        RadioServer *radios = RadioServer::get_instance();
        ERR_FAIL_NULL(radios);
        radios->connect(
                RadioServer::transmitted_signal, callable_mp(this, &MaszynaScenarioEventServer::_on_radio_transmitted));
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &MaszynaScenarioEventServer::_on_vehicle_freed));
        // ...and the isolated sections' events on what the sections report
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        // the track events hang on what the vehicles do on their tracks
        track_server->connect(
                TrackServer::track_vehicle_entered_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_track_vehicle_travelling));
        track_server->connect(
                TrackServer::track_vehicle_started_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_track_vehicle_travelling));
        track_server->connect(
                TrackServer::track_vehicle_stopped_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_track_vehicle_stopped));
        track_server->connect(
                TrackServer::isolated_occupied_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_isolated_occupied));
        track_server->connect(
                TrackServer::isolated_freed_signal, callable_mp(this, &MaszynaScenarioEventServer::_on_isolated_freed));
        track_server->connect(
                TrackServer::isolated_axle_entered_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_isolated_axle_entered));
        track_server->connect(
                TrackServer::isolated_axle_left_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_isolated_axle_left));
        // what the original says of an event goes with it
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(scenario);
        scenario->connect(
                ScenarioEventServer::event_freed_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_on_event_freed));
    }

    MaszynaScenarioEventServer::~MaszynaScenarioEventServer() {
        singleton = nullptr;
    }

    Ref<MaszynaScenarioEventImplementation> MaszynaScenarioEventServer::implementation_get() const {
        return implementation;
    }

    void MaszynaScenarioEventServer::system_release(const RID &p_system) {
        const SystemData *system = systems.getptr(p_system);
        if (system == nullptr) {
            return;
        }
        // copied: freeing takes each one out of the system's own lists
        const SystemData owned = *system;
        for (const RID &memory: owned.memories) {
            memory_free(memory);
        }
        for (const RID &launcher: owned.launchers) {
            launcher_free(launcher);
        }
        for (const RID &track: owned.tracks) {
            track_events.erase(track);
            every_move_tracks.erase(track);
        }
        _set_every_move_processing(!every_move_tracks.is_empty());
        for (const RID &isolated: owned.isolated) {
            isolated_events.erase(isolated);
        }
        systems.erase(p_system);
    }

    bool MaszynaScenarioEventServer::event_can_queue(const RID &p_event) const {
        return !passive_events.has(p_event);
    }

    /// Queued by a vehicle, an event of `departuredelay` waits for its train's departure by the
    /// timetable (Event.cpp:2431-2441)
    double MaszynaScenarioEventServer::event_get_run_time(
            const RID &p_event, const RID &p_activator, const double p_now, const double p_run_time) const {
        const double *departure_delay = departure_delays.getptr(p_event);
        const VehicleServer *vehicles = VehicleServer::get_instance();
        if (departure_delay == nullptr || vehicles == nullptr || !vehicles->vehicle_exists(p_activator)) {
            return p_run_time;
        }
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        const StationServer *stations = StationServer::get_instance();
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL_V(rail_vehicles, p_run_time);
        ERR_FAIL_NULL_V(stations, p_run_time);
        ERR_FAIL_NULL_V(runtime, p_run_time);
        // the timetable is the trainset's: its run's departure from where it stands, none without
        // one (TTrainParameters::seconds_until_departure(), mtable.cpp:184-190)
        const RID trainset = rail_vehicles->vehicle_get_trainset(p_activator);
        const RID run = trainset.is_valid() ? stations->run_find_by_trainset(trainset) : RID();
        const double until_departure = run.is_valid()
                                               ? stations->run_get_time_to_departure(run, runtime->get_time_of_day()) *
                                                         Units::SECONDS_PER_MINUTE
                                               : 0.0;
        // the departure may be past already: the event runs at once (Event.cpp:2444)
        return MAX(p_now, p_run_time + until_departure + *departure_delay);
    }

    /// A track's event0 goes on while its vehicle stands there, on the tracks of both its axles (the
    /// original queues it on every Move() of a standing vehicle, for each axle, TrkFoll.cpp:126-133 -
    /// an event already queued is not queued twice, Event.cpp:2384)
    void MaszynaScenarioEventServer::event_ran(const RID &p_event, const RID &p_activator) {
        const VehicleOnTrack *on_track = vehicles_on_tracks.getptr(p_activator);
        if (on_track == nullptr || !on_track->standing) {
            return;
        }
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(scenario);
        for (const RID &standing_track: {on_track->track, on_track->secondary_track}) {
            const TrackEvents *standing_events = track_events.getptr(standing_track);
            if (standing_events != nullptr && (standing_events->events[TRACK_EVENT0].has(p_event) ||
                                               standing_events->events[TRACK_EVENTALL0].has(p_event))) {
                event_add_to_query(p_event, p_activator);
                return;
            }
        }
    }

    /// A launcher's interval came: it is scheduled again, then fires (EvLaunch.cpp:187-196)
    void MaszynaScenarioEventServer::scheduled(const RID &p_owner, const int64_t p_ticket) {
        LauncherData *launcher = launchers.getptr(p_owner);
        if (launcher == nullptr || !(launcher->scheduled_ticket == p_ticket)) {
            return;
        }
        launcher->scheduled_ticket = 0;
        const SimulationServer *runtime = SimulationServer::get_instance();
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(runtime);
        ERR_FAIL_NULL(scenario);
        if (launcher->fire_once) {
            // the first time it is in reach and its condition passes (EvLaunch.cpp:182-186)
            launcher->fired = _in_reach(*launcher) && _fire(launcher->condition, launcher->event);
            if (!launcher->fired) {
                launcher->scheduled_ticket = scenario->system_schedule(
                        launcher->system, p_owner, runtime->simulation_get_time() + FIRE_ONCE_CHECK_SECONDS);
            }
            return;
        }
        if (launcher->interval > 0.0) {
            launcher->scheduled_ticket = scenario->system_schedule(
                    launcher->system, p_owner, runtime->simulation_get_time() + launcher->interval);
        }
        // an interval counts only in reach (scene.cpp:135-139)
        if (_in_reach(*launcher)) {
            _fire(launcher->condition, launcher->event);
        }
    }

    void MaszynaScenarioEventServer::event_set_departure_delay(const RID &p_event, const double p_seconds) {
        if (Math::is_nan(p_seconds)) {
            departure_delays.erase(p_event);
            return;
        }
        departure_delays[p_event] = p_seconds;
    }

    double MaszynaScenarioEventServer::event_get_departure_delay(const RID &p_event) const {
        const double *departure_delay = departure_delays.getptr(p_event);
        return departure_delay != nullptr ? *departure_delay : Math::NaN;
    }

    void MaszynaScenarioEventServer::event_set_passive(const RID &p_event, const bool p_passive) {
        if (p_passive) {
            passive_events.insert(p_event);
            return;
        }
        passive_events.erase(p_event);
    }

    bool MaszynaScenarioEventServer::event_is_passive(const RID &p_event) const {
        return passive_events.has(p_event);
    }

    void MaszynaScenarioEventServer::event_set_queued_on_every_move(const RID &p_event, const bool p_every_move) {
        if (p_every_move) {
            every_move_events.insert(p_event);
            return;
        }
        every_move_events.erase(p_event);
    }

    bool MaszynaScenarioEventServer::event_is_queued_on_every_move(const RID &p_event) const {
        return every_move_events.has(p_event);
    }

    /// TTrackFollower::Move() of a primary axle staying on its track (TrkFoll.cpp:249-260): on every
    /// step the track's event1/event2 of a delay of -1 or less go on for a vehicle whose cab is
    /// occupied (CabOccupied), its eventall1/eventall2 for any - an event already queued is not
    /// queued twice (Event.cpp:2384)
    void MaszynaScenarioEventServer::_queue_every_move_events(const double /* p_seconds */) {
        const TrackServer *tracks = TrackServer::get_instance();
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        const ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(tracks);
        ERR_FAIL_NULL(rail_vehicles);
        ERR_FAIL_NULL(scenario);
        // copied: queueing emits event_queued, and a listener may bind events to a track
        LocalVector<RID> moving_tracks;
        for (const RID &track: every_move_tracks) {
            moving_tracks.push_back(track);
        }
        for (const RID &track: moving_tracks) {
            const TypedArray<RID> on_track = tracks->track_get_vehicles(track);
            for (int64_t i = 0; i < on_track.size(); i++) {
                const RID vehicle = on_track[i];
                const bool occupied = rail_vehicles->vehicle_get_driver_cabin(vehicle).is_valid();
                for (const TrackEvent slot: {TRACK_EVENT1, TRACK_EVENT2, TRACK_EVENTALL1, TRACK_EVENTALL2}) {
                    const TrackEvents *bound = track_events.getptr(track);
                    if (bound == nullptr || (!occupied && (slot == TRACK_EVENT1 || slot == TRACK_EVENT2))) {
                        continue;
                    }
                    const Vector<RID> slot_events = bound->events[slot];
                    for (const RID &event: slot_events) {
                        if (every_move_events.has(event) && scenario->event_exists(event)) {
                            event_add_to_query(event, vehicle);
                        }
                    }
                }
            }
        }
    }

    void MaszynaScenarioEventServer::_set_every_move_processing(const bool p_processing) {
        if (every_move_processing == p_processing) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        every_move_processing = p_processing;
        if (p_processing) {
            runtime->connect(
                    SimulationServer::simulation_advanced_signal,
                    callable_mp(this, &MaszynaScenarioEventServer::_queue_every_move_events));
            return;
        }
        runtime->disconnect(
                SimulationServer::simulation_advanced_signal,
                callable_mp(this, &MaszynaScenarioEventServer::_queue_every_move_events));
    }

    bool MaszynaScenarioEventServer::event_add_to_query(const RID &p_event, const RID &p_activator) {
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL_V(scenario, false);
        const Ref<MaszynaMemoryAction> action = scenario->event_get_action(p_event);
        // basic_event::is_instant(), updatevalues_event::is_instant() (Event.cpp:566-571)
        if (action.is_valid() && action->get_mode() == MaszynaMemoryAction::MODE_ADD &&
            scenario->event_get_delay(p_event) == 0.0) {
            if (!event_can_queue(p_event)) {
                return false;
            }
            scenario->event_run(p_event, p_activator);
            return true;
        }
        return scenario->event_queue(p_event, p_activator);
    }

    void MaszynaScenarioEventServer::_on_event_freed(const RID &p_event) {
        departure_delays.erase(p_event);
        passive_events.erase(p_event);
        every_move_events.erase(p_event);
    }

    MaszynaScenarioEventServer::SystemData *MaszynaScenarioEventServer::_get_event_system(const RID &p_event) {
        const ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL_V(scenario, nullptr);
        const RID system = scenario->event_get_system(p_event);
        ERR_FAIL_COND_V_MSG(!system.is_valid(), nullptr, "The event belongs to no event system");
        return &systems[system];
    }

    void MaszynaScenarioEventServer::_on_time_of_day_changed() {
        _check_timed_launchers();
    }
    /// TEventLauncher::check_activation() (EvLaunch.cpp:197-211) under event_manager::update()
    /// (Event.cpp:2276-2284) and basic_cell::update_events() (scene.cpp:124-143): the original looks
    /// every frame, and only at a launcher in reach whose condition passes - it fires once while the
    /// clock shows its HH:MM, and is armed again when the hour is another. Its condition is a
    /// memory's (check_conditions(), EvLaunch.cpp:213-222), so here a launcher of no radius is looked
    /// at when the minute comes and again whenever a memory changes within it; one of a radius on
    /// every step besides, as the camera comes into its reach (_update_timed_reach_processing()). The minute is the
    /// clock's whole minutes, as SimulationServer counts them: a start set to 21:05 is
    /// 21.08333..., which truncated to minutes was 21:04 (docs/findings-archive.md, 2026-10-03
    /// scenarios that did not start).
    void MaszynaScenarioEventServer::_check_timed_launchers() {
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        const int minutes = static_cast<int>(Math::floor(runtime->get_time_of_day() * Units::MINUTES_PER_HOUR));
        const int hour = minutes / static_cast<int>(Units::MINUTES_PER_HOUR);
        const int minute = minutes % static_cast<int>(Units::MINUTES_PER_HOUR);
        // copied: firing queues events, and a listener may create launchers
        const Vector<RID> timed = timed_launchers;
        for (const RID &rid: timed) {
            LauncherData *launcher = launchers.getptr(rid);
            if (launcher == nullptr) {
                continue;
            }
            // only a launcher in reach whose condition passes is looked at (scene.cpp:132-134,
            // Event.cpp:2277), its arming included
            if (!_in_reach(*launcher) ||
                (launcher->condition.is_valid() && !launcher->condition->test(launcher->event, RID()))) {
                continue;
            }
            if (!(launcher->hour == hour)) {
                launcher->armed = true;
                continue;
            }
            if (!(launcher->minute == minute) || !launcher->armed) {
                continue;
            }
            launcher->armed = !_fire(launcher->condition, launcher->event);
        }
    }

    void MaszynaScenarioEventServer::_on_simulation_advanced_timed_launchers(const double /* p_seconds */) {
        _check_timed_launchers();
    }

    void MaszynaScenarioEventServer::_update_timed_reach_processing() {
        bool reach = false;
        for (const RID &rid: timed_launchers) {
            const LauncherData *launcher = launchers.getptr(rid);
            reach = reach || (launcher != nullptr && launcher->radius >= 0.0);
        }
        if (reach == timed_reach_processing) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        timed_reach_processing = reach;
        const Callable step = callable_mp(this, &MaszynaScenarioEventServer::_on_simulation_advanced_timed_launchers);
        if (reach) {
            runtime->connect(SimulationServer::simulation_advanced_signal, step);
        } else {
            runtime->disconnect(SimulationServer::simulation_advanced_signal, step);
        }
    }
    bool MaszynaScenarioEventServer::_in_reach(const LauncherData &p_launcher) {
        if (p_launcher.radius < 0.0) {
            return true;
        }
        const PlayerServer *player = PlayerServer::get_instance();
        const RID train = player != nullptr ? player->player_get_vehicle() : RID();
        if (p_launcher.train_triggered && train.is_valid()) {
            // the front of the player's vehicle, its coupler towards its trainset's head
            // (HeadPosition(), DynObj.h:702-703); its front is the body's -Z
            const VehicleServer *vehicles = VehicleServer::get_instance();
            const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
            ERR_FAIL_NULL_V(vehicles, false);
            ERR_FAIL_NULL_V(rail_vehicles, false);
            const Transform3D transform = vehicles->vehicle_get_transform(train);
            const Ref<VehicleController> controller = vehicles->vehicle_get_controller(train);
            const double half_length = controller.is_valid() ? controller->get_dimensions_length() / 2.0 : 0.0;
            const double head_side = rail_vehicles->vehicle_get_trainset(train).is_valid() &&
                                                     rail_vehicles->trainset_vehicle_get_direction(train) ==
                                                             TrackServer::DIRECTION_REVERSED
                                             ? -1.0
                                             : 1.0;
            const Vector3 head = transform.origin - transform.basis.get_column(2) * half_length * head_side;
            return p_launcher.position.distance_to(head) < p_launcher.radius;
        }
        const SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        return streaming != nullptr && streaming->streaming_has_camera() &&
               p_launcher.position.distance_to(streaming->streaming_get_camera_position()) < p_launcher.radius;
    }
    /// event_manager::queue_receivers() (Event.cpp:2255-2268): a radio call heard by the launchers
    /// listening for it within their radius - only a launcher's first event, and with no activator
    void MaszynaScenarioEventServer::_on_radio_transmitted(
            const int64_t p_kind, const int64_t /* p_channel */, const Variant &p_payload, const Vector3 &p_position,
            const double /* p_reach */) {
        if (p_kind != RadioServer::KIND_CALL) {
            return;
        }
        const RailVehicleRadio::RadioCall call = static_cast<RailVehicleRadio::RadioCall>(int(p_payload));
        // copied: firing queues events, and a listener may create launchers
        const Vector<RID> listening = radio_launchers;
        for (const RID &rid: listening) {
            const LauncherData *launcher = launchers.getptr(rid);
            if (launcher == nullptr || !(launcher->radio_call == call)) {
                continue;
            }
            if (launcher->radius >= 0.0 && launcher->position.distance_to(p_position) >= launcher->radius) {
                continue;
            }
            _fire(launcher->condition, launcher->event);
        }
    }
    void MaszynaScenarioEventServer::_on_track_vehicle_travelling(
            const RID &p_track, const RID &p_vehicle, const TrackServer::TravelDirection p_direction) {
        if (p_direction == TrackServer::TRAVEL_NONE) {
            return; // put there standing, or crossed at a crawl: no event of a moving vehicle
        }
        VehicleOnTrack &on_track = _place_vehicle(p_vehicle, p_track);
        on_track.standing = false;
        on_track.secondary_track = RID();
        const bool to_start = p_direction == TrackServer::TRAVEL_TOWARD_START;
        bool &fired = to_start ? on_track.fired_to_start : on_track.fired_to_end;
        if (fired) {
            return;
        }
        // the trainset fires the events of a move only going the way its head's driver set it to go
        // - the head's direction (CheckDirection(), Driver.cpp:2384-2392), turned as the vehicle
        // faces the head; a trainset nobody drives goes any way (eventfilter, TrkFoll.cpp:115-121,
        // 131, 147). A move filtered out leaves the events for a later one (SetFlag(), :132, 148)
        const VehicleServer *vehicles = VehicleServer::get_instance();
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_NULL(rail_vehicles);
        const RID trainset = rail_vehicles->vehicle_get_trainset(p_vehicle);
        const RID head = trainset.is_valid() ? rail_vehicles->trainset_get_head(trainset) : RID();
        if (head.is_valid() && vehicles->vehicle_has_person_role(head, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER)) {
            const Ref<RailVehicleController> head_controller = vehicles->vehicle_get_controller(head);
            const Ref<VehicleController> controller = vehicles->vehicle_get_controller(p_vehicle);
            ERR_FAIL_COND(head_controller.is_null() || controller.is_null());
            int intended = head_controller->get_direction_absolute();
            if (intended == VehicleController::DIRECTION_NEUTRAL) {
                const RailVehicleCabinKind::Kind cabin = head_controller->get_active_cabin_kind();
                if (cabin == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT) {
                    intended = VehicleController::DIRECTION_FORWARD;
                } else if (cabin == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR) {
                    intended = VehicleController::DIRECTION_BACKWARD;
                }
            }
            if (!(rail_vehicles->trainset_vehicle_get_direction(head) ==
                  rail_vehicles->trainset_vehicle_get_direction(p_vehicle))) {
                intended = -intended;
            }
            const double velocity = controller->get_velocity();
            if (intended == VehicleController::DIRECTION_NEUTRAL || (velocity > 0.0) != (intended > 0)) {
                return;
            }
        }
        fired = true;
        _queue_track_events(
                p_track, to_start ? TRACK_EVENT1 : TRACK_EVENT2, to_start ? TRACK_EVENTALL1 : TRACK_EVENTALL2,
                p_vehicle);
    }
    void MaszynaScenarioEventServer::_on_track_vehicle_stopped(const RID &p_track, const RID &p_vehicle) {
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        // the vehicle is listed on its own track only - the other axle's is the other one
        if (tracks->track_get_vehicle_list(p_track).has(p_vehicle)) {
            VehicleOnTrack &on_track = _place_vehicle(p_vehicle, p_track);
            on_track.standing = true;
            on_track.secondary_track = RID();
        } else {
            // reported right after the vehicle's own track: its once-per-entry state stays
            VehicleOnTrack *on_track = vehicles_on_tracks.getptr(p_vehicle);
            ERR_FAIL_NULL(on_track);
            on_track->secondary_track = p_track;
        }
        _queue_track_events(p_track, TRACK_EVENT0, TRACK_EVENTALL0, p_vehicle);
    }
    void MaszynaScenarioEventServer::_on_vehicle_freed(const RID &p_vehicle) {
        vehicles_on_tracks.erase(p_vehicle);
        pending_loads.erase(p_vehicle);
        _set_pending_load_processing(!pending_loads.is_empty());
    }

    void MaszynaScenarioEventServer::vehicle_put_command(
            const RID &p_vehicle, const String &p_command, const double p_value1, const double p_value2,
            const Vector3 &p_position) {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        const bool load = p_command.begins_with(LOAD_COMMAND);
        const bool unload = p_command.begins_with(UNLOAD_COMMAND);
        PackedFloat64Array values;
        values.push_back(p_value1);
        values.push_back(p_value2);
        if (!load && !unload) {
            pending_loads.erase(p_vehicle);
            _set_pending_load_processing(!pending_loads.is_empty());
            vehicles->vehicle_send_command(
                    p_vehicle, MaszynaMoverRailVehicleController::PUT_COMMAND, p_command, values);
            return;
        }
        // the load takes the place of the vehicle's own order waiting (PutCommand(), Mover.cpp:12101)
        vehicles->vehicle_send_command(p_vehicle, MaszynaMoverRailVehicleController::PUT_COMMAND, String(), values);
        PendingLoad pending;
        pending.unload = unload;
        pending.load_name = p_command.trim_prefix(load ? LOAD_COMMAND : UNLOAD_COMMAND).to_lower();
        pending.position = p_position;
        pending.reach = p_value2 > LOAD_REACH_GIVEN_ABOVE ? p_value2 : LOAD_REACH;
        pending_loads[p_vehicle] = pending;
        _try_pending_load(p_vehicle);
        _set_pending_load_processing(!pending_loads.is_empty());
    }

    /// Load= and UnLoad= (Mover.cpp:12658-12694): taken or given while the vehicle stands within the
    /// reach, again after a move, until it is full or empty - tried as it is put, then on every step
    void MaszynaScenarioEventServer::_process_pending_loads(const double /* p_seconds */) {
        // copied: an exchange started emits signals, and a listener may put another order
        LocalVector<RID> waiting;
        for (const KeyValue<RID, PendingLoad> &entry: pending_loads) {
            waiting.push_back(entry.key);
        }
        for (const RID &vehicle: waiting) {
            _try_pending_load(vehicle);
        }
        _set_pending_load_processing(!pending_loads.is_empty());
    }

    void MaszynaScenarioEventServer::_try_pending_load(const RID &p_vehicle) {
        VehicleServer *vehicles = VehicleServer::get_instance();
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_NULL(rail_vehicles);
        const PendingLoad *pending = pending_loads.getptr(p_vehicle);
        if (pending == nullptr) {
            return;
        }
        const Ref<RailVehicleLoad> load =
                vehicles->vehicle_component_get(p_vehicle, VehicleComponentType::COMPONENT_LOAD);
        const Ref<VehicleController> controller = vehicles->vehicle_get_controller(p_vehicle);
        if (load.is_null() || controller.is_null() ||
            (pending->unload ? load->get_load_amount() <= 0.0 : load->get_load_amount() >= load->get_max_load())) {
            pending_loads.erase(p_vehicle);
            return;
        }
        if (rail_vehicles->load_get_exchange_time(p_vehicle) > 0.0 || !rail_vehicles->vehicle_is_standing(p_vehicle)) {
            return;
        }
        // Distance(): across the ground, less the vehicle's length (Mover.cpp:575-578)
        const Vector3 offset = vehicles->vehicle_get_position(p_vehicle) - pending->position;
        if (Vector2(offset.x, offset.z).length() - controller->get_dimensions_length() >= pending->reach) {
            return;
        }
        if (pending->unload) {
            rail_vehicles->load_remove(p_vehicle, load->get_load_amount(), RailVehicleLoad::PLATFORM_SIDE_BOTH);
        } else {
            rail_vehicles->load_add(
                    p_vehicle, load->get_max_load() - load->get_load_amount(), RailVehicleLoad::PLATFORM_SIDE_BOTH,
                    pending->load_name);
        }
    }

    void MaszynaScenarioEventServer::_set_pending_load_processing(const bool p_processing) {
        if (pending_load_processing == p_processing) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        pending_load_processing = p_processing;
        const Callable step = callable_mp(this, &MaszynaScenarioEventServer::_process_pending_loads);
        if (p_processing) {
            runtime->connect(SimulationServer::simulation_advanced_signal, step);
        } else {
            runtime->disconnect(SimulationServer::simulation_advanced_signal, step);
        }
    }
    void MaszynaScenarioEventServer::_on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_BUSY, p_vehicle);
    }
    void MaszynaScenarioEventServer::_on_isolated_freed(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_FREE, p_vehicle);
    }
    void MaszynaScenarioEventServer::_on_isolated_axle_entered(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_INC, p_vehicle);
    }
    void MaszynaScenarioEventServer::_on_isolated_axle_left(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_DEC, p_vehicle);
    }
    void MaszynaScenarioEventServer::_queue_isolated_events(
            const RID &p_isolated, const IsolatedEvent p_slot, const RID &p_vehicle) {
        const IsolatedEvents *bound = isolated_events.getptr(p_isolated);
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        if (bound == nullptr || scenario == nullptr) {
            return;
        }
        // copied: queueing emits event_queued, and a listener may bind events
        const Vector<RID> slot_events = bound->events[p_slot];
        for (const RID &event: slot_events) {
            if (scenario->event_exists(event)) {
                event_add_to_query(event, p_vehicle);
            }
        }
    }
    MaszynaScenarioEventServer::VehicleOnTrack &
    MaszynaScenarioEventServer::_place_vehicle(const RID &p_vehicle, const RID &p_track) {
        VehicleOnTrack &on_track = vehicles_on_tracks[p_vehicle];
        if (!(on_track.track == p_track)) {
            on_track = VehicleOnTrack();
            on_track.track = p_track;
        }
        return on_track;
    }
    /// Only a vehicle with somebody aboard fires the crew events (Owner->Mechanik, TrkFoll.cpp:125,
    /// 138, 151); the `all` events fire for any
    void MaszynaScenarioEventServer::_queue_track_events(
            const RID &p_track, const TrackEvent p_crew_slot, const TrackEvent p_all_slot, const RID &p_vehicle) {
        const TrackEvents *bound = track_events.getptr(p_track);
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        if (bound == nullptr || scenario == nullptr) {
            return;
        }
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        // copied: queueing emits event_queued, and a listener may bind events to a track
        const Vector<RID> all_events = bound->events[p_all_slot];
        // the crew events go for one driver of a trainset, the one at its head (Mechanik->primary(),
        // TrkFoll.cpp:125, 138, 151) - a vehicle alone is its own head
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        const RID trainset = rail_vehicles->vehicle_get_trainset(p_vehicle);
        const bool crewed =
                !bound->events[p_crew_slot].is_empty() &&
                vehicles->vehicle_has_person_role(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER) &&
                (!trainset.is_valid() || rail_vehicles->trainset_get_head(trainset) == p_vehicle);
        const Vector<RID> crew_events = crewed ? bound->events[p_crew_slot] : Vector<RID>();
        for (const RID &event: crew_events) {
            if (scenario->event_exists(event)) {
                event_add_to_query(event, p_vehicle);
            }
        }
        for (const RID &event: all_events) {
            if (scenario->event_exists(event)) {
                event_add_to_query(event, p_vehicle);
            }
        }
    }
    /// TEventLauncher's condition is tested when it fires (EvLaunch.cpp:216-227). Both come by
    /// value: a script condition may create a launcher, which rehashes the table they came from.
    bool MaszynaScenarioEventServer::_fire(const Ref<ScenarioEventCondition> p_condition, const RID p_event) {
        if (!p_event.is_valid()) {
            return false;
        }
        if (p_condition.is_valid() && !p_condition->test(p_event, RID())) {
            return false;
        }
        MaszynaScenarioEventServer *events = get_instance();
        ERR_FAIL_NULL_V(events, false);
        events->event_add_to_query(p_event);
        return true;
    }
    // --- memory ---

    RID MaszynaScenarioEventServer::memory_create(const RID &p_system) {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        MemoryData memory;
        memory.system = p_system;
        memories.insert(rid, memory);
        systems[p_system].memories.push_back(rid);
        return rid;
    }

    void MaszynaScenarioEventServer::memory_free(const RID &p_memory) {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        names_rename(memories_by_name, memory->name, StringName(), p_memory);
        if (SystemData *system = systems.getptr(memory->system); system != nullptr) {
            system->memories.erase(p_memory);
        }
        memories.erase(p_memory);
    }

    void MaszynaScenarioEventServer::memory_set_name(const RID &p_memory, const StringName &p_name) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        const StringName previous = memory->name;
        memory->name = p_name;
        names_rename(memories_by_name, previous, p_name, p_memory);
    }

    StringName MaszynaScenarioEventServer::memory_get_name(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, StringName());
        return memory->name;
    }

    RID MaszynaScenarioEventServer::memory_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = memories_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    void MaszynaScenarioEventServer::memory_set_values(
            const RID &p_memory, const String &p_text, const double p_value1, const double p_value2) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->text = p_text;
        memory->value1 = p_value1;
        memory->value2 = p_value2;
        if (TrackServer *tracks = TrackServer::get_instance(); tracks != nullptr) {
            for (const RID &track: memory->friction_tracks) {
                tracks->track_set_friction_modifier(track, memory->value1, memory->value2);
            }
        }
        emit_signal(memory_values_changed_signal, p_memory);
        // a launcher waiting within its minute for its memory's condition
        _check_timed_launchers();
    }

    void MaszynaScenarioEventServer::memory_check_command(const RID &p_memory) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        bool command = !memory->text.begins_with(PASSENGER_STOP_POINT);
        for (const char *text: TEXTS_NOT_SENT) {
            command = command && !(memory->text == text);
        }
        memory->command = command;
    }

    bool MaszynaScenarioEventServer::memory_has_command(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, false);
        return memory->command;
    }

    void MaszynaScenarioEventServer::memory_command_sent(const RID &p_memory) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        if (!memory->command) {
            return;
        }
        memory->command = false;
        if (memory->sent_event.is_valid()) {
            event_add_to_query(memory->sent_event);
        }
    }

    void MaszynaScenarioEventServer::memory_set_sent_event(const RID &p_memory, const RID &p_event) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->sent_event = p_event;
    }

    String MaszynaScenarioEventServer::memory_get_text(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, String());
        return memory->text;
    }

    double MaszynaScenarioEventServer::memory_get_value1(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, 0.0);
        return memory->value1;
    }

    double MaszynaScenarioEventServer::memory_get_value2(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, 0.0);
        return memory->value2;
    }

    void MaszynaScenarioEventServer::memory_add_friction_track(const RID &p_memory, const RID &p_track) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        memory->friction_tracks.push_back(p_track);
        tracks->track_set_friction_modifier(p_track, memory->value1, memory->value2);
    }

    void MaszynaScenarioEventServer::memory_set_track(const RID &p_memory, const RID &p_track) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->track = p_track;
    }

    RID MaszynaScenarioEventServer::memory_get_track(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, RID());
        return memory->track;
    }

    void MaszynaScenarioEventServer::memory_set_position(const RID &p_memory, const Vector3 &p_position) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->position = p_position;
    }

    Vector3 MaszynaScenarioEventServer::memory_get_position(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, Vector3());
        return memory->position;
    }

    // --- track ---

    void MaszynaScenarioEventServer::track_add_event(const RID &p_track, const TrackEvent p_slot, const RID &p_event) {
        ERR_FAIL_INDEX(p_slot, TRACK_EVENT_MAX);
        SystemData *system = _get_event_system(p_event);
        ERR_FAIL_NULL(system);
        if (!track_events.has(p_track)) {
            system->tracks.push_back(p_track);
        }
        track_events[p_track].events[p_slot].push_back(p_event);
        if (every_move_events.has(p_event) && p_slot != TRACK_EVENT0 && p_slot != TRACK_EVENTALL0) {
            every_move_tracks.insert(p_track);
            _set_every_move_processing(true);
        }
    }

    TypedArray<RID> MaszynaScenarioEventServer::track_get_events(const RID &p_track, const TrackEvent p_slot) const {
        TypedArray<RID> result;
        ERR_FAIL_INDEX_V(p_slot, TRACK_EVENT_MAX, result);
        const TrackEvents *bound = track_events.getptr(p_track);
        if (bound == nullptr) {
            return result;
        }
        for (const RID &event: bound->events[p_slot]) {
            result.push_back(event);
        }
        return result;
    }

    void MaszynaScenarioEventServer::track_set_velocity_changing(const RID &p_track, const RID &p_event) {
        SystemData *system = _get_event_system(p_event);
        ERR_FAIL_NULL(system);
        if (!track_events.has(p_track)) {
            system->tracks.push_back(p_track);
        }
        track_events[p_track].velocity_changing = true;
    }

    bool MaszynaScenarioEventServer::track_is_velocity_changing(const RID &p_track) const {
        const TrackEvents *bound = track_events.getptr(p_track);
        return bound != nullptr && bound->velocity_changing;
    }

    // --- isolated section ---

    void MaszynaScenarioEventServer::isolated_add_event(
            const RID &p_isolated, const IsolatedEvent p_slot, const RID &p_event) {
        ERR_FAIL_INDEX(p_slot, ISOLATED_EVENT_MAX);
        SystemData *system = _get_event_system(p_event);
        ERR_FAIL_NULL(system);
        if (!isolated_events.has(p_isolated)) {
            system->isolated.push_back(p_isolated);
        }
        isolated_events[p_isolated].events[p_slot].push_back(p_event);
    }

    // --- launcher ---

    RID MaszynaScenarioEventServer::launcher_create(const RID &p_system) {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        LauncherData launcher;
        launcher.system = p_system;
        launchers.insert(rid, launcher);
        systems[p_system].launchers.push_back(rid);
        return rid;
    }

    /// A scheduled firing is left in the queue and skipped when its time comes
    void MaszynaScenarioEventServer::launcher_free(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        names_rename(launchers_by_name, launcher->name, StringName(), p_launcher);
        if (SystemData *system = systems.getptr(launcher->system); system != nullptr) {
            system->launchers.erase(p_launcher);
        }
        launchers.erase(p_launcher);
        timed_launchers.erase(p_launcher);
        radio_launchers.erase(p_launcher);
        _update_timed_reach_processing();
    }

    void MaszynaScenarioEventServer::launcher_set_name(const RID &p_launcher, const StringName &p_name) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        const StringName previous = launcher->name;
        launcher->name = p_name;
        names_rename(launchers_by_name, previous, p_name, p_launcher);
    }

    StringName MaszynaScenarioEventServer::launcher_get_name(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, StringName());
        return launcher->name;
    }

    RID MaszynaScenarioEventServer::launcher_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = launchers_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    void MaszynaScenarioEventServer::launcher_set_events(
            const RID &p_launcher, const RID &p_event, const RID &p_shift_event) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->event = p_event;
        launcher->shift_event = p_shift_event;
    }

    RID MaszynaScenarioEventServer::launcher_get_event(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, RID());
        return launcher->event;
    }

    RID MaszynaScenarioEventServer::launcher_get_shift_event(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, RID());
        return launcher->shift_event;
    }

    void MaszynaScenarioEventServer::launcher_attach_condition(
            const RID &p_launcher, const Ref<ScenarioEventCondition> &p_condition) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->condition = p_condition;
    }

    void MaszynaScenarioEventServer::launcher_set_position(const RID &p_launcher, const Vector3 &p_position) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->position = p_position;
    }

    Vector3 MaszynaScenarioEventServer::launcher_get_position(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, Vector3());
        return launcher->position;
    }

    void MaszynaScenarioEventServer::launcher_set_radius(const RID &p_launcher, const double p_radius) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->radius = p_radius;
        _update_timed_reach_processing();
    }

    double MaszynaScenarioEventServer::launcher_get_radius(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, 0.0);
        return launcher->radius;
    }

    void MaszynaScenarioEventServer::launcher_set_key(const RID &p_launcher, const Key p_key) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->key = p_key;
    }

    Key MaszynaScenarioEventServer::launcher_get_key(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, KEY_NONE);
        return launcher->key;
    }

    /// TEventLauncher's negative DeltaTime (EvLaunch.cpp:136-137, 187-196)
    void MaszynaScenarioEventServer::launcher_set_interval(const RID &p_launcher, const double p_seconds) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->interval = p_seconds;
        launcher->scheduled_ticket = 0;
        if (p_seconds <= 0.0) {
            return;
        }
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(scenario);
        launcher->scheduled_ticket =
                scenario->system_schedule(launcher->system, p_launcher, runtime->simulation_get_time() + p_seconds);
    }

    /// TEventLauncher's HHMM DeltaTime (EvLaunch.cpp:138-157)
    void
    MaszynaScenarioEventServer::launcher_set_time_of_day(const RID &p_launcher, const int p_hour, const int p_minute) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->hour = p_hour;
        launcher->minute = p_minute;
        launcher->armed = true;
        if (p_hour < 0) {
            timed_launchers.erase(p_launcher);
        } else if (!timed_launchers.has(p_launcher)) {
            timed_launchers.push_back(p_launcher);
        }
        _update_timed_reach_processing();
    }

    void MaszynaScenarioEventServer::launcher_set_radio_call(
            const RID &p_launcher, const RailVehicleRadio::RadioCall p_call) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->radio_call = p_call;
        if (!radio_launchers.has(p_launcher)) {
            radio_launchers.push_back(p_launcher);
        }
    }

    TypedArray<RID> MaszynaScenarioEventServer::launcher_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, LauncherData> &launcher: launchers) {
            result.push_back(launcher.key);
        }
        return result;
    }

    void MaszynaScenarioEventServer::launcher_fire(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        _fire(launcher->condition, launcher->event);
    }

    /// launch_event(): with Shift its second event, its first when it has none (scene.cpp:700-703)
    void MaszynaScenarioEventServer::launcher_fire_shift(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        _fire(launcher->condition, launcher->shift_event.is_valid() ? launcher->shift_event : launcher->event);
    }

    void MaszynaScenarioEventServer::launcher_set_key_needs_shift(const RID &p_launcher, const bool p_needs_shift) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->key_needs_shift = p_needs_shift;
    }

    bool MaszynaScenarioEventServer::launcher_get_key_needs_shift(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, false);
        return launcher->key_needs_shift;
    }

    void MaszynaScenarioEventServer::launcher_set_fire_once(const RID &p_launcher, const bool p_fire_once) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->fire_once = p_fire_once;
        launcher->fired = false;
        launcher->scheduled_ticket = 0;
        if (!p_fire_once) {
            return;
        }
        const SimulationServer *runtime = SimulationServer::get_instance();
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(runtime);
        ERR_FAIL_NULL(scenario);
        launcher->scheduled_ticket = scenario->system_schedule(
                launcher->system, p_launcher, runtime->simulation_get_time() + FIRE_ONCE_CHECK_SECONDS);
    }

    void MaszynaScenarioEventServer::launcher_set_train_triggered(const RID &p_launcher, const bool p_train_triggered) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->train_triggered = p_train_triggered;
    }
} // namespace godot
