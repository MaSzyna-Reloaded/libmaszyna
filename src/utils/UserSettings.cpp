#include "UserSettings.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

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

    UserSettings::UserSettings() {
        config.instantiate();

        Dictionary e3d;
        e3d["auto_generate_normal"] = false;
        e3d["auto_generate_metallic"] = false;
        e3d["auto_generate_height"] = false;

        Dictionary maszyna;
        maszyna["game_dir"] = ".";

        Dictionary render;
        render["msaa_3d"] = RenderingServer::VIEWPORT_MSAA_DISABLED;
        render["anisotropic_filtering_level"] = RenderingServer::VIEWPORT_ANISOTROPY_DISABLED;
        render["use_taa"] = true;

        defaults["e3d"] = e3d;
        defaults["maszyna"] = maszyna;
        defaults["render"] = render;

        load_config();
    }

    void UserSettings::_bind_methods() {
        ClassDB::bind_method(D_METHOD("load_config"), &UserSettings::load_config);

        ClassDB::bind_method(D_METHOD("save_setting", "section", "key", "value"), &UserSettings::save_setting);
        ClassDB::bind_method(D_METHOD("set_setting", "section", "key", "value"), &UserSettings::set_setting);
        ClassDB::bind_method(D_METHOD("save_config"), &UserSettings::save_config);
        ClassDB::bind_method(D_METHOD("is_config_changed"), &UserSettings::is_config_changed);
        ClassDB::bind_method(D_METHOD("erase_setting", "section", "key"), &UserSettings::erase_setting);

        ClassDB::bind_method(
                D_METHOD("get_setting", "section", "key", "default_value"), &UserSettings::get_setting,
                DEFVAL(Variant()));

        ClassDB::bind_method(D_METHOD("get_maszyna_game_dir"), &UserSettings::get_maszyna_game_dir);

        ClassDB::bind_method(D_METHOD("save_maszyna_game_dir", "path"), &UserSettings::save_maszyna_game_dir);
        ClassDB::bind_method(D_METHOD("is_maszyna_game_dir", "path"), &UserSettings::is_maszyna_game_dir);
        ClassDB::bind_method(D_METHOD("is_maszyna_game_dir_valid"), &UserSettings::is_maszyna_game_dir_valid);
        ClassDB::bind_method(D_METHOD("find_maszyna_game_dirs"), &UserSettings::find_maszyna_game_dirs);

        ADD_SIGNAL(MethodInfo("config_changed"));
        ADD_SIGNAL(MethodInfo(
                "setting_changed", PropertyInfo(Variant::STRING, "section"), PropertyInfo(Variant::STRING, "key")));
        ADD_SIGNAL(MethodInfo("game_dir_changed"));
    }

    void UserSettings::_apply_defaults() {
        Array sections = defaults.keys();

        for (int i = 0; i < sections.size(); i++) {
            String section = sections[i];
            Dictionary section_defaults = defaults[section];
            Array keys = section_defaults.keys();

            for (int j = 0; j < keys.size(); j++) {
                String key = keys[j];

                if (!config->has_section_key(section, key)) {
                    config->set_value(section, key, section_defaults[key]);
                }
            }
        }
    }

    void UserSettings::load_config() {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");

        /* What was set and not saved is dropped: the configuration is the file again, over the
         * defaults, and every project setting the player changed is the project's own until the
         * file sets it again. */
        config->clear();
        _apply_defaults();
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        for (const Variant &name: project_values.keys()) {
            project_settings->set_setting(name, project_values[name]);
        }
        project_values.clear();

        Error err = config->load(config_file_path);

        if (err != OK) {
            Error save_err = config->save(config_file_path);
            ERR_FAIL_COND_MSG(save_err != OK, "Cannot save default user settings.");
        }
        // what was set and not saved ("Discard changes") may have been another game directory
        const String previous_game_dir = game_dir;
        _update_game_dir();

        /* The player's own values of the project's settings, keyed by their full name. The
         * constructor runs before every server's, so each server reads them at its own init. */
        if (config->has_section(PROJECT_SETTINGS_SECTION)) {
            for (const String &key: config->get_section_keys(PROJECT_SETTINGS_SECTION)) {
                _set_project_setting(key, config->get_value(PROJECT_SETTINGS_SECTION, key));
            }
        }
        saved_text = config->encode_to_text();

        emit_signal("config_changed");
        if (game_dir != previous_game_dir) {
            emit_signal("game_dir_changed");
        }
    }

    void UserSettings::set_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");

        // an empty Variant is no default to ConfigFile: a key not saved yet is looked up only if it is there
        const Variant old_value =
                config->has_section_key(p_section, p_key) ? config->get_value(p_section, p_key) : Variant();
        if (old_value == p_value) {
            return;
        }
        config->set_value(p_section, p_key, p_value);

        // what is created from now on takes it; what read it at its init keeps the old value
        if (p_section == PROJECT_SETTINGS_SECTION) {
            _set_project_setting(p_key, p_value);
        }
        const bool is_game_dir = p_section == MASZYNA_GAMEDIR_SECTION && p_key == MASZYNA_GAMEDIR_KEY;
        if (is_game_dir) {
            _update_game_dir();
        }
        emit_signal("setting_changed", p_section, p_key);
        emit_signal("config_changed");

        if (is_game_dir) {
            emit_signal("game_dir_changed");
        }
    }

    void UserSettings::erase_setting(const String &p_section, const String &p_key) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");
        if (!config->has_section_key(p_section, p_key)) {
            return;
        }
        const Dictionary section_defaults = defaults.get(p_section, Dictionary());
        if (section_defaults.has(p_key)) {
            set_setting(p_section, p_key, section_defaults[p_key]);
            return;
        }
        config->erase_section_key(p_section, p_key);
        if (p_section == PROJECT_SETTINGS_SECTION && project_values.has(p_key)) {
            ProjectSettings::get_singleton()->set_setting(p_key, project_values[p_key]);
            project_values.erase(p_key);
        }
        emit_signal("setting_changed", p_section, p_key);
        emit_signal("config_changed");
        if (p_section == MASZYNA_GAMEDIR_SECTION && p_key == MASZYNA_GAMEDIR_KEY) {
            emit_signal("game_dir_changed");
        }
    }

    void UserSettings::save_config() {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");

        Error err = config->save(config_file_path);
        ERR_FAIL_COND_MSG(err != OK, "Cannot save user settings.");
        saved_text = config->encode_to_text();
    }

    /* Told from the text of the whole configuration, so a value set and set back is no change */
    bool UserSettings::is_config_changed() const {
        ERR_FAIL_COND_V_MSG(config.is_null(), false, "UserSettings config is null.");
        return config->encode_to_text() != saved_text;
    }

    void UserSettings::save_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        set_setting(p_section, p_key, p_value);
        save_config();
    }

    /* The editor keeps the project's values: one set there would be saved into project.godot. The
     * project's own value is kept the first time, so load_config() can put it back. */
    void UserSettings::_set_project_setting(const String &p_name, const Variant &p_value) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        if (!project_values.has(p_name)) {
            project_values[p_name] = project_settings->get_setting(p_name);
        }
        project_settings->set_setting(p_name, p_value);
    }

    Variant
    UserSettings::get_setting(const String &p_section, const String &p_key, const Variant &p_default_value) const {
        ERR_FAIL_COND_V_MSG(config.is_null(), p_default_value, "UserSettings config is null.");
        return config->get_value(p_section, p_key, p_default_value);
    }

    /* The directory the player chose, when it holds the data; otherwise the game's own directory
     * when it does - the archive unpacked where it belongs - then the one installation the search
     * finds (find_maszyna_game_dirs()), taken and saved without asking when it is the only one,
     * and the chosen one again when none of these does, so the player is told which one is wrong.
     * Made absolute here, against the directory the game itself sits in, so a build dropped
     * anywhere still finds the data next to it. It must not be left relative: a path with no
     * prefix is read as res://, which in a shipped build is the packed data and holds no game
     * files, so every FileAccess check over such a path answers "missing" (see FINDINGS.md,
     * 2026-09-23). */
    void UserSettings::_update_game_dir() {
        OS *os = OS::get_singleton();
        ERR_FAIL_NULL(os);

        const String executable_dir = os->get_executable_path().get_base_dir();
        String dir = get_setting(MASZYNA_GAMEDIR_SECTION, MASZYNA_GAMEDIR_KEY, ".");
        if (dir.is_empty()) {
            dir = ".";
        }
        if (dir.is_relative_path()) {
            dir = executable_dir.path_join(dir);
        }
        dir = dir.simplify_path();
        if (is_maszyna_game_dir(dir)) {
            game_dir = dir;
            return;
        }
        if (is_maszyna_game_dir(executable_dir)) {
            game_dir = executable_dir;
            return;
        }
        const PackedStringArray found = find_maszyna_game_dirs();
        if (found.size() != 1) {
            game_dir = dir;
            return;
        }
        game_dir = found[0];
        // saved at once, and only this key - the rest of the config may hold changes not saved yet
        config->set_value(MASZYNA_GAMEDIR_SECTION, MASZYNA_GAMEDIR_KEY, game_dir);
        Ref<ConfigFile> saved;
        saved.instantiate();
        saved->load(config_file_path);
        saved->set_value(MASZYNA_GAMEDIR_SECTION, MASZYNA_GAMEDIR_KEY, game_dir);
        ERR_FAIL_COND_MSG(saved->save(config_file_path) != OK, "Cannot save the game directory.");
    }

    String UserSettings::get_maszyna_game_dir() const {
        return game_dir;
    }

    void UserSettings::save_maszyna_game_dir(const String &p_path) {
        save_setting(MASZYNA_GAMEDIR_SECTION, MASZYNA_GAMEDIR_KEY, p_path);
    }

    bool UserSettings::is_maszyna_game_dir(const String &p_path) const {
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

    bool UserSettings::is_maszyna_game_dir_valid() const {
        return is_maszyna_game_dir(game_dir);
    }

    PackedStringArray UserSettings::find_maszyna_game_dirs() const {
        OS *os = OS::get_singleton();
        ERR_FAIL_NULL_V(os, PackedStringArray());

        PackedStringArray found;
        const auto add = [this, &found](const String &p_path) {
            const String path = p_path.simplify_path();
            if (!found.has(path) && is_maszyna_game_dir(path)) {
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
