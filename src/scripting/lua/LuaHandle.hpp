#pragma once
#include "scripting/ScriptRuntime.hpp"
#include <godot_cpp/variant/rid.hpp>

#include "lua.h"

namespace godot {
    /// A RID as a script holds it: a userdata of its kind (maszyna.Vehicle, maszyna.Track, ...),
    /// so that a function taking a vehicle refuses a track. Two handles of one RID are equal.
    class LuaHandle {
        public:
            /// The type names, in the order of ScriptHandleKind
            static constexpr const char *TYPE_NAMES[] = {
                    "maszyna.RID",          "maszyna.Vehicle",    "maszyna.Cabin",
                    "maszyna.Event",        "maszyna.Memory",     "maszyna.Track",
                    "maszyna.Isolated",     "maszyna.SignalHead", "maszyna.SignallingSystem",
                    "maszyna.Subscription",
            };

            static void register_types(lua_State *p_state);
            /// Pushes the handle; nil for an invalid RID
            static void push(lua_State *p_state, const RID &p_rid, ScriptHandleKind p_kind);
            /// The RID of the handle of the kind at p_index; raises an error for anything else
            static RID check(lua_State *p_state, int p_index, ScriptHandleKind p_kind);
            /// ...or an invalid RID for nil and none
            static RID optional(lua_State *p_state, int p_index, ScriptHandleKind p_kind);
            /// The RID of a handle of any kind at p_index; an invalid RID when it is no handle
            static RID to_rid(lua_State *p_state, int p_index);
    };
} // namespace godot
