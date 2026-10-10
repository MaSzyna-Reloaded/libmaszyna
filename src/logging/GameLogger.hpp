#pragma once

#include "./GameLog.hpp"
#include "./GameLogHandler.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {
    /// A named logger of GameLog (GameLog::get_logger()): its lines go to its handlers, which GameLog
    /// attaches to it (GameLog::assign_handler()). It knows nothing of GameLog.
    class GameLogger : public RefCounted {
            GDCLASS(GameLogger, RefCounted);

        public:
            /// Called once, by GameLog when it makes the logger
            void setup(const String &p_logger_id);
            /// By GameLog only - not bound, a script assigns a handler through GameLog
            void add_handler(const Ref<GameLogHandler> &p_handler);
            void remove_handler(const Ref<GameLogHandler> &p_handler);

            String get_logger_id() const;
            void log(GameLog::LogLevel p_level, const String &p_line);
            void debug(const String &p_line);
            void info(const String &p_line);
            void warning(const String &p_line);
            void error(const String &p_line);

        protected:
            static void _bind_methods();

        private:
            String logger_id;
            Vector<Ref<GameLogHandler>> handlers;
    };
} // namespace godot
