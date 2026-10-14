#include "./GameLogger.hpp"

namespace godot {
    void GameLogger::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_logger_id"), &GameLogger::get_logger_id);
        ClassDB::bind_method(D_METHOD("log", "loglevel", "line"), &GameLogger::log);
        ClassDB::bind_method(D_METHOD("debug", "line"), &GameLogger::debug);
        ClassDB::bind_method(D_METHOD("info", "line"), &GameLogger::info);
        ClassDB::bind_method(D_METHOD("warning", "line"), &GameLogger::warning);
        ClassDB::bind_method(D_METHOD("error", "line"), &GameLogger::error);
    }

    void GameLogger::setup(const String &p_logger_id) {
        logger_id = p_logger_id;
    }

    void GameLogger::add_handler(const Ref<GameLogHandler> &p_handler) {
        ERR_FAIL_COND(p_handler.is_null());
        handlers.push_back(p_handler);
    }

    void GameLogger::remove_handler(const Ref<GameLogHandler> &p_handler) {
        handlers.erase(p_handler);
    }

    String GameLogger::get_logger_id() const {
        return logger_id;
    }

    void GameLogger::log(const GameLog::LogLevel p_level, const String &p_line) {
        for (const Ref<GameLogHandler> &handler: handlers) {
            handler->handle(logger_id, p_level, p_line);
        }
    }

    void GameLogger::debug(const String &p_line) {
        log(GameLog::LogLevel::DEBUG, p_line);
    }

    void GameLogger::info(const String &p_line) {
        log(GameLog::LogLevel::INFO, p_line);
    }

    void GameLogger::warning(const String &p_line) {
        log(GameLog::LogLevel::WARNING, p_line);
    }

    void GameLogger::error(const String &p_line) {
        log(GameLog::LogLevel::ERROR, p_line);
    }
} // namespace godot
