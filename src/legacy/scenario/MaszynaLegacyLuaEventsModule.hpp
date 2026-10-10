#pragma once
#include "lua.h"

namespace godot {
    /// The original's scripting API, `eu07.events` (lua.cpp:17-42), for scenery scripts written
    /// against it. Written in Lua on the `maszyna` modules only - it adds nothing they lack - and
    /// open in every script context as the global `eu07.events` and as require("eu07.events").
    ///
    /// An event, a track, an isolated section, a memory cell and a vehicle (the original's
    /// dynobj) are the `maszyna` handles. A memory cell's values are the original's table
    /// {str =, num1 =, num2 =}.
    class MaszynaLegacyLuaEventsModule {
        public:
            static constexpr const char *MODULE_NAME = "eu07.events";

            /// Opens the module in the state; raises a script error when it fails
            static void open(lua_State *p_state);
    };
} // namespace godot
