#pragma once

#include "./GameLogHandler.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot {
    /// Writes a logger's lines to a file, started afresh by open(). Every line is on the disk at
    /// once: a log that explains a crash must not lose its end in the file's buffer.
    class GameLogFileHandler : public GameLogHandler {
            GDCLASS(GameLogFileHandler, GameLogHandler);

        public:
            /// The handler of a new file at p_path, its directory made; null when it cannot be opened
            static Ref<GameLogFileHandler> open(const String &p_path);

            String get_path() const;

        protected:
            static void _bind_methods();
            void _handle_line(const String &p_logger_id, GameLog::LogLevel p_level, const String &p_line) override;

        private:
            Ref<FileAccess> file;
            String path;
    };
} // namespace godot
