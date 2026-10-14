#include "./GameLogFileHandler.hpp"
#include <godot_cpp/classes/dir_access.hpp>

namespace godot {
    void GameLogFileHandler::_bind_methods() {
        ClassDB::bind_static_method("GameLogFileHandler", D_METHOD("open", "path"), &GameLogFileHandler::open);
        ClassDB::bind_method(D_METHOD("get_path"), &GameLogFileHandler::get_path);
    }

    Ref<GameLogFileHandler> GameLogFileHandler::open(const String &p_path) {
        DirAccess::make_dir_recursive_absolute(p_path.get_base_dir());
        const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
        ERR_FAIL_COND_V_MSG(file.is_null(), Ref<GameLogFileHandler>(), "Cannot open log file: " + p_path);
        Ref<GameLogFileHandler> handler;
        handler.instantiate();
        handler->file = file;
        handler->path = p_path;
        return handler;
    }

    String GameLogFileHandler::get_path() const {
        return path;
    }

    void GameLogFileHandler::_handle_line(
            const String & /*p_logger_id*/, GameLog::LogLevel /*p_level*/, const String &p_line) {
        ERR_FAIL_COND_MSG(file.is_null(), "A file handler is made by open()");
        file->store_line(p_line);
        file->flush();
    }
} // namespace godot
