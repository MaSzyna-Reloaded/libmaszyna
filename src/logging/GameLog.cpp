#include "./GameLog.hpp"
#include "./GameLogHandler.hpp"
#include "./GameLogger.hpp"

namespace godot {
    const char *GameLog::logger_created_signal = "logger_created";
    const char *GameLog::logger_removing_signal = "logger_removing";
    const char *GameLog::GAME_LOGGER = "game";

    void GameLog::_bind_methods() {
        ClassDB::bind_method(D_METHOD("create_logger", "logger_id"), &GameLog::create_logger);
        ClassDB::bind_method(D_METHOD("get_logger", "logger_id"), &GameLog::get_logger);
        ClassDB::bind_method(D_METHOD("get_loggers"), &GameLog::get_loggers);
        ClassDB::bind_method(D_METHOD("remove_logger", "logger_id"), &GameLog::remove_logger);
        ClassDB::bind_method(D_METHOD("register_handler", "handler_name", "handler"), &GameLog::register_handler);
        ClassDB::bind_method(D_METHOD("unregister_handler", "handler_name"), &GameLog::unregister_handler);
        ClassDB::bind_method(D_METHOD("assign_handler", "logger_id", "handler_name"), &GameLog::assign_handler);
        ClassDB::bind_method(D_METHOD("unassign_handler", "logger_id", "handler_name"), &GameLog::unassign_handler);
        ADD_SIGNAL(MethodInfo(logger_created_signal, PropertyInfo(Variant::STRING, "logger_id")));
        ADD_SIGNAL(MethodInfo(logger_removing_signal, PropertyInfo(Variant::STRING, "logger_id")));
        BIND_ENUM_CONSTANT(DEBUG);
        BIND_ENUM_CONSTANT(INFO);
        BIND_ENUM_CONSTANT(WARNING);
        BIND_ENUM_CONSTANT(ERROR);
    }

    GameLog::GameLog() {
        create_logger(GAME_LOGGER);
    }

    Ref<GameLogger> GameLog::create_logger(const String &p_logger_id) {
        ERR_FAIL_COND_V_MSG(loggers.has(p_logger_id), loggers[p_logger_id], "Logger already exists: " + p_logger_id);
        Ref<GameLogger> logger;
        logger.instantiate();
        logger->setup(p_logger_id);
        const PackedStringArray handler_names = assignments.get(p_logger_id, PackedStringArray());
        for (const String &handler_name: handler_names) {
            if (handlers.has(handler_name)) {
                logger->add_handler(handlers[handler_name]);
            }
        }
        loggers[p_logger_id] = logger;
        emit_signal(logger_created_signal, p_logger_id);
        return logger;
    }

    Ref<GameLogger> GameLog::get_logger(const String &p_logger_id) {
        if (loggers.has(p_logger_id)) {
            return loggers[p_logger_id];
        }
        return create_logger(p_logger_id);
    }

    PackedStringArray GameLog::get_loggers() const {
        return PackedStringArray(loggers.keys());
    }

    void GameLog::remove_logger(const String &p_logger_id) {
        ERR_FAIL_COND_MSG(!loggers.has(p_logger_id), "No logger: " + p_logger_id);
        emit_signal(logger_removing_signal, p_logger_id);
        // a script keeping the logger logs to nobody from now on
        const Ref<GameLogger> logger = loggers[p_logger_id];
        const PackedStringArray handler_names = assignments.get(p_logger_id, PackedStringArray());
        for (const String &handler_name: handler_names) {
            if (handlers.has(handler_name)) {
                logger->remove_handler(handlers[handler_name]);
            }
        }
        loggers.erase(p_logger_id);
    }

    void GameLog::register_handler(const String &p_handler_name, const Ref<GameLogHandler> &p_handler) {
        ERR_FAIL_COND(p_handler.is_null());
        ERR_FAIL_COND_MSG(handlers.has(p_handler_name), "Handler already registered: " + p_handler_name);
        handlers[p_handler_name] = p_handler;
        for (const String &logger_id: get_loggers()) {
            const PackedStringArray handler_names = assignments.get(logger_id, PackedStringArray());
            if (handler_names.has(p_handler_name)) {
                Ref<GameLogger>(loggers[logger_id])->add_handler(p_handler);
            }
        }
    }

    void GameLog::unregister_handler(const String &p_handler_name) {
        ERR_FAIL_COND_MSG(!handlers.has(p_handler_name), "No handler: " + p_handler_name);
        const Ref<GameLogHandler> handler = handlers[p_handler_name];
        for (const String &logger_id: get_loggers()) {
            Ref<GameLogger>(loggers[logger_id])->remove_handler(handler);
        }
        handlers.erase(p_handler_name);
    }

    void GameLog::assign_handler(const String &p_logger_id, const String &p_handler_name) {
        PackedStringArray handler_names = assignments.get(p_logger_id, PackedStringArray());
        ERR_FAIL_COND_MSG(
                handler_names.has(p_handler_name), "Handler " + p_handler_name + " already assigned to " + p_logger_id);
        handler_names.append(p_handler_name);
        assignments[p_logger_id] = handler_names;
        if (loggers.has(p_logger_id) && handlers.has(p_handler_name)) {
            Ref<GameLogger>(loggers[p_logger_id])->add_handler(handlers[p_handler_name]);
        }
    }

    void GameLog::unassign_handler(const String &p_logger_id, const String &p_handler_name) {
        PackedStringArray handler_names = assignments.get(p_logger_id, PackedStringArray());
        ERR_FAIL_COND_MSG(
                !handler_names.has(p_handler_name), "Handler " + p_handler_name + " not assigned to " + p_logger_id);
        handler_names.erase(p_handler_name);
        assignments[p_logger_id] = handler_names;
        if (loggers.has(p_logger_id) && handlers.has(p_handler_name)) {
            Ref<GameLogger>(loggers[p_logger_id])->remove_handler(handlers[p_handler_name]);
        }
    }

} // namespace godot
