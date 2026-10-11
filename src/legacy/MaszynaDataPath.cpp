#include "legacy/MaszynaDataPath.hpp"

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

        const String original_path = p_base_dir.path_join(relative_path);
        if (FileAccess::file_exists(original_path) || DirAccess::dir_exists_absolute(original_path)) {
            return relative_path;
        }

        // no directory is listed for any other difference in case: a lookup of a file that is not
        // there - most of the candidates a caller tries - would list its directory every time
        String lowercase_path = relative_path.to_lower();
        const String lowercase_absolute_path = p_base_dir.path_join(lowercase_path);
        if (FileAccess::file_exists(lowercase_absolute_path) ||
            DirAccess::dir_exists_absolute(lowercase_absolute_path)) {
            return lowercase_path;
        }
        return relative_path;
    }
} // namespace godot
