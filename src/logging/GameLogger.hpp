#pragma once

#include "./GameLog.hpp"
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {
    /// A named logger of GameLog (GameLog::get_logger()): its lines go to the handlers registered for
    /// its id
    class GameLogger : public RefCounted {
            GDCLASS(GameLogger, RefCounted);

        public:
            /// Called once, by the GameLog that makes the logger
            void setup(GameLog *p_game_log, const String &p_logger_id);

            String get_logger_id() const;
            void log(GameLog::LogLevel p_level, const String &p_line);
            void debug(const String &p_line);
            void info(const String &p_line);
            void warning(const String &p_line);
            void error(const String &p_line);

        protected:
            static void _bind_methods();

        private:
            /// The log that owns the logger - a raw pointer, the log holds the logger's Ref
            GameLog *game_log = nullptr;
            String logger_id;
    };
} // namespace godot
