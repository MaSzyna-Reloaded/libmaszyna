#include "MaszynaGameDirectory.hpp"
#include "game_data/GameDataServer.hpp"
#include "utils/ReloadedTranslationServer.hpp"
#include "utils/UserSettings.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    MaszynaGameDirectory *MaszynaGameDirectory::singleton = nullptr;
    const char *MaszynaGameDirectory::game_dir_changed_signal = "game_dir_changed";

    /// Directories the original's data always has - eu07.exe is not one of them, a copy may come
    /// without it
    static constexpr const char *GAME_DIR_MARKERS[] = {"scenery", "dynamic", "textures"};
    /// Where Steam keeps itself, below the home directory (Linux: native, its symlink, Flatpak)
    static constexpr const char *STEAM_HOME_ROOTS[] = {
            ".steam/steam", ".local/share/Steam", ".var/app/com.valvesoftware.Steam/.local/share/Steam"};
    /// Environment variables naming where Steam installs itself on Windows, in its own folder
    static constexpr const char *STEAM_PROGRAM_FILES_VARIABLES[] = {"ProgramFiles(x86)", "ProgramFiles"};
    static constexpr const char *STEAM_PROGRAM_FILES_DIR = "Steam";
    /// Steam's list of its libraries, below its own directory; a line `"path"  "<library>"` each
    static constexpr const char *STEAM_LIBRARY_FOLDERS_FILE = "steamapps/libraryfolders.vdf";
    static constexpr const char *STEAM_LIBRARY_PATH_KEY = "\"path\"";
    /// Index of the value in a `"path"  "<library>"` line split by quotes
    static constexpr int STEAM_LIBRARY_PATH_SLICE = 3;
    static constexpr const char *STEAM_GAMES_DIR = "steamapps/common";
    /// The usual folder for games on a drive and in the home directory
    static constexpr const char *GAMES_DIR = "Games";

    MaszynaGameDirectory *MaszynaGameDirectory::get_instance() {
        return singleton;
    }

    /* Taken from the configuration as it is now, and again whenever it or the game_dir setting
     * changes */
    MaszynaGameDirectory::MaszynaGameDirectory() {
        singleton = this;
        UserSettings *settings = UserSettings::get_instance();
        ERR_FAIL_NULL(settings);
        settings->connect(UserSettings::config_changed_signal, callable_mp(this, &MaszynaGameDirectory::_update));
        settings->connect(
                UserSettings::setting_changed_signal, callable_mp(this, &MaszynaGameDirectory::_on_setting_changed));
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            connect(game_dir_changed_signal, callable_mp(game_data, &GameDataServer::data_reload));
        }
        _update();
    }

    MaszynaGameDirectory::~MaszynaGameDirectory() {
        singleton = nullptr;
    }

    void MaszynaGameDirectory::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_game_dir"), &MaszynaGameDirectory::get_game_dir);
        ClassDB::bind_method(D_METHOD("save_game_dir", "path"), &MaszynaGameDirectory::save_game_dir);
        ClassDB::bind_method(D_METHOD("is_game_dir", "path"), &MaszynaGameDirectory::is_game_dir);
        ClassDB::bind_method(D_METHOD("is_game_dir_valid"), &MaszynaGameDirectory::is_game_dir_valid);
        ClassDB::bind_method(D_METHOD("find_game_dirs"), &MaszynaGameDirectory::find_game_dirs);
        ADD_SIGNAL(MethodInfo(game_dir_changed_signal));
    }

    void MaszynaGameDirectory::_on_setting_changed(const String &p_section, const String &p_key) {
        if (p_section == GAME_DIR_SECTION && p_key == GAME_DIR_KEY) {
            _update();
        }
    }

    /* The directory the player chose, when it holds the data; otherwise the game's own directory
     * when it does - the archive unpacked where it belongs - then the one installation the search
     * finds (find_game_dirs()), taken and saved without asking when it is the only one, and the
     * chosen one again when none of these does, so the player is told which one is wrong. Made
     * absolute, against the directory the game itself sits in. */
    void MaszynaGameDirectory::_update() {
        OS *os = OS::get_singleton();
        UserSettings *settings = UserSettings::get_instance();
        ERR_FAIL_NULL(os);
        ERR_FAIL_NULL(settings);
        const String previous = game_dir;
        const String executable_dir = os->get_executable_path().get_base_dir();
        String dir = settings->get_setting(GAME_DIR_SECTION, GAME_DIR_KEY, ".");
        if (dir.is_empty()) {
            dir = ".";
        }
        if (dir.is_relative_path()) {
            dir = executable_dir.path_join(dir);
        }
        dir = dir.simplify_path();
        if (is_game_dir(dir)) {
            game_dir = dir;
        } else if (is_game_dir(executable_dir)) {
            game_dir = executable_dir;
        } else if (const PackedStringArray found = find_game_dirs(); found.size() == 1) {
            game_dir = found[0];
            settings->persist_setting(GAME_DIR_SECTION, GAME_DIR_KEY, game_dir);
        } else {
            game_dir = dir;
        }
        if (ReloadedTranslationServer *translations = ReloadedTranslationServer::get_instance();
            translations != nullptr) {
            translations->translation_set_catalogue_dir(game_dir.path_join(LANG_DIR));
        }
        if (game_dir != previous && !previous.is_empty()) {
            emit_signal(game_dir_changed_signal);
        }
    }

    String MaszynaGameDirectory::get_game_dir() const {
        return game_dir;
    }

    void MaszynaGameDirectory::save_game_dir(const String &p_path) {
        UserSettings *settings = UserSettings::get_instance();
        ERR_FAIL_NULL(settings);
        settings->save_setting(GAME_DIR_SECTION, GAME_DIR_KEY, p_path);
    }

    bool MaszynaGameDirectory::is_game_dir(const String &p_path) const {
        if (p_path.is_empty()) {
            return false;
        }
        for (const char *marker: GAME_DIR_MARKERS) {
            if (!DirAccess::dir_exists_absolute(p_path.path_join(marker))) {
                return false;
            }
        }
        return true;
    }

    bool MaszynaGameDirectory::is_game_dir_valid() const {
        return is_game_dir(game_dir);
    }

    PackedStringArray MaszynaGameDirectory::find_game_dirs() const {
        OS *os = OS::get_singleton();
        ERR_FAIL_NULL_V(os, PackedStringArray());

        PackedStringArray found;
        const auto add = [this, &found](const String &p_path) {
            const String path = p_path.simplify_path();
            if (!found.has(path) && is_game_dir(path)) {
                found.push_back(path);
            }
        };
        const auto add_children = [&add](const String &p_parent) {
            if (!DirAccess::dir_exists_absolute(p_parent)) {
                return;
            }
            for (const String &child: DirAccess::get_directories_at(p_parent)) {
                add(p_parent.path_join(child));
            }
        };

        // the game's own directory, and the one above it - the archive unpacked into a subfolder
        const String executable_dir = os->get_executable_path().get_base_dir();
        add(executable_dir);
        add(executable_dir.get_base_dir());

        const String home =
                os->has_environment("USERPROFILE") ? os->get_environment("USERPROFILE") : os->get_environment("HOME");
        PackedStringArray steam_roots;
        for (const char *variable: STEAM_PROGRAM_FILES_VARIABLES) {
            if (os->has_environment(variable)) {
                steam_roots.push_back(
                        os->get_environment(variable).replace("\\", "/").path_join(STEAM_PROGRAM_FILES_DIR));
            }
        }
        if (!home.is_empty()) {
            for (const char *root: STEAM_HOME_ROOTS) {
                steam_roots.push_back(home.path_join(root));
            }
        }
        // every library Steam lists, and every game in it - the folder's name is not relied on
        for (const String &steam_root: steam_roots) {
            PackedStringArray libraries;
            libraries.push_back(steam_root);
            const String library_folders = steam_root.path_join(STEAM_LIBRARY_FOLDERS_FILE);
            if (FileAccess::file_exists(library_folders)) {
                for (const String &line: FileAccess::get_file_as_string(library_folders).split("\n")) {
                    if (line.strip_edges().begins_with(STEAM_LIBRARY_PATH_KEY)) {
                        libraries.push_back(line.get_slice("\"", STEAM_LIBRARY_PATH_SLICE).replace("\\\\", "/"));
                    }
                }
            }
            for (const String &library: libraries) {
                add_children(library.path_join(STEAM_GAMES_DIR));
            }
        }

        for (int i = 0; i < DirAccess::get_drive_count(); i++) {
            const String drive = DirAccess::get_drive_name(i);
            add_children(drive);
            add_children(drive.path_join(GAMES_DIR));
        }

        if (!home.is_empty()) {
            add_children(home);
            add_children(home.path_join(GAMES_DIR));
        }
        return found;
    }

} // namespace godot
