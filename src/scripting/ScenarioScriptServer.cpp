#include "ScenarioScriptAction.hpp"
#include "ScenarioScriptServer.hpp"
#include "logging/GameLogger.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "signalling/SignallingServer.hpp"
#include "tracks/TrackServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <iterator>
#ifdef LIBMASZYNA_LUA
#include "lua/LuaScriptContext.hpp"
#endif

namespace godot {
    const char *ScenarioScriptServer::script_error_signal = "script_error";

    void ScenarioScriptServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("context_create", "base_dir"), &ScenarioScriptServer::context_create);
        ClassDB::bind_method(D_METHOD("context_free", "context"), &ScenarioScriptServer::context_free);
        ClassDB::bind_method(D_METHOD("context_get_rids"), &ScenarioScriptServer::context_get_rids);
        ClassDB::bind_method(D_METHOD("context_run_file", "context", "path"), &ScenarioScriptServer::context_run_file);
        ClassDB::bind_method(
                D_METHOD("context_check_source", "context", "unit", "source"),
                &ScenarioScriptServer::context_check_source);
        ClassDB::bind_method(
                D_METHOD("context_apply_source", "context", "unit", "source"),
                &ScenarioScriptServer::context_apply_source);
        ClassDB::bind_method(
                D_METHOD("context_attach_cabin_implementation", "context", "cabin"),
                &ScenarioScriptServer::context_attach_cabin_implementation);
        ClassDB::bind_method(
                D_METHOD("context_get_cabin_implementation", "context"),
                &ScenarioScriptServer::context_get_cabin_implementation);

        ADD_SIGNAL(MethodInfo(
                script_error_signal, PropertyInfo(Variant::RID, "context"), PropertyInfo(Variant::STRING, "message")));
    }

    /// No explicit disconnect: callable_mp reports this instance as the callable's object, so the
    /// engine drops the connections when the instance dies.
    ScenarioScriptServer::ScenarioScriptServer() {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_command_received_signal,
                callable_mp(this, &ScenarioScriptServer::_on_vehicle_command_received));
        RailVehicleServer *vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(
                RailVehicleServer::vehicle_heading_to_track_start_signal,
                callable_mp(this, &ScenarioScriptServer::_on_vehicle_heading_to_track_start));
        vehicles->connect(
                RailVehicleServer::vehicle_heading_to_track_end_signal,
                callable_mp(this, &ScenarioScriptServer::_on_vehicle_heading_to_track_end));
        vehicles->connect(
                RailVehicleServer::vehicle_stopped_on_track_signal,
                callable_mp(this, &ScenarioScriptServer::_on_vehicle_stopped_on_track));
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        tracks->connect(
                TrackServer::isolated_occupied_signal, callable_mp(this, &ScenarioScriptServer::_on_isolated_occupied));
        tracks->connect(
                TrackServer::isolated_freed_signal, callable_mp(this, &ScenarioScriptServer::_on_isolated_freed));
        tracks->connect(
                TrackServer::switch_active_track_changed_signal,
                callable_mp(this, &ScenarioScriptServer::_on_switch_active_track_changed));
        SignallingServer *signalling = SignallingServer::get_instance();
        ERR_FAIL_NULL(signalling);
        signalling->connect(
                SignallingServer::signal_head_aspect_changed_signal,
                callable_mp(this, &ScenarioScriptServer::_on_signal_head_aspect_changed));
        ScenarioEventServer *events = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(events);
        events->connect(
                ScenarioEventServer::event_launched_signal,
                callable_mp(this, &ScenarioScriptServer::_on_event_launched));
        events->connect(
                ScenarioEventServer::memory_values_changed_signal,
                callable_mp(this, &ScenarioScriptServer::_on_memory_values_changed));
    }

    // --- contexts ---

    RID ScenarioScriptServer::context_create(const String &p_base_dir) {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        Context context;
#ifdef LIBMASZYNA_LUA
        context.runtime = memnew(LuaScriptContext(rid, p_base_dir));
#endif
        contexts.insert(rid, context);
        return rid;
    }

    /// What the scripts made goes first, while the runtime still holds their functions
    void ScenarioScriptServer::context_free(const RID &p_context) {
        Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL(context);
        const Vector<RID> owned = context->hooks;
        for (const RID &hook: owned) {
            _free_hook(hook);
        }
        if (context->cabin.is_valid()) {
            context->cabin->disconnect(
                    ScenarioScriptCabinImplementation::control_changed_signal,
                    callable_mp(this, &ScenarioScriptServer::_on_cabin_control_changed).bind(p_context));
        }
        if (context->runtime != nullptr) {
            memdelete(context->runtime);
        }
        contexts.erase(p_context);
    }

    TypedArray<RID> ScenarioScriptServer::context_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, Context> &context: contexts) {
            result.push_back(context.key);
        }
        return result;
    }

    bool ScenarioScriptServer::context_run_file(const RID &p_context, const String &p_path) {
        return _enter(p_context, p_path, [&p_path](ScriptRuntime *p_runtime) { return p_runtime->run_file(p_path); });
    }

    String
    ScenarioScriptServer::context_check_source(const RID &p_context, const StringName &p_unit, const String &p_source) {
        const Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL_V(context, String());
        if (context->runtime == nullptr) {
            return NO_RUNTIME_MESSAGE;
        }
        return context->runtime->check_source(p_source, p_unit);
    }

    bool
    ScenarioScriptServer::context_apply_source(const RID &p_context, const StringName &p_unit, const String &p_source) {
        Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL_V(context, false);
        const Vector<RID> owned = context->hooks;
        for (const RID &hook: owned) {
            if (hooks[hook].unit == p_unit) {
                _free_hook(hook);
            }
        }
        return _enter(p_context, p_unit, [&p_source, &p_unit](ScriptRuntime *p_runtime) {
            return p_runtime->run_source(p_source, p_unit);
        });
    }

    void ScenarioScriptServer::context_attach_cabin_implementation(
            const RID &p_context, const Ref<ScenarioScriptCabinImplementation> &p_cabin) {
        Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL(context);
        const Callable changed = callable_mp(this, &ScenarioScriptServer::_on_cabin_control_changed).bind(p_context);
        if (context->cabin.is_valid()) {
            context->cabin->disconnect(ScenarioScriptCabinImplementation::control_changed_signal, changed);
        }
        context->cabin = p_cabin;
        if (p_cabin.is_valid()) {
            p_cabin->connect(ScenarioScriptCabinImplementation::control_changed_signal, changed);
        }
    }

    Ref<ScenarioScriptCabinImplementation>
    ScenarioScriptServer::context_get_cabin_implementation(const RID &p_context) const {
        const Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL_V(context, Ref<ScenarioScriptCabinImplementation>());
        return context->cabin;
    }

    // --- what the scripts make ---

    RID ScenarioScriptServer::script_event_create(
            const RID &p_context, const int64_t p_function, const int64_t p_else_function) {
        RID hook = _create_hook(p_context, HOOK_EVENT, p_function);
        ERR_FAIL_COND_V(!hook.is_valid(), RID());
        hooks[hook].else_function = p_else_function;
        return hooks[hook].event;
    }

    bool ScenarioScriptServer::script_event_free(const RID &p_context, const RID &p_event) {
        const Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL_V(context, false);
        for (const RID &hook: context->hooks) {
            const Hook &data = hooks[hook];
            if (data.kind == HOOK_EVENT && data.event == p_event) {
                _free_hook(hook);
                return true;
            }
        }
        return false;
    }

    /// A repeating timer is TEventLauncher's interval (EvLaunch.cpp:136-137) firing the hook's event
    RID ScenarioScriptServer::script_timer_create(
            const RID &p_context, const double p_seconds, const int64_t p_function, const Timer p_timer) {
        ScenarioEventServer *events = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL_V(events, RID());
        RID hook = _create_hook(p_context, p_timer == TIMER_ONCE ? HOOK_TIMER_ONCE : HOOK_TIMER_REPEATING, p_function);
        ERR_FAIL_COND_V(!hook.is_valid(), RID());
        const RID event = hooks[hook].event;
        if (p_timer == TIMER_ONCE) {
            events->event_set_delay(event, p_seconds);
            events->event_queue(event);
            return hook;
        }
        const RID launcher = events->launcher_create();
        hooks[hook].launcher = launcher;
        events->launcher_set_events(launcher, event, RID());
        events->launcher_set_interval(launcher, p_seconds);
        return hook;
    }

    RID ScenarioScriptServer::script_subscribe(
            const RID &p_context, const ScriptSignal p_signal, const RID &p_target, const int64_t p_function) {
        ERR_FAIL_INDEX_V(p_signal, SIGNAL_MAX, RID());
        RID hook = _create_hook(p_context, HOOK_SUBSCRIPTION, p_function);
        ERR_FAIL_COND_V(!hook.is_valid(), RID());
        hooks[hook].signal = p_signal;
        hooks[hook].target = p_target;
        subscriptions[p_signal][p_target].push_back(hook);
        return hook;
    }

    bool ScenarioScriptServer::script_cancel(const RID &p_context, const RID &p_subscription) {
        const Hook *hook = hooks.getptr(p_subscription);
        if (hook == nullptr || !(hook->context == p_context) || hook->kind == HOOK_EVENT) {
            return false;
        }
        _free_hook(p_subscription);
        return true;
    }

    // --- hooks ---

    RID ScenarioScriptServer::_create_hook(const RID &p_context, const HookKind p_kind, const int64_t p_function) {
        Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL_V(context, RID());
        ScenarioEventServer *events = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL_V(events, RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        Hook hook;
        hook.context = p_context;
        hook.unit = context->active_unit;
        hook.kind = p_kind;
        hook.function = p_function;
        hook.event = events->event_create();
        Ref<ScenarioScriptAction> action;
        action.instantiate();
        action->set_hook(rid);
        events->event_attach_action(hook.event, action);
        hooks.insert(rid, hook);
        context->hooks.push_back(rid);
        return rid;
    }

    void ScenarioScriptServer::_free_hook(const RID &p_hook) {
        const Hook *hook = hooks.getptr(p_hook);
        ERR_FAIL_NULL(hook);
        ScenarioEventServer *events = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(events);
        if (hook->kind == HOOK_SUBSCRIPTION) {
            Vector<RID> *subscribed = subscriptions[hook->signal].getptr(hook->target);
            subscribed->erase(p_hook);
            if (subscribed->is_empty()) {
                subscriptions[hook->signal].erase(hook->target);
            }
        }
        if (hook->launcher.is_valid()) {
            events->launcher_free(hook->launcher);
        }
        events->event_free(hook->event);
        if (Context *context = contexts.getptr(hook->context); context != nullptr) {
            if (context->runtime != nullptr) {
                for (const int64_t function: {hook->function, hook->else_function}) {
                    if (!(function == NO_FUNCTION)) {
                        context->runtime->release(function);
                    }
                }
            }
            context->hooks.erase(p_hook);
        }
        hooks.erase(p_hook);
    }

    /// The hook may be gone after any call into the script - the script may cancel it, or make
    /// another hook, which rehashes the table - so it is looked up again after every call
    void ScenarioScriptServer::_run_hook(
            const RID &p_hook, const RID &p_event, const RID &p_activator, const Branch p_branch) {
        Hook *hook = hooks.getptr(p_hook);
        if (hook == nullptr) {
            return;
        }
        const RID context = hook->context;
        const StringName unit = hook->unit;
        if (hook->kind == HOOK_SUBSCRIPTION) {
            const Vector<Vector<ScriptArgument>> pending = hook->pending;
            hook->pending.clear();
            const int64_t function = hook->function;
            for (const Vector<ScriptArgument> &arguments: pending) {
                if (!hooks.has(p_hook)) {
                    return;
                }
                _enter(context, unit, [function, &arguments](ScriptRuntime *p_runtime) {
                    return p_runtime->call(function, arguments);
                });
            }
            return;
        }
        if (hook->kind == HOOK_EVENT) {
            const int64_t function = p_branch == BRANCH_RUN ? hook->function : hook->else_function;
            if (function == NO_FUNCTION) {
                return;
            }
            Vector<ScriptArgument> arguments;
            arguments.push_back({p_event, ScriptHandleKind::EVENT});
            arguments.push_back({p_activator, ScriptHandleKind::VEHICLE});
            _enter(context, unit,
                   [function, &arguments](ScriptRuntime *p_runtime) { return p_runtime->call(function, arguments); });
            return;
        }
        const HookKind kind = hook->kind;
        const int64_t function = hook->function;
        _enter(context, unit,
               [function](ScriptRuntime *p_runtime) { return p_runtime->call(function, Vector<ScriptArgument>()); });
        if (kind == HOOK_TIMER_ONCE && hooks.has(p_hook)) {
            _free_hook(p_hook);
        }
    }

    bool ScenarioScriptServer::_enter(
            const RID &p_context, const StringName &p_unit, const std::function<String(ScriptRuntime *)> &p_code) {
        Context *context = contexts.getptr(p_context);
        ERR_FAIL_NULL_V(context, false);
        if (context->runtime == nullptr) {
            _report(p_context, NO_RUNTIME_MESSAGE);
            return false;
        }
        const StringName previous_unit = context->active_unit;
        context->active_unit = p_unit;
        // the code can free what it made, never a context
        const String error = p_code(context->runtime);
        context->active_unit = previous_unit;
        if (error.is_empty()) {
            return true;
        }
        _report(p_context, error);
        return false;
    }

    void ScenarioScriptServer::_report(const RID &p_context, const String &p_error) {
        GameLog *log = GameLog::get_instance();
        ERR_FAIL_NULL(log);
        log->get_logger(GameLog::GAME_LOGGER)->error("lua: " + p_error);
        emit_signal(script_error_signal, p_context, p_error);
    }

    // --- what the servers report ---

    void ScenarioScriptServer::_deliver(
            const ScriptSignal p_signal, const RID &p_target, const Vector<ScriptArgument> &p_arguments,
            const RID &p_activator, const RID &p_context) {
        const Vector<RID> *subscribed = subscriptions[p_signal].getptr(p_target);
        if (subscribed == nullptr) {
            return;
        }
        ScenarioEventServer *events = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(events);
        // copied: queueing emits event_queued, and a listener may subscribe
        const Vector<RID> targets = *subscribed;
        for (const RID &rid: targets) {
            Hook &hook = hooks[rid];
            if (p_context.is_valid() && !(hook.context == p_context)) {
                continue;
            }
            hook.pending.push_back(p_arguments);
            // a subscription waiting in the queue takes the new report along when it runs
            events->event_queue(hook.event, p_activator);
        }
    }

    void ScenarioScriptServer::_on_vehicle_command_received(
            const RID &p_vehicle, const String &p_command, const Variant &p_p1, const Variant &p_p2) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_command});
        arguments.push_back({p_p1});
        arguments.push_back({p_p2});
        _deliver(SIGNAL_VEHICLE_COMMAND_RECEIVED, p_vehicle, arguments, p_vehicle);
    }

    void ScenarioScriptServer::_on_vehicle_heading_to_track_start(const RID &p_vehicle, const RID &p_track) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_vehicle, ScriptHandleKind::VEHICLE});
        _deliver(SIGNAL_VEHICLE_HEADING_TO_TRACK_START, p_track, arguments, p_vehicle);
    }

    void ScenarioScriptServer::_on_vehicle_heading_to_track_end(const RID &p_vehicle, const RID &p_track) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_vehicle, ScriptHandleKind::VEHICLE});
        _deliver(SIGNAL_VEHICLE_HEADING_TO_TRACK_END, p_track, arguments, p_vehicle);
    }

    void ScenarioScriptServer::_on_vehicle_stopped_on_track(const RID &p_vehicle, const RID &p_track) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_vehicle, ScriptHandleKind::VEHICLE});
        _deliver(SIGNAL_VEHICLE_STOPPED_ON_TRACK, p_track, arguments, p_vehicle);
    }

    void ScenarioScriptServer::_on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_vehicle, ScriptHandleKind::VEHICLE});
        _deliver(SIGNAL_ISOLATED_OCCUPIED, p_isolated, arguments, p_vehicle);
    }

    void ScenarioScriptServer::_on_isolated_freed(const RID &p_isolated, const RID &p_vehicle) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_vehicle, ScriptHandleKind::VEHICLE});
        _deliver(SIGNAL_ISOLATED_FREED, p_isolated, arguments, p_vehicle);
    }

    void ScenarioScriptServer::_on_switch_active_track_changed(const RID &p_track, const int p_active_track) {
        ERR_FAIL_INDEX(p_active_track, static_cast<int>(std::size(SWITCH_TRACK_NAMES)));
        Vector<ScriptArgument> arguments;
        arguments.push_back({SWITCH_TRACK_NAMES[p_active_track]});
        _deliver(SIGNAL_SWITCH_CHANGED, p_track, arguments, RID());
    }

    void ScenarioScriptServer::_on_signal_head_aspect_changed(const RID &p_signal_head, const StringName &p_aspect) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_aspect});
        _deliver(SIGNAL_SIGNAL_HEAD_ASPECT_CHANGED, p_signal_head, arguments, RID());
    }

    void ScenarioScriptServer::_on_event_launched(const RID &p_event, const RID &p_activator) {
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_activator, ScriptHandleKind::VEHICLE});
        _deliver(SIGNAL_EVENT_LAUNCHED, p_event, arguments, p_activator);
    }

    void ScenarioScriptServer::_on_memory_values_changed(const RID &p_memory) {
        _deliver(SIGNAL_MEMORY_VALUES_CHANGED, p_memory, Vector<ScriptArgument>(), RID());
    }

    /* Subscribed to by the cabin's vehicle (maszyna.cabin.on_control_changed(v, fn)) */
    void ScenarioScriptServer::_on_cabin_control_changed(
            const RID &p_cabin, const StringName &p_control_id, const Variant &p_value, const RID &p_context) {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        const RID vehicle = vehicles->cabin_get_vehicle(p_cabin);
        Vector<ScriptArgument> arguments;
        arguments.push_back({p_cabin, ScriptHandleKind::CABIN});
        arguments.push_back({p_control_id});
        arguments.push_back({p_value});
        _deliver(SIGNAL_CABIN_CONTROL_CHANGED, vehicle, arguments, vehicle, p_context);
    }
} // namespace godot
