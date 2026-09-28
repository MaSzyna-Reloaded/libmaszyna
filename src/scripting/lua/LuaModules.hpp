#pragma once
#include "lauxlib.h"
#include "lua.h"
#include "scripting/ScenarioScriptServer.hpp"
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The `maszyna` modules a script sees, each a table of functions on handles of the servers'
    /// RIDs (`maszyna.vehicle.send_command(vehicle, "main_switch", true)`). Every function goes
    /// through the public API of the server it names.
    class LuaModules {
        public:
            struct Module {
                    const char *name;
                    const luaL_Reg *functions;
            };

            /// maszyna.cancel()
            static const luaL_Reg ROOT[];
            static const luaL_Reg SIM[];
            static const luaL_Reg VEHICLE[];
            static const luaL_Reg CABIN[];
            static const luaL_Reg DRIVER[];
            static const luaL_Reg EVENT[];
            static const luaL_Reg MEMORY[];
            static const luaL_Reg TRACK[];
            static const luaL_Reg SIGNAL[];
            static const luaL_Reg LOG[];
            static const luaL_Reg PLAYER[];
            static const luaL_Reg CAMERA[];
            static const luaL_Reg HUD[];

            static constexpr const char *ROOT_NAME = "maszyna";
            static const Module MODULES[];

            /// on_<signal>(target, fn): fn(...) whenever the server reports the signal about the
            /// target (argument 1, a handle of p_target); returns the subscription
            static int
            subscribe(lua_State *p_state, ScenarioScriptServer::ScriptSignal p_signal, ScriptHandleKind p_target);

            /// The server singleton; raises a script error when it is not there
            template<typename T>
            static T *server(lua_State *p_state) {
                T *instance = T::get_instance();
                if (instance == nullptr) {
                    // luaL_error is the Lua C API's vararg error call
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    luaL_error(p_state, "%s is not available", String(T::get_class_static()).utf8().get_data());
                }
                return instance;
            }
    };
} // namespace godot
