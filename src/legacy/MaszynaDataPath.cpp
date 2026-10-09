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

        String lowercase_path = relative_path.to_lower();
        const String lowercase_absolute_path = p_base_dir.path_join(lowercase_path);
        if (FileAccess::file_exists(lowercase_absolute_path) ||
            DirAccess::dir_exists_absolute(lowercase_absolute_path)) {
            return lowercase_path;
        }

        // any other difference in case ("ep07p-2003_przebieg.txt" for EP07P-2003_przebieg.txt): each
        // part of the path is the entry of its directory that matches it letter case aside
        String resolved_path;
        for (const String &part: relative_path.split("/", false)) {
            const String directory = p_base_dir.path_join(resolved_path);
            if (FileAccess::file_exists(directory.path_join(part)) ||
                DirAccess::dir_exists_absolute(directory.path_join(part))) {
                resolved_path = resolved_path.path_join(part);
                continue;
            }
            if (!DirAccess::dir_exists_absolute(directory)) {
                return relative_path;
            }
            PackedStringArray entries = DirAccess::get_files_at(directory);
            entries.append_array(DirAccess::get_directories_at(directory));
            String entry;
            for (int64_t index = 0; entry.is_empty() && index < entries.size(); index++) {
                if (entries[index].nocasecmp_to(part) == 0) {
                    entry = entries[index];
                }
            }
            if (entry.is_empty()) {
                return relative_path;
            }
            resolved_path = resolved_path.path_join(entry);
        }
        return resolved_path;
    }
} // namespace godot
