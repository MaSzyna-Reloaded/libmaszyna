#include "LuaHandle.hpp"
#include "LuaVariant.hpp"
#include "lauxlib.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    /// A stack slot per nesting level: the table, a key and a value
    static constexpr int STACK_SLOTS_PER_LEVEL = 3;
    /// x, y and z
    static constexpr int VECTOR_FIELDS = 3;

    static void push_string(lua_State *p_state, const String &p_text) {
        const CharString text = p_text.utf8();
        lua_pushlstring(p_state, text.get_data(), text.length());
    }

    static void push_vector(lua_State *p_state, const real_t p_x, const real_t p_y, const Variant &p_z) {
        lua_createtable(p_state, 0, VECTOR_FIELDS);
        lua_pushnumber(p_state, p_x);
        lua_setfield(p_state, -2, "x");
        lua_pushnumber(p_state, p_y);
        lua_setfield(p_state, -2, "y");
        if (p_z.get_type() == Variant::NIL) {
            return;
        }
        lua_pushnumber(p_state, static_cast<double>(p_z));
        lua_setfield(p_state, -2, "z");
    }

    void LuaVariant::push(lua_State *p_state, const Variant &p_value, const int p_depth) {
        if (p_depth > MAX_CONVERSION_DEPTH) {
            lua_pushnil(p_state);
            return;
        }
        luaL_checkstack(p_state, STACK_SLOTS_PER_LEVEL, nullptr);
        switch (p_value.get_type()) {
            case Variant::NIL:
                lua_pushnil(p_state);
                return;
            case Variant::BOOL:
                lua_pushboolean(p_state, static_cast<int>(static_cast<bool>(p_value)));
                return;
            case Variant::INT:
                lua_pushinteger(p_state, static_cast<int64_t>(p_value));
                return;
            case Variant::FLOAT:
                lua_pushnumber(p_state, static_cast<double>(p_value));
                return;
            case Variant::STRING:
            case Variant::STRING_NAME:
            case Variant::NODE_PATH:
                push_string(p_state, p_value);
                return;
            case Variant::RID:
                LuaHandle::push(p_state, p_value, ScriptHandleKind::NONE);
                return;
            case Variant::VECTOR2: {
                const Vector2 vector = p_value;
                push_vector(p_state, vector.x, vector.y, Variant());
                return;
            }
            case Variant::VECTOR3: {
                const Vector3 vector = p_value;
                push_vector(p_state, vector.x, vector.y, vector.z);
                return;
            }
            case Variant::DICTIONARY: {
                const Dictionary dictionary = p_value;
                lua_createtable(p_state, 0, static_cast<int>(dictionary.size()));
                const Array keys = dictionary.keys();
                for (int64_t i = 0; i < keys.size(); i++) {
                    push(p_state, keys[i], p_depth + 1);
                    push(p_state, dictionary[keys[i]], p_depth + 1);
                    // a key that could not be converted is left out
                    if (lua_isnil(p_state, -2)) {
                        lua_pop(p_state, 2);
                        continue;
                    }
                    lua_settable(p_state, -3);
                }
                return;
            }
            case Variant::OBJECT: {
                // a resource (a timetable) as the values it keeps; any other object is nothing a
                // script could hold
                const Resource *resource = Object::cast_to<Resource>(p_value);
                if (resource == nullptr) {
                    lua_pushnil(p_state);
                    return;
                }
                lua_newtable(p_state);
                const TypedArray<Dictionary> properties = resource->get_property_list();
                for (int64_t i = 0; i < properties.size(); i++) {
                    const Dictionary property = properties[i];
                    const String name = property["name"];
                    const int64_t usage = property["usage"];
                    if ((usage & PROPERTY_USAGE_STORAGE) == 0 || name.begins_with("resource_") || name == "script") {
                        continue;
                    }
                    push(p_state, resource->get(name), p_depth + 1);
                    lua_setfield(p_state, -2, name.utf8().get_data());
                }
                return;
            }
            default:
                break;
        }
        if (p_value.get_type() >= Variant::ARRAY) {
            // Array and every packed array
            const Array array = p_value;
            lua_createtable(p_state, static_cast<int>(array.size()), 0);
            for (int64_t i = 0; i < array.size(); i++) {
                push(p_state, array[i], p_depth + 1);
                lua_rawseti(p_state, -2, i + 1);
            }
            return;
        }
        push_string(p_state, p_value.stringify());
    }

    Variant LuaVariant::to_variant(lua_State *p_state, int p_index, const int p_depth) {
        p_index = lua_absindex(p_state, p_index);
        switch (lua_type(p_state, p_index)) {
            case LUA_TBOOLEAN:
                return !(lua_toboolean(p_state, p_index) == 0);
            case LUA_TNUMBER:
                if (lua_isinteger(p_state, p_index) == 0) {
                    return lua_tonumber(p_state, p_index);
                }
                return static_cast<int64_t>(lua_tointeger(p_state, p_index));
            case LUA_TSTRING: {
                size_t length = 0;
                const char *text = lua_tolstring(p_state, p_index, &length);
                return String::utf8(text, static_cast<int>(length));
            }
            case LUA_TUSERDATA:
                return LuaHandle::to_rid(p_state, p_index);
            case LUA_TTABLE:
                break;
            default:
                return Variant();
        }
        if (p_depth > MAX_CONVERSION_DEPTH) {
            return Variant();
        }
        luaL_checkstack(p_state, STACK_SLOTS_PER_LEVEL, nullptr);
        // a sequence 1..n and nothing else is an array
        const lua_Unsigned length = lua_rawlen(p_state, p_index);
        lua_Unsigned keys = 0;
        lua_pushnil(p_state);
        while (!(lua_next(p_state, p_index) == 0)) {
            keys++;
            lua_pop(p_state, 1);
        }
        if (length > 0 && keys == length) {
            Array array;
            for (lua_Unsigned i = 1; i <= length; i++) {
                lua_rawgeti(p_state, p_index, static_cast<lua_Integer>(i));
                array.push_back(to_variant(p_state, -1, p_depth + 1));
                lua_pop(p_state, 1);
            }
            return array;
        }
        Dictionary dictionary;
        lua_pushnil(p_state);
        while (!(lua_next(p_state, p_index) == 0)) {
            dictionary[to_variant(p_state, -2, p_depth + 1)] = to_variant(p_state, -1, p_depth + 1);
            lua_pop(p_state, 1);
        }
        return dictionary;
    }
} // namespace godot
