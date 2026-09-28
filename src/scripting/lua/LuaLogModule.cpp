#include "LuaModules.hpp"
#include "logging/GameLogger.hpp"

namespace godot {
    static int log_at(lua_State *p_state, const GameLog::LogLevel p_level) {
        const String line = String::utf8(luaL_checkstring(p_state, 1));
        LuaModules::server<GameLog>(p_state)->get_logger(GameLog::GAME_LOGGER)->log(p_level, line);
        return 0;
    }

    static int log_debug(lua_State *p_state) {
        return log_at(p_state, GameLog::DEBUG);
    }

    static int log_info(lua_State *p_state) {
        return log_at(p_state, GameLog::INFO);
    }

    static int log_warning(lua_State *p_state) {
        return log_at(p_state, GameLog::WARNING);
    }

    static int log_error(lua_State *p_state) {
        return log_at(p_state, GameLog::ERROR);
    }

    const luaL_Reg LuaModules::LOG[] = {
            {"debug", log_debug}, {"info", log_info}, {"warning", log_warning},
            {"error", log_error}, {nullptr, nullptr},
    };
} // namespace godot
