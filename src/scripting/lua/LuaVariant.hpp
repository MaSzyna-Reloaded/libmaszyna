#pragma once
#include <godot_cpp/variant/variant.hpp>

#include "lua.h"

namespace godot {
    /// Godot values to Lua values and back. A RID becomes a handle of no particular kind (the
    /// modules push typed handles themselves), a vector a table {x, y, z}, an array a sequence and
    /// a resource a table of its stored properties. A Lua sequence becomes an Array, any other
    /// table a Dictionary.
    class LuaVariant {
        public:
            /// Deeper nesting is cut off - it also stops a table that contains itself
            static constexpr int MAX_CONVERSION_DEPTH = 16;

            static void push(lua_State *p_state, const Variant &p_value, int p_depth = 0);
            static Variant to_variant(lua_State *p_state, int p_index, int p_depth = 0);
    };
} // namespace godot
