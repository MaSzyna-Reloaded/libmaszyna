#include "../tracks/TrackServer.hpp"
#include "ScenarioEventServer.hpp"
#include "driver/DriverServer.hpp"
#include "legacy/e3d/E3DRenderingServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "utils/Names.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *ScenarioEventServer::event_queued_signal = "event_queued";
    const char *ScenarioEventServer::event_launched_signal = "event_launched";
    const char *ScenarioEventServer::event_dequeued_signal = "event_dequeued";
    const char *ScenarioEventServer::memory_values_changed_signal = "memory_values_changed";

    void ScenarioEventServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("event_create"), &ScenarioEventServer::event_create);
        ClassDB::bind_method(D_METHOD("event_free", "event"), &ScenarioEventServer::event_free);
        ClassDB::bind_method(D_METHOD("event_set_name", "event", "name"), &ScenarioEventServer::event_set_name);
        ClassDB::bind_method(D_METHOD("event_get_name", "event"), &ScenarioEventServer::event_get_name);
        ClassDB::bind_method(D_METHOD("event_get_rid_by_name", "name"), &ScenarioEventServer::event_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("event_set_delay", "event", "seconds"), &ScenarioEventServer::event_set_delay);
        ClassDB::bind_method(D_METHOD("event_get_delay", "event"), &ScenarioEventServer::event_get_delay);
        ClassDB::bind_method(
                D_METHOD("event_set_random_delay", "event", "seconds"), &ScenarioEventServer::event_set_random_delay);
        ClassDB::bind_method(D_METHOD("event_get_random_delay", "event"), &ScenarioEventServer::event_get_random_delay);
        ClassDB::bind_method(
                D_METHOD("event_set_departure_delay", "event", "seconds"),
                &ScenarioEventServer::event_set_departure_delay);
        ClassDB::bind_method(
                D_METHOD("event_get_departure_delay", "event"), &ScenarioEventServer::event_get_departure_delay);
        ClassDB::bind_method(
                D_METHOD("event_set_passive", "event", "passive"), &ScenarioEventServer::event_set_passive);
        ClassDB::bind_method(D_METHOD("event_is_passive", "event"), &ScenarioEventServer::event_is_passive);
        ClassDB::bind_method(
                D_METHOD("event_attach_action", "event", "action"), &ScenarioEventServer::event_attach_action);
        ClassDB::bind_method(D_METHOD("event_get_action", "event"), &ScenarioEventServer::event_get_action);
        ClassDB::bind_method(
                D_METHOD("event_attach_condition", "event", "condition"), &ScenarioEventServer::event_attach_condition);
        ClassDB::bind_method(D_METHOD("event_get_condition", "event"), &ScenarioEventServer::event_get_condition);
        ClassDB::bind_method(
                D_METHOD("event_queue", "event", "activator", "extra_delay"), &ScenarioEventServer::event_queue,
                DEFVAL(RID()), DEFVAL(0.0));
        ClassDB::bind_method(D_METHOD("event_is_queued", "event"), &ScenarioEventServer::event_is_queued);
        ClassDB::bind_method(D_METHOD("event_get_run_time", "event"), &ScenarioEventServer::event_get_run_time);
        ClassDB::bind_method(D_METHOD("queue_get_events"), &ScenarioEventServer::queue_get_events);

        ClassDB::bind_method(D_METHOD("memory_create"), &ScenarioEventServer::memory_create);
        ClassDB::bind_method(D_METHOD("memory_free", "memory"), &ScenarioEventServer::memory_free);
        ClassDB::bind_method(D_METHOD("memory_set_name", "memory", "name"), &ScenarioEventServer::memory_set_name);
        ClassDB::bind_method(D_METHOD("memory_get_name", "memory"), &ScenarioEventServer::memory_get_name);
        ClassDB::bind_method(D_METHOD("memory_get_rid_by_name", "name"), &ScenarioEventServer::memory_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("memory_set_values", "memory", "text", "value1", "value2"),
                &ScenarioEventServer::memory_set_values);
        ClassDB::bind_method(D_METHOD("memory_get_text", "memory"), &ScenarioEventServer::memory_get_text);
        ClassDB::bind_method(D_METHOD("memory_get_value1", "memory"), &ScenarioEventServer::memory_get_value1);
        ClassDB::bind_method(D_METHOD("memory_get_value2", "memory"), &ScenarioEventServer::memory_get_value2);
        ClassDB::bind_method(D_METHOD("memory_set_track", "memory", "track"), &ScenarioEventServer::memory_set_track);
        ClassDB::bind_method(D_METHOD("memory_get_track", "memory"), &ScenarioEventServer::memory_get_track);
        ClassDB::bind_method(
                D_METHOD("memory_set_position", "memory", "position"), &ScenarioEventServer::memory_set_position);
        ClassDB::bind_method(D_METHOD("memory_get_position", "memory"), &ScenarioEventServer::memory_get_position);

        ClassDB::bind_method(
                D_METHOD("track_add_event", "track", "slot", "event"), &ScenarioEventServer::track_add_event);
        ClassDB::bind_method(D_METHOD("track_get_events", "track", "slot"), &ScenarioEventServer::track_get_events);
        ClassDB::bind_method(D_METHOD("track_clear_events", "track"), &ScenarioEventServer::track_clear_events);
        ClassDB::bind_method(
                D_METHOD("isolated_add_event", "isolated", "slot", "event"), &ScenarioEventServer::isolated_add_event);
        ClassDB::bind_method(
                D_METHOD("isolated_clear_events", "isolated"), &ScenarioEventServer::isolated_clear_events);
        ClassDB::bind_method(
                D_METHOD("animation_set_done_event", "instance", "submodel", "event"),
                &ScenarioEventServer::animation_set_done_event);

        ClassDB::bind_method(D_METHOD("launcher_create"), &ScenarioEventServer::launcher_create);
        ClassDB::bind_method(D_METHOD("launcher_free", "launcher"), &ScenarioEventServer::launcher_free);
        ClassDB::bind_method(
                D_METHOD("launcher_set_name", "launcher", "name"), &ScenarioEventServer::launcher_set_name);
        ClassDB::bind_method(D_METHOD("launcher_get_name", "launcher"), &ScenarioEventServer::launcher_get_name);
        ClassDB::bind_method(
                D_METHOD("launcher_get_rid_by_name", "name"), &ScenarioEventServer::launcher_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("launcher_set_events", "launcher", "event", "shift_event"),
                &ScenarioEventServer::launcher_set_events);
        ClassDB::bind_method(D_METHOD("launcher_get_event", "launcher"), &ScenarioEventServer::launcher_get_event);
        ClassDB::bind_method(
                D_METHOD("launcher_get_shift_event", "launcher"), &ScenarioEventServer::launcher_get_shift_event);
        ClassDB::bind_method(
                D_METHOD("launcher_attach_condition", "launcher", "condition"),
                &ScenarioEventServer::launcher_attach_condition);
        ClassDB::bind_method(
                D_METHOD("launcher_set_position", "launcher", "position"), &ScenarioEventServer::launcher_set_position);
        ClassDB::bind_method(
                D_METHOD("launcher_get_position", "launcher"), &ScenarioEventServer::launcher_get_position);
        ClassDB::bind_method(
                D_METHOD("launcher_set_radius", "launcher", "radius"), &ScenarioEventServer::launcher_set_radius);
        ClassDB::bind_method(D_METHOD("launcher_get_radius", "launcher"), &ScenarioEventServer::launcher_get_radius);
        ClassDB::bind_method(D_METHOD("launcher_set_key", "launcher", "key"), &ScenarioEventServer::launcher_set_key);
        ClassDB::bind_method(D_METHOD("launcher_get_key", "launcher"), &ScenarioEventServer::launcher_get_key);
        ClassDB::bind_method(
                D_METHOD("launcher_set_interval", "launcher", "seconds"), &ScenarioEventServer::launcher_set_interval);
        ClassDB::bind_method(
                D_METHOD("launcher_set_time_of_day", "launcher", "hour", "minute"),
                &ScenarioEventServer::launcher_set_time_of_day);
        ClassDB::bind_method(D_METHOD("launcher_get_rids"), &ScenarioEventServer::launcher_get_rids);
        ClassDB::bind_method(
                D_METHOD("launcher_set_radio_call", "launcher", "call"), &ScenarioEventServer::launcher_set_radio_call);
        ClassDB::bind_method(D_METHOD("launcher_fire", "launcher"), &ScenarioEventServer::launcher_fire);
        ClassDB::bind_method(D_METHOD("launcher_fire_shift", "launcher"), &ScenarioEventServer::launcher_fire_shift);

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

        ADD_SIGNAL(MethodInfo(
                event_queued_signal, PropertyInfo(Variant::RID, "event"), PropertyInfo(Variant::RID, "activator")));
        ADD_SIGNAL(MethodInfo(
                event_launched_signal, PropertyInfo(Variant::RID, "event"), PropertyInfo(Variant::RID, "activator")));
        ADD_SIGNAL(MethodInfo(event_dequeued_signal, PropertyInfo(Variant::RID, "event")));
        ADD_SIGNAL(MethodInfo(memory_values_changed_signal, PropertyInfo(Variant::RID, "memory")));
    }

    /// No explicit disconnect: callable_mp reports this instance as the callable's object, so the
    /// engine drops the connections when the instance dies.
    ScenarioEventServer::ScenarioEventServer() {
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        // a launcher at an hour looks at the time of day, which the runtime's clock runs
        runtime->connect(
                SimulationServer::time_of_day_changed_signal,
                callable_mp(this, &ScenarioEventServer::_on_time_of_day_changed));
        // ...and once the clock runs: a scenario's start minute is set before the camera is in
        // place and before its onstart events have run
        runtime->connect(
                SimulationServer::simulation_unpaused_signal,
                callable_mp(this, &ScenarioEventServer::_check_timed_launchers));
        // the track events hang on what the vehicles do on their tracks
        RailVehicleServer *vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(
                RailVehicleServer::vehicle_heading_to_track_start_signal,
                callable_mp(this, &ScenarioEventServer::_on_vehicle_heading_to_track_start));
        vehicles->connect(
                RailVehicleServer::vehicle_heading_to_track_end_signal,
                callable_mp(this, &ScenarioEventServer::_on_vehicle_heading_to_track_end));
        vehicles->connect(
                RailVehicleServer::vehicle_stopped_on_track_signal,
                callable_mp(this, &ScenarioEventServer::_on_vehicle_stopped_on_track));
        vehicles->connect(
                RailVehicleServer::vehicle_radio_called_signal,
                callable_mp(this, &ScenarioEventServer::_on_vehicle_radio_called));
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &ScenarioEventServer::_on_vehicle_freed));
        // ...and the isolated sections' events on what the sections report
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        track_server->connect(
                TrackServer::isolated_occupied_signal, callable_mp(this, &ScenarioEventServer::_on_isolated_occupied));
        track_server->connect(
                TrackServer::isolated_freed_signal, callable_mp(this, &ScenarioEventServer::_on_isolated_freed));
        track_server->connect(
                TrackServer::isolated_vehicle_entered_signal,
                callable_mp(this, &ScenarioEventServer::_on_isolated_vehicle_entered));
        track_server->connect(
                TrackServer::isolated_vehicle_left_signal,
                callable_mp(this, &ScenarioEventServer::_on_isolated_vehicle_left));
        // ...and the animations' `:done` events on what the rendering server reports
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        rendering->connect(
                E3DRenderingServer::instance_submodel_animation_finished_signal,
                callable_mp(this, &ScenarioEventServer::_on_instance_submodel_animation_finished));
        rendering->connect(
                E3DRenderingServer::instance_freed_signal, callable_mp(this, &ScenarioEventServer::_on_instance_freed));
    }

    ScenarioEventServer::~ScenarioEventServer() {
        _set_processing(false);
    }

    void ScenarioEventServer::_on_time_of_day_changed() {
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
    void ScenarioEventServer::_check_timed_launchers() {
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        const int minutes =
                static_cast<int>(Math::floor(runtime->get_time_of_day() * LibMaszynaUnits::MINUTES_PER_HOUR));
        const int hour = minutes / static_cast<int>(LibMaszynaUnits::MINUTES_PER_HOUR);
        const int minute = minutes % static_cast<int>(LibMaszynaUnits::MINUTES_PER_HOUR);
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

    bool ScenarioEventServer::_in_reach(const LauncherData &p_launcher) {
        if (p_launcher.radius < 0.0) {
            return true;
        }
        const SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        return streaming != nullptr && streaming->streaming_has_camera() &&
               p_launcher.position.distance_to(streaming->streaming_get_camera_position()) < p_launcher.radius;
    }

    /// event_manager::queue_receivers() (Event.cpp:2255-2268): only a launcher's first event, and
    /// with no activator
    void ScenarioEventServer::_on_vehicle_radio_called(
            const RID &p_vehicle, const RailVehicleRadio::RadioCall p_call, const Vector3 &p_position) {
        // copied: firing queues events, and a listener may create launchers
        const Vector<RID> listening = radio_launchers;
        for (const RID &rid: listening) {
            const LauncherData *launcher = launchers.getptr(rid);
            if (launcher == nullptr || !(launcher->radio_call == p_call)) {
                continue;
            }
            if (launcher->radius >= 0.0 && launcher->position.distance_to(p_position) >= launcher->radius) {
                continue;
            }
            _fire(launcher->condition, launcher->event);
        }
    }

    /// Queued, it holds the runtime's clock, so the time the queue waits for passes
    void ScenarioEventServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        processing = p_processing;
        if (p_processing) {
            runtime->clock_subscribe(callable_mp(this, &ScenarioEventServer::_process_queue));
            return;
        }
        runtime->clock_unsubscribe(callable_mp(this, &ScenarioEventServer::_process_queue));
    }

    /// event_manager::CheckQuery() (Event.cpp:2465-2490): the entries whose time has come run in
    /// time order. Only those queued before the pass started are taken - what an action queues,
    /// even for now, runs on the next frame, as in the original - so an event that queues itself
    /// again cannot keep the pass going.
    void ScenarioEventServer::_process_queue(double /* p_seconds */) {
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        const double time = runtime->simulation_get_time();
        const uint64_t pass_end = next_sequence;
        while (!queue.empty() && queue.top().time <= time && queue.top().sequence < pass_end) {
            const QueueEntry entry = queue.top();
            queue.pop();
            if (EventData *event = events.getptr(entry.owner); event != nullptr) {
                if (!(event->queued_sequence == entry.sequence)) {
                    continue;
                }
                event->queued_sequence = 0;
                // taken before anything runs: an action may create or free events, which
                // rehashes the table
                const Ref<ScenarioEventAction> action = event->action;
                const Ref<ScenarioEventCondition> condition = event->condition;
                emit_signal(event_launched_signal, entry.owner, entry.activator);
                const bool passed = condition.is_null() || condition->test(entry.owner, entry.activator);
                if (action.is_null()) {
                    continue;
                }
                if (passed) {
                    action->run(entry.owner, entry.activator);
                } else {
                    action->run_else(entry.owner, entry.activator);
                }
                // a track's event0 goes on while its vehicle stands there (the original queues it
                // on every Move() of a standing vehicle, TrkFoll.cpp:126-133)
                const VehicleOnTrack *on_track = vehicles_on_tracks.getptr(entry.activator);
                const TrackEvents *standing_events =
                        on_track != nullptr && on_track->standing ? track_events.getptr(on_track->track) : nullptr;
                if (standing_events != nullptr && (standing_events->events[TRACK_EVENT0].has(entry.owner) ||
                                                   standing_events->events[TRACK_EVENTALL0].has(entry.owner))) {
                    event_queue(entry.owner, entry.activator);
                }
                continue;
            }
            LauncherData *launcher = launchers.getptr(entry.owner);
            if (launcher == nullptr || !(launcher->scheduled_sequence == entry.sequence)) {
                continue;
            }
            launcher->scheduled_sequence =
                    launcher->interval > 0.0 ? _schedule(entry.owner, time + launcher->interval, RID()) : 0;
            _fire(launcher->condition, launcher->event);
        }
        _set_processing(!queue.empty());
    }

    void ScenarioEventServer::_on_vehicle_heading_to_track_start(const RID &p_vehicle, const RID &p_track) {
        VehicleOnTrack &on_track = _place_vehicle(p_vehicle, p_track);
        on_track.standing = false;
        if (on_track.fired_to_start) {
            return;
        }
        on_track.fired_to_start = true;
        _queue_track_events(p_track, TRACK_EVENT1, TRACK_EVENTALL1, p_vehicle);
    }

    void ScenarioEventServer::_on_vehicle_heading_to_track_end(const RID &p_vehicle, const RID &p_track) {
        VehicleOnTrack &on_track = _place_vehicle(p_vehicle, p_track);
        on_track.standing = false;
        if (on_track.fired_to_end) {
            return;
        }
        on_track.fired_to_end = true;
        _queue_track_events(p_track, TRACK_EVENT2, TRACK_EVENTALL2, p_vehicle);
    }

    void ScenarioEventServer::_on_vehicle_stopped_on_track(const RID &p_vehicle, const RID &p_track) {
        _place_vehicle(p_vehicle, p_track).standing = true;
        _queue_track_events(p_track, TRACK_EVENT0, TRACK_EVENTALL0, p_vehicle);
    }

    void ScenarioEventServer::_on_vehicle_freed(const RID &p_vehicle) {
        vehicles_on_tracks.erase(p_vehicle);
    }

    void ScenarioEventServer::_on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_BUSY, p_vehicle);
    }

    void ScenarioEventServer::_on_isolated_freed(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_FREE, p_vehicle);
    }

    void ScenarioEventServer::_on_isolated_vehicle_entered(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_INC, p_vehicle);
    }

    void ScenarioEventServer::_on_isolated_vehicle_left(const RID &p_isolated, const RID &p_vehicle) {
        _queue_isolated_events(p_isolated, ISOLATED_DEC, p_vehicle);
    }

    void
    ScenarioEventServer::_on_instance_submodel_animation_finished(const RID &p_instance, const String &p_submodel) {
        const HashMap<String, RID> *done_events = animation_done_events.getptr(p_instance);
        if (done_events == nullptr) {
            return;
        }
        const RID *event = done_events->getptr(p_submodel.to_lower());
        if (event != nullptr && events.has(*event)) {
            event_queue(*event);
        }
    }

    void ScenarioEventServer::_on_instance_freed(const RID &p_instance) {
        animation_done_events.erase(p_instance);
    }

    void ScenarioEventServer::_queue_isolated_events(
            const RID &p_isolated, const IsolatedEvent p_slot, const RID &p_vehicle) {
        const IsolatedEvents *bound = isolated_events.getptr(p_isolated);
        if (bound == nullptr) {
            return;
        }
        // copied: queueing emits event_queued, and a listener may bind events
        const Vector<RID> slot_events = bound->events[p_slot];
        for (const RID &event: slot_events) {
            if (events.has(event)) {
                event_queue(event, p_vehicle);
            }
        }
    }

    ScenarioEventServer::VehicleOnTrack &ScenarioEventServer::_place_vehicle(const RID &p_vehicle, const RID &p_track) {
        VehicleOnTrack &on_track = vehicles_on_tracks[p_vehicle];
        if (!(on_track.track == p_track)) {
            on_track = VehicleOnTrack();
            on_track.track = p_track;
        }
        return on_track;
    }

    /// Only a vehicle with somebody aboard fires the crew events (Owner->Mechanik, TrkFoll.cpp:125,
    /// 138, 151); the `all` events fire for any
    void ScenarioEventServer::_queue_track_events(
            const RID &p_track, const TrackEvent p_crew_slot, const TrackEvent p_all_slot, const RID &p_vehicle) {
        const TrackEvents *bound = track_events.getptr(p_track);
        if (bound == nullptr) {
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
            if (events.has(event)) {
                event_queue(event, p_vehicle);
            }
        }
        for (const RID &event: all_events) {
            if (events.has(event)) {
                event_queue(event, p_vehicle);
            }
        }
    }

    uint64_t ScenarioEventServer::_schedule(const RID &p_owner, const double p_time, const RID &p_activator) {
        QueueEntry entry;
        entry.time = p_time;
        entry.sequence = next_sequence++;
        entry.owner = p_owner;
        entry.activator = p_activator;
        queue.push(entry);
        _set_processing(!queue.empty());
        return entry.sequence;
    }

    /// TEventLauncher's condition is tested when it fires (EvLaunch.cpp:216-227). Both come by
    /// value: a script condition may create a launcher, which rehashes the table they came from.
    bool ScenarioEventServer::_fire(const Ref<ScenarioEventCondition> p_condition, const RID p_event) {
        if (!p_event.is_valid()) {
            return false;
        }
        if (p_condition.is_valid() && !p_condition->test(p_event, RID())) {
            return false;
        }
        event_queue(p_event);
        return true;
    }

    // --- event ---

    RID ScenarioEventServer::event_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        events.insert(rid, EventData());
        return rid;
    }

    /// A queued entry of the event is left in the queue and skipped when its time comes; that the
    /// event will never launch is announced by event_dequeued, once it is gone
    void ScenarioEventServer::event_free(const RID &p_event) {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        const bool queued = event->queued_sequence > 0;
        names_rename(events_by_name, event->name, StringName(), p_event);
        events.erase(p_event);
        if (queued) {
            emit_signal(event_dequeued_signal, p_event);
        }
    }

    void ScenarioEventServer::event_set_name(const RID &p_event, const StringName &p_name) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        const StringName previous = event->name;
        event->name = p_name;
        names_rename(events_by_name, previous, p_name, p_event);
    }

    StringName ScenarioEventServer::event_get_name(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, StringName());
        return event->name;
    }

    RID ScenarioEventServer::event_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = events_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    void ScenarioEventServer::event_set_delay(const RID &p_event, const double p_seconds) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->delay = p_seconds;
    }

    double ScenarioEventServer::event_get_delay(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, 0.0);
        return event->delay;
    }

    void ScenarioEventServer::event_set_random_delay(const RID &p_event, const double p_seconds) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->random_delay = p_seconds;
    }

    double ScenarioEventServer::event_get_random_delay(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, 0.0);
        return event->random_delay;
    }

    void ScenarioEventServer::event_set_departure_delay(const RID &p_event, const double p_seconds) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->departure_delay = p_seconds;
    }

    double ScenarioEventServer::event_get_departure_delay(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, Math::NaN);
        return event->departure_delay;
    }

    void ScenarioEventServer::event_set_passive(const RID &p_event, const bool p_passive) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->passive = p_passive;
    }

    bool ScenarioEventServer::event_is_passive(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, false);
        return event->passive;
    }

    void ScenarioEventServer::event_attach_action(const RID &p_event, const Ref<ScenarioEventAction> &p_action) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->action = p_action;
    }

    Ref<ScenarioEventAction> ScenarioEventServer::event_get_action(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, Ref<ScenarioEventAction>());
        return event->action;
    }

    void
    ScenarioEventServer::event_attach_condition(const RID &p_event, const Ref<ScenarioEventCondition> &p_condition) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->condition = p_condition;
    }

    Ref<ScenarioEventCondition> ScenarioEventServer::event_get_condition(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, Ref<ScenarioEventCondition>());
        return event->condition;
    }

    bool ScenarioEventServer::event_queue(const RID &p_event, const RID &p_activator, const double p_extra_delay) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, false);
        if (event->passive || event->queued_sequence > 0) {
            return false;
        }
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL_V(runtime, false);
        const double now = runtime->simulation_get_time();
        double run_time = now + event->delay + p_extra_delay + (event->random_delay * UtilityFunctions::randf());
        const VehicleServer *vehicles = VehicleServer::get_instance();
        if (!Math::is_nan(event->departure_delay) && vehicles != nullptr && vehicles->vehicle_exists(p_activator)) {
            const DriverServer *drivers = DriverServer::get_instance();
            ERR_FAIL_NULL_V(drivers, false);
            // the departure may be past already: the event runs at once (Event.cpp:2444)
            run_time = MAX(
                    now, run_time +
                                 drivers->vehicle_get_seconds_until_departure(p_activator, runtime->get_time_of_day()) +
                                 event->departure_delay);
        }
        event->queued_sequence = _schedule(p_event, run_time, p_activator);
        event->run_time = run_time;
        emit_signal(event_queued_signal, p_event, p_activator);
        return true;
    }

    bool ScenarioEventServer::event_is_queued(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, false);
        return event->queued_sequence > 0;
    }

    double ScenarioEventServer::event_get_run_time(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, -1.0);
        return event->queued_sequence > 0 ? event->run_time : -1.0;
    }

    TypedArray<RID> ScenarioEventServer::queue_get_events() const {
        TypedArray<RID> result;
        // a copy popped in order; an entry of an event queued again since, or of a launcher, is
        // not the event's (as _process_queue() skips it)
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> pending = queue;
        while (!pending.empty()) {
            const QueueEntry entry = pending.top();
            pending.pop();
            const EventData *event = events.getptr(entry.owner);
            if (event != nullptr && event->queued_sequence == entry.sequence) {
                result.push_back(entry.owner);
            }
        }
        return result;
    }

    // --- memory ---

    RID ScenarioEventServer::memory_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        memories.insert(rid, MemoryData());
        return rid;
    }

    void ScenarioEventServer::memory_free(const RID &p_memory) {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        names_rename(memories_by_name, memory->name, StringName(), p_memory);
        memories.erase(p_memory);
    }

    void ScenarioEventServer::memory_set_name(const RID &p_memory, const StringName &p_name) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        const StringName previous = memory->name;
        memory->name = p_name;
        names_rename(memories_by_name, previous, p_name, p_memory);
    }

    StringName ScenarioEventServer::memory_get_name(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, StringName());
        return memory->name;
    }

    RID ScenarioEventServer::memory_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = memories_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    void ScenarioEventServer::memory_set_values(
            const RID &p_memory, const String &p_text, const double p_value1, const double p_value2) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->text = p_text;
        memory->value1 = p_value1;
        memory->value2 = p_value2;
        emit_signal(memory_values_changed_signal, p_memory);
        // a launcher waiting within its minute for its memory's condition
        _check_timed_launchers();
    }

    String ScenarioEventServer::memory_get_text(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, String());
        return memory->text;
    }

    double ScenarioEventServer::memory_get_value1(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, 0.0);
        return memory->value1;
    }

    double ScenarioEventServer::memory_get_value2(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, 0.0);
        return memory->value2;
    }

    void ScenarioEventServer::memory_set_track(const RID &p_memory, const RID &p_track) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->track = p_track;
    }

    RID ScenarioEventServer::memory_get_track(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, RID());
        return memory->track;
    }

    void ScenarioEventServer::memory_set_position(const RID &p_memory, const Vector3 &p_position) {
        MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL(memory);
        memory->position = p_position;
    }

    Vector3 ScenarioEventServer::memory_get_position(const RID &p_memory) const {
        const MemoryData *memory = memories.getptr(p_memory);
        ERR_FAIL_NULL_V(memory, Vector3());
        return memory->position;
    }

    // --- track ---

    void ScenarioEventServer::track_add_event(const RID &p_track, const TrackEvent p_slot, const RID &p_event) {
        ERR_FAIL_INDEX(p_slot, TRACK_EVENT_MAX);
        track_events[p_track].events[p_slot].push_back(p_event);
    }

    TypedArray<RID> ScenarioEventServer::track_get_events(const RID &p_track, const TrackEvent p_slot) const {
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

    void ScenarioEventServer::track_clear_events(const RID &p_track) {
        track_events.erase(p_track);
    }

    // --- isolated section ---

    void
    ScenarioEventServer::isolated_add_event(const RID &p_isolated, const IsolatedEvent p_slot, const RID &p_event) {
        ERR_FAIL_INDEX(p_slot, ISOLATED_EVENT_MAX);
        isolated_events[p_isolated].events[p_slot].push_back(p_event);
    }

    void ScenarioEventServer::isolated_clear_events(const RID &p_isolated) {
        isolated_events.erase(p_isolated);
    }

    // --- animation ---

    void
    ScenarioEventServer::animation_set_done_event(const RID &p_instance, const String &p_submodel, const RID &p_event) {
        animation_done_events[p_instance][p_submodel.to_lower()] = p_event;
    }

    // --- launcher ---

    RID ScenarioEventServer::launcher_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        launchers.insert(rid, LauncherData());
        return rid;
    }

    /// A scheduled firing is left in the queue and skipped when its time comes
    void ScenarioEventServer::launcher_free(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        names_rename(launchers_by_name, launcher->name, StringName(), p_launcher);
        launchers.erase(p_launcher);
        timed_launchers.erase(p_launcher);
        radio_launchers.erase(p_launcher);
    }

    void ScenarioEventServer::launcher_set_name(const RID &p_launcher, const StringName &p_name) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        const StringName previous = launcher->name;
        launcher->name = p_name;
        names_rename(launchers_by_name, previous, p_name, p_launcher);
    }

    StringName ScenarioEventServer::launcher_get_name(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, StringName());
        return launcher->name;
    }

    RID ScenarioEventServer::launcher_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = launchers_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    void ScenarioEventServer::launcher_set_events(const RID &p_launcher, const RID &p_event, const RID &p_shift_event) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->event = p_event;
        launcher->shift_event = p_shift_event;
    }

    RID ScenarioEventServer::launcher_get_event(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, RID());
        return launcher->event;
    }

    RID ScenarioEventServer::launcher_get_shift_event(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, RID());
        return launcher->shift_event;
    }

    void ScenarioEventServer::launcher_attach_condition(
            const RID &p_launcher, const Ref<ScenarioEventCondition> &p_condition) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->condition = p_condition;
    }

    void ScenarioEventServer::launcher_set_position(const RID &p_launcher, const Vector3 &p_position) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->position = p_position;
    }

    Vector3 ScenarioEventServer::launcher_get_position(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, Vector3());
        return launcher->position;
    }

    void ScenarioEventServer::launcher_set_radius(const RID &p_launcher, const double p_radius) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->radius = p_radius;
    }

    double ScenarioEventServer::launcher_get_radius(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, 0.0);
        return launcher->radius;
    }

    void ScenarioEventServer::launcher_set_key(const RID &p_launcher, const Key p_key) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->key = p_key;
    }

    Key ScenarioEventServer::launcher_get_key(const RID &p_launcher) const {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL_V(launcher, KEY_NONE);
        return launcher->key;
    }

    /// TEventLauncher's negative DeltaTime (EvLaunch.cpp:136-137, 187-196)
    void ScenarioEventServer::launcher_set_interval(const RID &p_launcher, const double p_seconds) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->interval = p_seconds;
        launcher->scheduled_sequence = 0;
        if (p_seconds <= 0.0) {
            return;
        }
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        launcher->scheduled_sequence = _schedule(p_launcher, runtime->simulation_get_time() + p_seconds, RID());
    }

    /// TEventLauncher's HHMM DeltaTime (EvLaunch.cpp:138-157)
    void ScenarioEventServer::launcher_set_time_of_day(const RID &p_launcher, const int p_hour, const int p_minute) {
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

    void ScenarioEventServer::launcher_set_radio_call(const RID &p_launcher, const RailVehicleRadio::RadioCall p_call) {
        LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        launcher->radio_call = p_call;
        if (!radio_launchers.has(p_launcher)) {
            radio_launchers.push_back(p_launcher);
        }
    }

    TypedArray<RID> ScenarioEventServer::launcher_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, LauncherData> &launcher: launchers) {
            result.push_back(launcher.key);
        }
        return result;
    }

    void ScenarioEventServer::launcher_fire(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        _fire(launcher->condition, launcher->event);
    }

    void ScenarioEventServer::launcher_fire_shift(const RID &p_launcher) {
        const LauncherData *launcher = launchers.getptr(p_launcher);
        ERR_FAIL_NULL(launcher);
        _fire(launcher->condition, launcher->shift_event);
    }
} // namespace godot
