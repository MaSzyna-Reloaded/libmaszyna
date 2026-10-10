#include "GameDataServer.hpp"
#include "utils/UserSettings.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    GameDataServer *GameDataServer::singleton = nullptr;
    const char *GameDataServer::cache_clear_requested_signal = "cache_clear_requested";
    const char *GameDataServer::data_unload_requested_signal = "data_unload_requested";
    const char *GameDataServer::data_reload_requested_signal = "data_reload_requested";

    GameDataServer *GameDataServer::get_instance() {
        return singleton;
    }

    /* A different game directory is different data: everything read from it is read again */
    GameDataServer::GameDataServer() {
        singleton = this;
        build_number = FileAccess::get_file_as_string(BUILD_NUMBER_PATH).strip_edges();
        // the first thing the session's log says: which build wrote it
        UtilityFunctions::print(
                "[GameDataServer] Build ", build_number.is_empty() ? String("none (unbuilt checkout)") : build_number);
        if (UserSettings *settings = UserSettings::get_instance(); settings != nullptr) {
            settings->connect("game_dir_changed", callable_mp(this, &GameDataServer::data_reload));
        }
    }

    GameDataServer::~GameDataServer() {
        singleton = nullptr;
    }

    void GameDataServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("cache_clear"), &GameDataServer::cache_clear);
        ClassDB::bind_method(D_METHOD("data_reload"), &GameDataServer::data_reload);
        ClassDB::bind_method(D_METHOD("build_get_number"), &GameDataServer::build_get_number);
        ClassDB::bind_method(D_METHOD("build_check_version"), &GameDataServer::build_check_version);
        ADD_SIGNAL(MethodInfo(cache_clear_requested_signal));
        ADD_SIGNAL(MethodInfo(data_unload_requested_signal));
        ADD_SIGNAL(MethodInfo(data_reload_requested_signal));
    }

    void GameDataServer::cache_clear() {
        emit_signal(cache_clear_requested_signal);
    }

    void GameDataServer::data_reload() {
        emit_signal(data_unload_requested_signal);
        emit_signal(data_reload_requested_signal);
    }

    String GameDataServer::build_get_number() const {
        return build_number;
    }

    /// A cache on disk outlives the code that produced it, so a new build starts from clean data.
    bool GameDataServer::build_check_version() {
        if (build_version_checked) {
            return false;
        }
        build_version_checked = true;

        const String current = build_number;
        if (current.is_empty()) {
            return false;
        }

        UserSettings *settings = UserSettings::get_instance();
        ERR_FAIL_NULL_V(settings, false);

        const String stored = settings->get_setting(BUILD_SECTION, BUILD_NUMBER_KEY, "");
        if (stored == current) {
            return false;
        }

        UtilityFunctions::print(
                "[GameDataServer] Build changed (" + (stored.is_empty() ? String("none") : stored) + " -> " + current +
                "), clearing cache...");
        cache_clear();
        settings->save_setting(BUILD_SECTION, BUILD_NUMBER_KEY, current);
        return true;
    }
} // namespace godot
