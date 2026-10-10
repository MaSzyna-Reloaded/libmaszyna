#pragma once
#include "ScenarioScriptCabinImplementation.hpp"
#include "ScriptRuntime.hpp"
#include <functional>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// Runs a scenario's scripts (the original's `lua <file>`, simulationstateserializer.cpp:346).
    ///
    /// A context is one scenario's script environment: the code loaded into it, and everything
    /// its scripts made - the events they created, their timers and their subscriptions to what
    /// the servers report. context_free() takes all of it along. The scripts reach the simulation
    /// only through the servers' own APIs; no server knows the scripts exist.
    ///
    /// A script's function never runs inside the server that reported something: a timer, an
    /// event and every subscription run as a ScenarioEventServer event, so they follow the
    /// simulation's clock and stop while it is paused, as the scenery's own events do.
    ///
    /// The code a context runs is grouped in units - a file, or what an editor applied under a
    /// name. Applying a unit again first frees whatever its previous run made.
    class ScenarioScriptServer : public Object {
            GDCLASS(ScenarioScriptServer, Object)
            friend class ScenarioScriptAction;

        public:
            /// Whether a timer runs once or keeps running
            enum Timer {
                TIMER_ONCE,
                TIMER_REPEATING,
            };

            /// What a server reports that a script may subscribe to, each about one target
            enum ScriptSignal {
                /// A vehicle received a command (target: the vehicle)
                SIGNAL_VEHICLE_COMMAND_RECEIVED,
                /// A control of a vehicle's cab changed (target: the vehicle)
                SIGNAL_CABIN_CONTROL_CHANGED,
                /// A vehicle moves along the track towards its start (target: the track)
                SIGNAL_VEHICLE_HEADING_TO_TRACK_START,
                /// ...towards its end
                SIGNAL_VEHICLE_HEADING_TO_TRACK_END,
                /// A vehicle stopped on the track
                SIGNAL_VEHICLE_STOPPED_ON_TRACK,
                /// The first vehicle came onto the isolated section (target: the section)
                SIGNAL_ISOLATED_OCCUPIED,
                /// The last vehicle left the isolated section
                SIGNAL_ISOLATED_FREED,
                /// The switch moved to another track (target: the switch)
                SIGNAL_SWITCH_CHANGED,
                /// The signal head shows another aspect (target: the head)
                SIGNAL_SIGNAL_HEAD_ASPECT_CHANGED,
                /// The event ran (target: the event)
                SIGNAL_EVENT_LAUNCHED,
                /// The memory's values changed (target: the memory)
                SIGNAL_MEMORY_VALUES_CHANGED,
                SIGNAL_MAX,
            };

            /// Which of its functions an event runs: its condition passed, or it failed
            enum Branch {
                BRANCH_RUN,
                BRANCH_ELSE,
            };

            /// The tracks of a switch, as a script names them (TrackServer::SwitchTrack)
            static constexpr const char *SWITCH_TRACK_NAMES[] = {"common", "diverging"};
            /// No function of the script
            static constexpr int64_t NO_FUNCTION = -1;
            /// What a context says when the build has no runtime (simulationstateserializer.cpp:355)
            static constexpr const char *NO_RUNTIME_MESSAGE = "lua scripts not supported in this build.";

            /// A script failed (context: RID, message: String) - it is logged as well
            static const char *script_error_signal;

            static ScenarioScriptServer *get_instance() {
                return Object::cast_to<ScenarioScriptServer>(
                        Engine::get_singleton()->get_singleton("ScenarioScriptServer"));
            }

        private:
            enum HookKind {
                /// An event a script created
                HOOK_EVENT,
                HOOK_TIMER_ONCE,
                HOOK_TIMER_REPEATING,
                HOOK_SUBSCRIPTION,
            };

            /// Something a script made that runs one of its functions
            struct Hook {
                    RID context;
                    StringName unit;
                    HookKind kind = HOOK_EVENT;
                    ScriptSignal signal = SIGNAL_MAX;
                    RID target;
                    /// The ScenarioEventServer event that runs the function
                    RID event;
                    /// ...and the launcher that queues it, for a repeating timer
                    RID launcher;
                    int64_t function = NO_FUNCTION;
                    int64_t else_function = NO_FUNCTION;
                    /// What the servers reported for a subscription since its event last ran
                    Vector<Vector<ScriptArgument>> pending;
            };

            struct Context {
                    /// Null in a build without a runtime
                    ScriptRuntime *runtime = nullptr;
                    Ref<ScenarioScriptCabinImplementation> cabin;
                    Vector<RID> hooks;
                    /// The unit whose code is running, which owns what it makes
                    StringName active_unit;
            };

            HashMap<RID, Context> contexts;
            HashMap<RID, Hook> hooks;
            /// The subscriptions of each signal, by target
            HashMap<RID, Vector<RID>> subscriptions[SIGNAL_MAX];

            RID _create_hook(const RID &p_context, HookKind p_kind, int64_t p_function);
            void _free_hook(const RID &p_hook);
            /// Called by ScenarioScriptAction when the hook's event runs
            void _run_hook(const RID &p_hook, const RID &p_event, const RID &p_activator, Branch p_branch);
            /// Runs code of the context as the unit, reports its error; false when it failed
            bool
            _enter(const RID &p_context, const StringName &p_unit,
                   const std::function<String(ScriptRuntime *)> &p_code);
            void _report(const RID &p_context, const String &p_error);
            /// Hands the arguments to the target's subscriptions and queues their events; only to
            /// the subscriptions of p_context when it is valid
            void _deliver(
                    ScriptSignal p_signal, const RID &p_target, const Vector<ScriptArgument> &p_arguments,
                    const RID &p_activator, const RID &p_context = RID());

            void _on_vehicle_command_received(
                    const RID &p_vehicle, const String &p_command, const Variant &p_p1, const Variant &p_p2);
            void _on_vehicle_heading_to_track_start(const RID &p_vehicle, const RID &p_track);
            void _on_vehicle_heading_to_track_end(const RID &p_vehicle, const RID &p_track);
            void _on_vehicle_stopped_on_track(const RID &p_vehicle, const RID &p_track);
            void _on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_freed(const RID &p_isolated, const RID &p_vehicle);
            void _on_switch_active_track_changed(const RID &p_track, int p_active_track);
            void _on_signal_head_aspect_changed(const RID &p_signal_head, const StringName &p_aspect);
            void _on_event_launched(const RID &p_event, const RID &p_activator);
            void _on_memory_values_changed(const RID &p_memory);
            void _on_cabin_control_changed(
                    const RID &p_cabin, const StringName &p_control_id, const Variant &p_value, const RID &p_context);

        protected:
            static void _bind_methods();

        public:
            ScenarioScriptServer();

            /// A context whose scripts load their files from p_base_dir
            RID context_create(const String &p_base_dir);
            void context_free(const RID &p_context);
            TypedArray<RID> context_get_rids() const;
            /// Runs a file of the base directory as the unit named after it; false when it failed
            bool context_run_file(const RID &p_context, const String &p_path);
            /// Compiles the code of the unit without running it; returns the error, empty when there
            /// is none
            String context_check_source(const RID &p_context, const StringName &p_unit, const String &p_source);
            /// Frees what the unit made, then runs the code as the unit; false when it failed
            bool context_apply_source(const RID &p_context, const StringName &p_unit, const String &p_source);
            void context_attach_cabin_implementation(
                    const RID &p_context, const Ref<ScenarioScriptCabinImplementation> &p_cabin);
            Ref<ScenarioScriptCabinImplementation> context_get_cabin_implementation(const RID &p_context) const;

            // What the runtimes call for the scripts; what they make belongs to the running unit

            /// An event that runs the functions; the script owns it
            RID script_event_create(const RID &p_context, int64_t p_function, int64_t p_else_function);
            /// Frees an event the script created; false when it did not create it
            bool script_event_free(const RID &p_context, const RID &p_event);
            /// Runs the function after p_seconds of simulation time, once or on and on
            RID script_timer_create(const RID &p_context, double p_seconds, int64_t p_function, Timer p_timer);
            RID script_subscribe(const RID &p_context, ScriptSignal p_signal, const RID &p_target, int64_t p_function);
            /// Stops a timer or a subscription; false when it is not the script's
            bool script_cancel(const RID &p_context, const RID &p_subscription);
    };
} // namespace godot
