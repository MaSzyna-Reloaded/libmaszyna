#include "driver/DriverServer.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "radio/RadioServer.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "tracks/TrackServer.hpp"
#include "utils/Names.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
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
        ClassDB::bind_method(D_METHOD("event_is_passive", "event"), &MaszynaScenarioEventServer::event_is_passive);
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
        }
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
        const DriverServer *drivers = DriverServer::get_instance();
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL_V(drivers, p_run_time);
        ERR_FAIL_NULL_V(runtime, p_run_time);
        // the departure may be past already: the event runs at once (Event.cpp:2444)
        return MAX(
                p_now, p_run_time +
                               drivers->vehicle_get_seconds_until_departure(p_activator, runtime->get_time_of_day()) +
                               *departure_delay);
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
                scenario->event_queue(p_event, p_activator);
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
        if (launcher->interval > 0.0) {
            const SimulationServer *runtime = SimulationServer::get_instance();
            ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
            ERR_FAIL_NULL(runtime);
            ERR_FAIL_NULL(scenario);
            launcher->scheduled_ticket = scenario->system_schedule(
                    launcher->system, p_owner, runtime->simulation_get_time() + launcher->interval);
        }
        _fire(launcher->condition, launcher->event);
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

    void MaszynaScenarioEventServer::_on_event_freed(const RID &p_event) {
        departure_delays.erase(p_event);
        passive_events.erase(p_event);
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
    /// (Event.cpp:2293-2306): the original looks every frame - a launcher fires once while the
    /// clock shows its HH:MM and its condition passes, and is armed again when the hour is another.
    /// Its condition is a memory's (check_conditions(), EvLaunch.cpp:213-222), so here it is looked
    /// at when the minute comes and again whenever a memory changes within it. The minute is the
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
            if (!(launcher->hour == hour)) {
                launcher->armed = true;
                continue;
            }
            if (!(launcher->minute == minute) || !launcher->armed || !_in_reach(*launcher)) {
                continue;
            }
            launcher->armed = !_fire(launcher->condition, launcher->event);
        }
    }
    bool MaszynaScenarioEventServer::_in_reach(const LauncherData &p_launcher) {
        if (p_launcher.radius < 0.0) {
            return true;
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
                scenario->event_queue(event, p_vehicle);
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
        const bool crewed = !bound->events[p_crew_slot].is_empty() &&
                            vehicles->vehicle_has_person_role(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
        const Vector<RID> crew_events = crewed ? bound->events[p_crew_slot] : Vector<RID>();
        for (const RID &event: crew_events) {
            if (scenario->event_exists(event)) {
                scenario->event_queue(event, p_vehicle);
            }
        }
        for (const RID &event: all_events) {
            if (scenario->event_exists(event)) {
                scenario->event_queue(event, p_vehicle);
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
        ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL_V(scenario, false);
        scenario->event_queue(p_event);
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

    void MaszynaScenarioEventServer::launcher_fire_shift(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        _fire(launcher->condition, launcher->shift_event);
    }
} // namespace godot
