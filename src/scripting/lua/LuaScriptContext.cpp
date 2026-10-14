#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "LuaVariant.hpp"
#include "lauxlib.h"
#include "legacy/MaszynaDataPath.hpp"
#include "legacy/scenario/MaszynaLegacyLuaEventsModule.hpp"
#include "logging/GameLogger.hpp"
#include "lualib.h"
#include "utils/LibMaszynaUnits.hpp"
#include <cstdlib>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /// The libraries a script gets (linit.c's list, without io, os, debug and package)
    static const luaL_Reg LIBRARIES[] = {
            {LUA_GNAME, luaopen_base},        {LUA_COLIBNAME, luaopen_coroutine}, {LUA_TABLIBNAME, luaopen_table},
            {LUA_STRLIBNAME, luaopen_string}, {LUA_MATHLIBNAME, luaopen_math},    {LUA_UTF8LIBNAME, luaopen_utf8},
    };

    /// Runs the function a call() pushed, with its arguments - inside the protected call, so
    /// that running out of memory while pushing them is a script error too
    static int call_with_arguments(lua_State *p_state) {
        const auto *arguments = static_cast<const Vector<ScriptArgument> *>(lua_touserdata(p_state, 1));
        const lua_Integer function = lua_tointeger(p_state, 2);
        lua_settop(p_state, 0);
        luaL_checkstack(p_state, static_cast<int>(arguments->size()) + 1, nullptr);
        lua_rawgeti(p_state, LUA_REGISTRYINDEX, function);
        for (const ScriptArgument &argument: *arguments) {
            if (argument.handle == ScriptHandleKind::NONE) {
                LuaVariant::push(p_state, argument.value);
            } else {
                LuaHandle::push(p_state, argument.value, argument.handle);
            }
        }
        lua_call(p_state, static_cast<int>(arguments->size()), 0);
        return 0;
    }

    LuaScriptContext::LuaScriptContext(const RID &p_context, const String &p_base_dir) :
        context(p_context), base_dir(p_base_dir) {
        random_generator.instantiate();
        random_generator->randomize();
        // not in the initializer: the allocator counts into memory_used, which is declared (and so
        // initialised) after state
        state = lua_newstate(_allocate, this); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        ERR_FAIL_NULL(state);
        // lua_getextraspace is the Lua C API's macro: pointer arithmetic with a void pointer result
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-casting-through-void)
        *static_cast<LuaScriptContext **>(lua_getextraspace(state)) = this;
        lua_sethook(state, _count_instructions, LUA_MASKCOUNT, INSTRUCTION_CHECK_STEP);
        lua_pushcfunction(state, _open_libraries);
        const String error = _protected_call(0);
        ERR_FAIL_COND_MSG(!error.is_empty(), "Cannot open the script libraries: " + error);
    }

    LuaScriptContext::~LuaScriptContext() {
        if (!(state == nullptr)) {
            lua_close(state);
        }
    }

    LuaScriptContext *LuaScriptContext::from_state(lua_State *p_state) {
        // lua_getextraspace is the Lua C API's macro: pointer arithmetic with a void pointer result
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-casting-through-void)
        return *static_cast<LuaScriptContext **>(lua_getextraspace(p_state));
    }

    RID LuaScriptContext::get_rid() const {
        return context;
    }

    int64_t LuaScriptContext::keep_function(lua_State *p_state, const int p_index) {
        luaL_checktype(p_state, p_index, LUA_TFUNCTION);
        lua_pushvalue(p_state, p_index);
        return luaL_ref(p_state, LUA_REGISTRYINDEX);
    }

    String LuaScriptContext::run_file(const String &p_path) {
        if (!_load_file(state, p_path)) {
            const String error = String::utf8(lua_tostring(state, -1));
            lua_pop(state, 1);
            return error;
        }
        return _protected_call(0);
    }

    String LuaScriptContext::run_source(const String &p_source, const String &p_chunk_name) {
        const CharString source = p_source.utf8();
        const CharString chunk_name = ("=" + p_chunk_name).utf8();
        if (!(luaL_loadbufferx(state, source.get_data(), source.length(), chunk_name.get_data(), "t") == LUA_OK)) {
            const String error = String::utf8(lua_tostring(state, -1));
            lua_pop(state, 1);
            return error;
        }
        return _protected_call(0);
    }

    String LuaScriptContext::check_source(const String &p_source, const String &p_chunk_name) {
        const CharString source = p_source.utf8();
        const CharString chunk_name = ("=" + p_chunk_name).utf8();
        const bool compiled =
                luaL_loadbufferx(state, source.get_data(), source.length(), chunk_name.get_data(), "t") == LUA_OK;
        const String error = compiled ? String() : String::utf8(lua_tostring(state, -1));
        lua_pop(state, 1);
        return error;
    }

    String LuaScriptContext::call(const int64_t p_function, const Vector<ScriptArgument> &p_arguments) {
        lua_pushcfunction(state, call_with_arguments);
        lua_pushlightuserdata(state, const_cast<Vector<ScriptArgument> *>(&p_arguments));
        lua_pushinteger(state, p_function);
        return _protected_call(2);
    }

    void LuaScriptContext::release(const int64_t p_function) {
        luaL_unref(state, LUA_REGISTRYINDEX, static_cast<int>(p_function));
    }

    // --- the state ---

    /// A block that would take the state past MAX_MEMORY_BYTES is refused; Lua raises that as a
    /// memory error of the script
    void *
    LuaScriptContext::_allocate(void *p_context, void *p_block, const size_t p_old_size, const size_t p_new_size) {
        auto *self = static_cast<LuaScriptContext *>(p_context);
        // without a block, the old size is the kind of object being made (lua_Alloc)
        const size_t old_size = p_block == nullptr ? 0 : p_old_size;
        if (p_new_size == 0) {
            std::free(p_block); // NOLINT(cppcoreguidelines-no-malloc) - lua_Alloc frees with free()
            self->memory_used -= old_size;
            return nullptr;
        }
        if (p_new_size > old_size && self->memory_used - old_size + p_new_size > MAX_MEMORY_BYTES) {
            return nullptr;
        }
        // lua_Alloc's contract is realloc()'s
        void *block = std::realloc(p_block, p_new_size); // NOLINT(cppcoreguidelines-no-malloc)
        if (!(block == nullptr)) {
            self->memory_used = self->memory_used - old_size + p_new_size;
        }
        return block;
    }

    void LuaScriptContext::_count_instructions(lua_State *p_state, lua_Debug * /* p_debug */) {
        LuaScriptContext *self = from_state(p_state);
        self->instructions += INSTRUCTION_CHECK_STEP;
        if (self->instructions > MAX_INSTRUCTIONS_PER_CALL) {
            // luaL_error is the Lua C API's vararg error call
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            luaL_error(
                    p_state, "the script runs too long (over %I instructions)",
                    static_cast<lua_Integer>(MAX_INSTRUCTIONS_PER_CALL));
        }
    }

    int LuaScriptContext::_open_libraries(lua_State *p_state) {
        for (const luaL_Reg &library: LIBRARIES) {
            luaL_requiref(p_state, library.name, library.func, 1);
            lua_pop(p_state, 1);
        }

        // what reaches past the simulation goes; what stays reads only the scripts' own files
        lua_pushnil(p_state);
        lua_setglobal(p_state, "dofile");
        lua_pushnil(p_state);
        lua_setglobal(p_state, "loadfile");
        lua_register(p_state, "load", _load);
        lua_register(p_state, "collectgarbage", _collect_garbage);
        lua_register(p_state, "print", _print);
        lua_register(p_state, "require", _require);
        lua_getglobal(p_state, LUA_STRLIBNAME);
        lua_pushnil(p_state);
        lua_setfield(p_state, -2, "dump");
        lua_pop(p_state, 1);
        lua_getglobal(p_state, LUA_MATHLIBNAME);
        lua_pushcfunction(p_state, _math_random);
        lua_setfield(p_state, -2, "random");
        lua_pushcfunction(p_state, _math_random_seed);
        lua_setfield(p_state, -2, "randomseed");
        lua_pop(p_state, 1);

        LuaHandle::register_types(p_state);

        // maszyna and maszyna.<module>, as globals and for require()
        luaL_getsubtable(p_state, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_newtable(p_state);
        luaL_setfuncs(p_state, LuaModules::ROOT, 0);
        // MODULES is a table of unknown bound ended by a null entry, as luaL_Reg lists are
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (const LuaModules::Module *module = LuaModules::MODULES; !(module->name == nullptr); module++) {
            lua_newtable(p_state);
            luaL_setfuncs(p_state, module->functions, 0);
            lua_pushvalue(p_state, -1);
            lua_setfield(p_state, -4, (String(LuaModules::ROOT_NAME) + "." + module->name).utf8().get_data());
            lua_setfield(p_state, -2, module->name);
        }
        lua_pushvalue(p_state, -1);
        lua_setfield(p_state, -3, LuaModules::ROOT_NAME);
        lua_setglobal(p_state, LuaModules::ROOT_NAME);
        lua_pop(p_state, 1);

        MaszynaLegacyLuaEventsModule::open(p_state);
        return 0;
    }

    /// Adds where the error happened (the debug library is not open to the scripts)
    int LuaScriptContext::_traceback(lua_State *p_state) {
        const char *message = luaL_tolstring(p_state, 1, nullptr);
        luaL_traceback(p_state, p_state, message, 1);
        return 1;
    }

    /// print() writes to the game log
    int LuaScriptContext::_print(lua_State *p_state) {
        String line;
        const int count = lua_gettop(p_state);
        for (int i = 1; i <= count; i++) {
            size_t length = 0;
            const char *text = luaL_tolstring(p_state, i, &length);
            line += (i > 1 ? "\t" : "") + String::utf8(text, static_cast<int>(length));
            lua_pop(p_state, 1);
        }
        GameLog *log = LuaModules::server<GameLog>(p_state);
        log->get_logger(GameLog::GAME_LOGGER)->info(line);
        return 0;
    }

    /// load() of text only, never of a binary chunk
    int LuaScriptContext::_load(lua_State *p_state) {
        size_t length = 0;
        const char *chunk = luaL_checklstring(p_state, 1, &length);
        const char *chunk_name = luaL_optstring(p_state, 2, chunk);
        const bool has_environment = !lua_isnone(p_state, 4);
        if (!(luaL_loadbufferx(p_state, chunk, length, chunk_name, "t") == LUA_OK)) {
            lua_pushnil(p_state);
            lua_insert(p_state, -2);
            return 2;
        }
        if (has_environment) {
            lua_pushvalue(p_state, 4);
            // the chunk's first upvalue is its _ENV
            if (lua_setupvalue(p_state, -2, 1) == nullptr) {
                lua_pop(p_state, 1);
            }
        }
        return 1;
    }

    /// collectgarbage("count") only - the script does not run the collector
    int LuaScriptContext::_collect_garbage(lua_State *p_state) {
        static constexpr const char *COUNT_OPTION = "count";
        const String option = luaL_optstring(p_state, 1, COUNT_OPTION);
        luaL_argcheck(p_state, option == COUNT_OPTION, 1, "only \"count\" is available");
        // lua_gc is the Lua C API's vararg call
        const int kilobytes = lua_gc(p_state, LUA_GCCOUNT); // NOLINT(cppcoreguidelines-pro-type-vararg)
        const int bytes = lua_gc(p_state, LUA_GCCOUNTB);    // NOLINT(cppcoreguidelines-pro-type-vararg)
        lua_pushnumber(p_state, kilobytes + (static_cast<double>(bytes) / LibMaszynaUnits::BYTES_PER_KILOBYTE));
        return 1;
    }

    /// math.random() as the manual has it, drawn from the state's own generator
    int LuaScriptContext::_math_random(lua_State *p_state) {
        const Ref<RandomNumberGenerator> &generator = from_state(p_state)->random_generator;
        switch (lua_gettop(p_state)) {
            case 0:
                lua_pushnumber(p_state, generator->randf());
                return 1;
            case 1: {
                const lua_Integer upper = luaL_checkinteger(p_state, 1);
                luaL_argcheck(p_state, upper >= 1, 1, "interval is empty");
                lua_pushinteger(p_state, generator->randi_range(1, static_cast<int32_t>(upper)));
                return 1;
            }
            case 2: {
                const lua_Integer lower = luaL_checkinteger(p_state, 1);
                const lua_Integer upper = luaL_checkinteger(p_state, 2);
                luaL_argcheck(p_state, lower <= upper, 2, "interval is empty");
                lua_pushinteger(
                        p_state, generator->randi_range(static_cast<int32_t>(lower), static_cast<int32_t>(upper)));
                return 1;
            }
            default:
                // luaL_error is the Lua C API's vararg error call
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                return luaL_error(p_state, "wrong number of arguments");
        }
    }

    int LuaScriptContext::_math_random_seed(lua_State *p_state) {
        from_state(p_state)->random_generator->set_seed(luaL_checkinteger(p_state, 1));
        return 0;
    }

    /// require("a.b") loads a/b.lua of the base directory once; the built-in modules come first
    int LuaScriptContext::_require(lua_State *p_state) {
        const char *name = luaL_checkstring(p_state, 1);
        lua_settop(p_state, 1);
        luaL_getsubtable(p_state, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_getfield(p_state, 2, name);
        if (!(lua_toboolean(p_state, -1) == 0)) {
            return 1;
        }
        lua_pop(p_state, 1);
        if (!from_state(p_state)->_load_file(p_state, String::utf8(name).replace(".", "/") + SCRIPT_EXTENSION)) {
            return lua_error(p_state);
        }
        lua_pushvalue(p_state, 1);
        lua_call(p_state, 1, 1);
        if (lua_isnil(p_state, -1)) {
            lua_pop(p_state, 1);
            lua_pushboolean(p_state, 1);
        }
        lua_pushvalue(p_state, -1);
        lua_setfield(p_state, 2, name);
        return 1;
    }

    /// Only a file of the base directory: no absolute path, no drive, no way up
    bool LuaScriptContext::_load_file(lua_State *p_state, const String &p_path) {
        const String relative = p_path.replace("\\", "/");
        const PackedStringArray parts = relative.split("/");
        bool inside = !relative.is_absolute_path() && !relative.contains(":");
        for (int64_t i = 0; i < parts.size(); i++) {
            inside = inside && !parts[i].is_empty() && !(parts[i] == "..");
        }
        if (!inside) {
            lua_pushstring(p_state, ("cannot open " + p_path + ": outside the scenery directory").utf8().get_data());
            return false;
        }
        const String path = base_dir.path_join(MaszynaDataPath::resolve(base_dir, relative));
        if (!FileAccess::file_exists(path)) {
            lua_pushstring(p_state, ("cannot open " + p_path).utf8().get_data());
            return false;
        }
        const PackedByteArray code = FileAccess::get_file_as_bytes(path);
        const CharString chunk_name = ("@" + p_path).utf8();
        return luaL_loadbufferx(
                       p_state, reinterpret_cast<const char *>(code.ptr()), code.size(), chunk_name.get_data(), "t") ==
               LUA_OK;
    }

    String LuaScriptContext::_protected_call(const int p_arguments) {
        const int handler = lua_gettop(state) - p_arguments;
        lua_pushcfunction(state, _traceback);
        lua_insert(state, handler);
        instructions = 0;
        const int status = lua_pcall(state, p_arguments, 0, handler);
        lua_remove(state, handler);
        if (status == LUA_OK) {
            return String();
        }
        const String error = String::utf8(lua_tostring(state, -1));
        lua_pop(state, 1);
        return error;
    }
} // namespace godot
