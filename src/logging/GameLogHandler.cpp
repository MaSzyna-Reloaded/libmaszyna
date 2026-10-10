#include "./GameLogHandler.hpp"

namespace godot {
    void GameLogHandler::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_min_level", "min_level"), &GameLogHandler::set_min_level);
        ClassDB::bind_method(D_METHOD("get_min_level"), &GameLogHandler::get_min_level);
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "min_level", PROPERTY_HINT_ENUM, "Debug,Info,Warning,Error"),
                "set_min_level", "get_min_level");
        GDVIRTUAL_BIND(_handle, "logger_id", "loglevel", "line")
    }

    void GameLogHandler::set_min_level(const GameLog::LogLevel p_min_level) {
        min_level = p_min_level;
    }

    GameLog::LogLevel GameLogHandler::get_min_level() const {
        return min_level;
    }

    void GameLogHandler::handle(const String &p_logger_id, const GameLog::LogLevel p_level, const String &p_line) {
        if (p_level < min_level) {
            return;
        }
        _handle_line(p_logger_id, p_level, p_line);
    }

    void
    GameLogHandler::_handle_line(const String &p_logger_id, const GameLog::LogLevel p_level, const String &p_line) {
        GDVIRTUAL_CALL(_handle, p_logger_id, p_level, p_line);
    }
} // namespace godot
