#include "maszyna/MaszynaFileIndexer.hpp"

#include "maszyna/MaszynaDataPath.hpp"
#include "maszyna/MaszynaGameDirectory.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>

namespace godot {
    MaszynaFileIndexer *MaszynaFileIndexer::singleton = nullptr;

    /* OS::get_name() of Windows */
    static constexpr const char *WINDOWS_OS_NAME = "Windows";

    MaszynaFileIndexer *MaszynaFileIndexer::get_instance() {
        return singleton;
    }

    /* Another game directory is other data: its directories are listed again */
    MaszynaFileIndexer::MaszynaFileIndexer() : case_matching(OS::get_singleton()->get_name() != WINDOWS_OS_NAME) {
        singleton = this;
        MaszynaGameDirectory *game_directory = MaszynaGameDirectory::get_instance();
        ERR_FAIL_NULL(game_directory);
        game_directory->connect(
                MaszynaGameDirectory::game_dir_changed_signal,
                callable_mp(this, &MaszynaFileIndexer::_on_game_dir_changed));
    }

    MaszynaFileIndexer::~MaszynaFileIndexer() {
        singleton = nullptr;
    }

    void MaszynaFileIndexer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("path_resolve", "base_dir", "relative_path"), &MaszynaFileIndexer::path_resolve);
        ClassDB::bind_method(D_METHOD("files_clear"), &MaszynaFileIndexer::files_clear);
        ClassDB::bind_method(D_METHOD("missing_file_add", "path", "type"), &MaszynaFileIndexer::missing_file_add);
        ClassDB::bind_method(D_METHOD("missing_files_get"), &MaszynaFileIndexer::missing_files_get);
        ClassDB::bind_method(D_METHOD("found_files_get_count"), &MaszynaFileIndexer::found_files_get_count);

        BIND_ENUM_CONSTANT(MASZYNA_FILE_INCLUDE);
        BIND_ENUM_CONSTANT(MASZYNA_FILE_FIZ);
        BIND_ENUM_CONSTANT(MASZYNA_FILE_MMD);
        BIND_ENUM_CONSTANT(MASZYNA_FILE_MODEL);
        BIND_ENUM_CONSTANT(MASZYNA_FILE_TEXTURE);
        BIND_ENUM_CONSTANT(MASZYNA_FILE_SOUND);
    }

    void MaszynaFileIndexer::_on_game_dir_changed() {
        MutexLock lock(mutex);
        directories.clear();
    }

    String MaszynaFileIndexer::path_resolve(const String &p_base_dir, const String &p_relative_path) {
        String on_disk = MaszynaDataPath::find_as_written(p_base_dir, p_relative_path);
        if (on_disk.is_empty() && case_matching) {
            // its directory found the same way, and the name looked up among the directory's names
            const String parent = p_relative_path.get_base_dir();
            const String parent_on_disk = parent.is_empty() ? String() : path_resolve(p_base_dir, parent);
            const String directory = p_base_dir.path_join(parent_on_disk);
            HashMap<String, String> names;
            bool listed = false;
            {
                MutexLock lock(mutex);
                if (const HashMap<String, String> *kept = directories.getptr(directory); kept != nullptr) {
                    names = *kept;
                    listed = true;
                }
            }
            // listed outside the lock - another thread may list it too, and keep the same names
            if (!listed && DirAccess::dir_exists_absolute(directory)) {
                PackedStringArray entries = DirAccess::get_directories_at(directory);
                entries.append_array(DirAccess::get_files_at(directory));
                for (const String &entry: entries) {
                    names.insert(entry.to_lower(), entry);
                }
            }
            if (const String *found = names.getptr(p_relative_path.get_file().to_lower()); found != nullptr) {
                on_disk = parent_on_disk.is_empty() ? *found : parent_on_disk.path_join(*found);
            }
            if (!listed) {
                MutexLock lock(mutex);
                directories.insert(directory, names);
            }
        }
        if (on_disk.is_empty()) {
            return p_relative_path;
        }
        // a name with an extension is a file: the data's directories have none
        if (!on_disk.get_extension().is_empty()) {
            MutexLock lock(mutex);
            found_files.insert(p_base_dir.path_join(on_disk));
        }
        return on_disk;
    }

    void MaszynaFileIndexer::files_clear() {
        MutexLock lock(mutex);
        found_files.clear();
        missing_files.clear();
    }

    void MaszynaFileIndexer::missing_file_add(const String &p_path, FileType p_type) {
        const String game_prefix = MaszynaGameDirectory::get_instance()->get_game_dir() + "/";
        MutexLock lock(mutex);
        missing_files[p_path.begins_with(game_prefix) ? p_path.substr(game_prefix.length()) : p_path] = p_type;
    }

    Dictionary MaszynaFileIndexer::missing_files_get() const {
        MutexLock lock(mutex);
        return missing_files.duplicate();
    }

    int64_t MaszynaFileIndexer::found_files_get_count() const {
        MutexLock lock(mutex);
        return found_files.size();
    }
} // namespace godot
