#pragma once
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
    /// What a handle a script holds stands for - the scripts see each kind as its own type, so a
    /// track cannot be passed where a vehicle is expected
    enum class ScriptHandleKind {
        NONE,
        VEHICLE,
        CABIN,
        EVENT,
        MEMORY,
        TRACK,
        ISOLATED,
        SIGNAL_HEAD,
        SIGNALLING_SYSTEM,
        SUBSCRIPTION,
    };

    /// A value handed to a script's function; a RID goes as a handle of its kind
    struct ScriptArgument {
            Variant value;
            ScriptHandleKind handle = ScriptHandleKind::NONE;
    };

    /// The language ScenarioScriptServer runs its scripts in. The server keeps what the scripts
    /// own and when they run; the runtime only loads code and calls it. Every method returns the
    /// script's error, empty when there was none.
    class ScriptRuntime {
        public:
            virtual ~ScriptRuntime() = default;
            /// Runs a file of the runtime's base directory
            virtual String run_file(const String &p_path) = 0;
            /// Runs a piece of code, named p_chunk_name in its errors
            virtual String run_source(const String &p_source, const String &p_chunk_name) = 0;
            /// Compiles a piece of code without running it
            virtual String check_source(const String &p_source, const String &p_chunk_name) = 0;
            /// Calls a function a script handed over, by the id the runtime gave it
            virtual String call(int64_t p_function, const Vector<ScriptArgument> &p_arguments) = 0;
            /// The function is no longer called; its id is invalid from now on
            virtual void release(int64_t p_function) = 0;
    };
} // namespace godot
