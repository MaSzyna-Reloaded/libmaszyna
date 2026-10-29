#include "maszyna/MaszynaDataPath.hpp"

#include "maszyna/MaszynaFileIndexer.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot {
    void MaszynaDataPath::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaDataPath", D_METHOD("resolve", "base_dir", "relative_path"), &MaszynaDataPath::resolve);
    }

    String MaszynaDataPath::resolve(const String &p_base_dir, const String &p_relative_path) {
        String relative_path = p_relative_path.replace("\\", "/");
        if (relative_path.is_empty()) {
            return relative_path;
        }
        // any other difference in case, on a filesystem that keeps it
        if (MaszynaFileIndexer *file_indexer = MaszynaFileIndexer::get_instance(); file_indexer != nullptr) {
            return file_indexer->path_resolve(p_base_dir, relative_path);
        }
        String as_written = find_as_written(p_base_dir, relative_path);
        return as_written.is_empty() ? relative_path : as_written;
    }

    String MaszynaDataPath::find_as_written(const String &p_base_dir, const String &p_relative_path) {
        const String original_path = p_base_dir.path_join(p_relative_path);
        if (FileAccess::file_exists(original_path) || DirAccess::dir_exists_absolute(original_path)) {
            return p_relative_path;
        }
        String lowercase_path = p_relative_path.to_lower();
        const String lowercase_absolute_path = p_base_dir.path_join(lowercase_path);
        if (FileAccess::file_exists(lowercase_absolute_path) ||
            DirAccess::dir_exists_absolute(lowercase_absolute_path)) {
            return lowercase_path;
        }
        return String();
    }
} // namespace godot
