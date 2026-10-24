#pragma once
#include "lua.h"
#include "scripting/ScenarioScriptServer.hpp"

namespace godot {
    /// `reloaded.memory`: the scenery's memory cells for scenario scripts - found by name, read and
    /// written, and their changes subscribed to. Open in every script context, before eu07.events,
    /// which is written on it.
    class MaszynaLuaMemoryModule {
        public:
            static constexpr const char *MODULE_NAME = "memory";
            /// The script type of a memory cell's handle (maszyna.Memory)
            static constexpr const char *HANDLE_KIND_NAME = "Memory";
            static constexpr const char *VALUES_CHANGED_SIGNAL_NAME = "memory_values_changed";

            /// Registered with ScenarioScriptServer by the plugin (register_module())
            static ScriptHandleKind handle_kind;
            static ScenarioScriptServer::ScriptSignal values_changed_signal;

            /// The handle kind, the signal and the module, from the plugin's register_classes()
            static void register_module();
            /// Opens the module in the state, as reloaded.memory and for require()
            static void open(lua_State *p_state);
            /// The memory's values changed: its subscribers' functions run
            static void deliver_values_changed(const RID &p_memory);
    };
} // namespace godot
