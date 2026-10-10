#pragma once

#include "./GameLog.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {
    /// Where the lines of loggers go (GameLog::register_handler(), GameLog::assign_handler()): the
    /// lines below min_level are left out. Implement it in C++ by overriding _handle_line(), or in GDScript by
    /// overriding _handle().
    class GameLogHandler : public RefCounted {
            GDCLASS(GameLogHandler, RefCounted);

        public:
            void set_min_level(GameLog::LogLevel p_min_level);
            GameLog::LogLevel get_min_level() const;

            /// A line of a logger it is attached to, by GameLogger::log()
            void handle(const String &p_logger_id, GameLog::LogLevel p_level, const String &p_line);

        protected:
            static void _bind_methods();
            virtual void _handle_line(const String &p_logger_id, GameLog::LogLevel p_level, const String &p_line);

            GDVIRTUAL3(_handle, String, GameLog::LogLevel, String)

        private:
            GameLog::LogLevel min_level = GameLog::LogLevel::DEBUG;
    };
} // namespace godot
