#include "LuaHandle.hpp"
#include "lauxlib.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    static int64_t &handle_id(lua_State *p_state, const int p_index) {
        return *static_cast<int64_t *>(lua_touserdata(p_state, p_index));
    }

    /// Handles of one kind and one RID are equal (the metatables tell the kind)
    static int handle_equal(lua_State *p_state) {
        lua_getmetatable(p_state, 1);
        lua_getmetatable(p_state, 2);
        const bool same_kind = !(lua_rawequal(p_state, -1, -2) == 0);
        lua_pushboolean(p_state, static_cast<int>(same_kind && handle_id(p_state, 1) == handle_id(p_state, 2)));
        return 1;
    }

    static int handle_to_string(lua_State *p_state) {
        luaL_getmetafield(p_state, 1, "__name");
        const String text = String::utf8(lua_tostring(p_state, -1)) + ": " + String::num_int64(handle_id(p_state, 1));
        lua_pushstring(p_state, text.utf8().get_data());
        return 1;
    }

    void LuaHandle::register_types(lua_State *p_state) {
        for (const char *name: TYPE_NAMES) {
            luaL_newmetatable(p_state, name);
            lua_pushcfunction(p_state, handle_equal);
            lua_setfield(p_state, -2, "__eq");
            lua_pushcfunction(p_state, handle_to_string);
            lua_setfield(p_state, -2, "__tostring");
            // getmetatable() answers the name; the metatable itself stays out of the scripts' reach
            lua_pushstring(p_state, name);
            lua_setfield(p_state, -2, "__metatable");
            lua_pop(p_state, 1);
        }
    }

    void LuaHandle::push(lua_State *p_state, const RID &p_rid, const ScriptHandleKind p_kind) {
        if (!p_rid.is_valid()) {
            lua_pushnil(p_state);
            return;
        }
        *static_cast<int64_t *>(lua_newuserdatauv(p_state, sizeof(int64_t), 0)) = p_rid.get_id();
        luaL_setmetatable(p_state, TYPE_NAMES[static_cast<int>(p_kind)]);
    }

    RID LuaHandle::check(lua_State *p_state, const int p_index, const ScriptHandleKind p_kind) {
        const auto *id =
                static_cast<int64_t *>(luaL_checkudata(p_state, p_index, TYPE_NAMES[static_cast<int>(p_kind)]));
        return UtilityFunctions::rid_from_int64(*id);
    }

    RID LuaHandle::optional(lua_State *p_state, const int p_index, const ScriptHandleKind p_kind) {
        return lua_isnoneornil(p_state, p_index) ? RID() : check(p_state, p_index, p_kind);
    }

    RID LuaHandle::to_rid(lua_State *p_state, const int p_index) {
        for (const char *name: TYPE_NAMES) {
            if (!(luaL_testudata(p_state, p_index, name) == nullptr)) {
                return UtilityFunctions::rid_from_int64(handle_id(p_state, p_index));
            }
        }
        return RID();
    }
} // namespace godot
