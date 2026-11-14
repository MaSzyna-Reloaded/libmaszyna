#include "MaszynaVehicleMmdManager.hpp"
#include "maszyna/MaszynaDataPath.hpp"
#include "maszyna/MaszynaGameDirectory.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    MaszynaVehicleMmdManager *MaszynaVehicleMmdManager::singleton = nullptr;

    void MaszynaVehicleMmdManager::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaVehicleMmdManager", D_METHOD("names_vehicle", "abs_mmd_path"),
                &MaszynaVehicleMmdManager::names_vehicle);
        ClassDB::bind_method(
                D_METHOD("set_mmd_data_reader", "reader", "format_version"),
                &MaszynaVehicleMmdManager::set_mmd_data_reader);
        ClassDB::bind_method(
                D_METHOD("set_patch_digest", "patch_digest"), &MaszynaVehicleMmdManager::set_patch_digest);
        ClassDB::bind_method(
                D_METHOD("load_mmd_data", "data_path", "file_name", "livery", "vehicle_id"),
                &MaszynaVehicleMmdManager::load_mmd_data);
    }

    MaszynaVehicleMmdManager *MaszynaVehicleMmdManager::get_instance() {
        return singleton;
    }

    MaszynaVehicleMmdManager::MaszynaVehicleMmdManager() {
        singleton = this;
        cache = ResourceCache::create(CACHE_DIRECTORY);
    }

    MaszynaVehicleMmdManager::~MaszynaVehicleMmdManager() {
        singleton = nullptr;
    }

    bool MaszynaVehicleMmdManager::names_vehicle(const String &p_abs_mmd_path) {
        return FileAccess::get_file_as_bytes(p_abs_mmd_path).get_string_from_ascii().contains(VEHICLE_NAME_PARAMETER);
    }

    void MaszynaVehicleMmdManager::set_mmd_data_reader(const Callable &p_reader, const int p_format_version) {
        MutexLock lock(mutex);
        reader = p_reader;
        format_version = p_format_version;
    }

    void MaszynaVehicleMmdManager::set_patch_digest(const String &p_patch_digest) {
        MutexLock lock(mutex);
        patch_digest = p_patch_digest;
    }

    Ref<Resource> MaszynaVehicleMmdManager::load_mmd_data(
            const String &p_data_path, const String &p_file_name, const String &p_livery, const String &p_vehicle_id) {
        if (p_data_path.is_empty() || p_file_name.is_empty()) {
            return {};
        }
        Callable mmd_reader;
        int version = 0;
        String patch;
        {
            MutexLock lock(mutex);
            mmd_reader = reader;
            version = format_version;
            patch = patch_digest;
        }
        ERR_FAIL_COND_V_MSG(!mmd_reader.is_valid(), Ref<Resource>(), "No reader of the vehicles' MMD data");
        const MaszynaGameDirectory *game_directory = MaszynaGameDirectory::get_instance();
        ERR_FAIL_NULL_V(game_directory, Ref<Resource>());
        const String game_dir = game_directory->get_game_dir();
        const String relative_base_path = p_data_path.trim_prefix("/").path_join(p_file_name);
        const String abs_mmd_path =
                game_dir.path_join(MaszynaDataPath::resolve(game_dir, relative_base_path + String(".mmd")));
        const String abs_fiz_path =
                game_dir.path_join(MaszynaDataPath::resolve(game_dir, relative_base_path + String(".fiz")));
        const String variant = names_vehicle(abs_mmd_path) ? p_livery + String("|") + p_vehicle_id : p_livery;
        const String cache_path = String("/")
                                          .path_join(p_data_path.trim_prefix("/"))
                                          .path_join(vformat("%s_%s.res", p_file_name, variant.md5_text()));
        const String cache_hash = vformat("%d:%d:%d:%s:%s", version, FileAccess::get_modified_time(abs_mmd_path),
                                          FileAccess::get_modified_time(abs_fiz_path), abs_mmd_path, patch)
                                          .md5_text();
        Ref<Resource> mmd_data = cache->get(cache_path, cache_hash);
        if (mmd_data.is_valid()) {
            return mmd_data;
        }
        mmd_data = mmd_reader.call(p_data_path, p_file_name, p_livery, p_vehicle_id);
        if (mmd_data.is_valid()) {
            cache->set(cache_path, mmd_data, cache_hash);
        }
        return mmd_data;
    }
} // namespace godot
