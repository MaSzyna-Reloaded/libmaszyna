#pragma once

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <map>

#ifdef LIBMASZYNA_DEBUG
#if defined(_MSC_VER)
// MSVC doesn't require special handling.
#define DEBUG(msg, ...)                                                                                                \
    UtilityFunctions::print_rich("[color=orange](debug) " + vformat(String(msg), Array::make(__VA_ARGS__)) + "[/color]")
#elif defined(__cplusplus) && __cplusplus >= 202002L
// C++20 with __VA_OPT__
#define DEBUG(msg, ...)                                                                                                \
    UtilityFunctions::print_rich(                                                                                      \
            "[color=orange](debug) " + vformat(String(msg) __VA_OPT__(, ) Array::make(__VA_ARGS__)) + "[/color]")
#else
// Use a workaround for C++11 to C++17
#define DEBUG(msg, ...)                                                                                                \
    UtilityFunctions::print_rich("[color=orange](debug) " + vformat(String(msg), ##__VA_ARGS__) + "[/color]")
#endif
#else
#define DEBUG(msg, ...)
#endif

namespace godot {
    class GameLogger;
    class GameLogHandler;

    /// The game's log, as Python's logging: named loggers (get_logger()), no hierarchy. GameLog only
    /// manages them: the handlers are registered by name (register_handler()) and assigned to the
    /// loggers by name (assign_handler()), and GameLog attaches a logger's handlers to it - one
    /// handler may serve many loggers. A logger calls its handlers itself; nothing is kept in
    /// memory.
    class GameLog : public Object {
            GDCLASS(GameLog, Object);

        public:
            static const char *logger_created_signal;
            static const char *logger_removing_signal;
            /// The logger of the game's own messages, made with the log
            static const char *GAME_LOGGER;

            static GameLog *get_instance() {
                return dynamic_cast<GameLog *>(Engine::get_singleton()->get_singleton("GameLog"));
            }

            enum LogLevel {
                DEBUG = 0,
                INFO,
                WARNING,
                ERROR,
            };

            GameLog();

            Ref<GameLogger> create_logger(const String &p_logger_id);
            /// The one getter that makes what it returns (CODE_STYLE.md, "Logging"): a logger
            /// missing yet is created, as Python's logging.getLogger() does
            Ref<GameLogger> get_logger(const String &p_logger_id);
            PackedStringArray get_loggers() const;
            /// logger_removing goes first, while the logger still has its handlers; then they are
            /// detached. Its assignments stay for a logger of the same id made later
            void remove_logger(const String &p_logger_id);

            /// Attached at once to the loggers it is assigned to
            void register_handler(const String &p_handler_name, const Ref<GameLogHandler> &p_handler);
            /// Detached from every logger; its assignments stay
            void unregister_handler(const String &p_handler_name);
            /// Attached at once when both the logger and the handler exist, else when the second
            /// of them comes
            void assign_handler(const String &p_logger_id, const String &p_handler_name);
            void unassign_handler(const String &p_logger_id, const String &p_handler_name);

        protected:
            static void _bind_methods();

        private:
            /// logger id -> GameLogger
            Dictionary loggers;
            /// handler name -> GameLogHandler
            Dictionary handlers;
            /// logger id -> PackedStringArray of handler names
            Dictionary assignments;
    };
} // namespace godot
VARIANT_ENUM_CAST(GameLog::LogLevel)
