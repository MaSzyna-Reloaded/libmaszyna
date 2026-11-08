#include "MaszynaFizManager.hpp"
#include "maszyna/MaszynaDataPath.hpp"
#include "maszyna/MaszynaFileIndexer.hpp"
#include "maszyna/MaszynaGameDirectory.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    MaszynaFizManager *MaszynaFizManager::singleton = nullptr;

    void MaszynaFizManager::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_description_reader", "reader", "format_version"),
                &MaszynaFizManager::set_description_reader);
        ClassDB::bind_method(
                D_METHOD("load_description", "data_path", "fiz_filename", "wipers"),
                &MaszynaFizManager::load_description, DEFVAL(Variant()));
        ClassDB::bind_method(
                D_METHOD("load_description_at", "fiz_path", "wipers"), &MaszynaFizManager::load_description_at,
                DEFVAL(Variant()));
    }

    MaszynaFizManager *MaszynaFizManager::get_instance() {
        return singleton;
    }

    MaszynaFizManager::MaszynaFizManager() {
        singleton = this;
        cache = ResourceCache::create(CACHE_DIRECTORY);
    }

    MaszynaFizManager::~MaszynaFizManager() {
        singleton = nullptr;
    }

    void MaszynaFizManager::set_description_reader(const Callable &p_reader, const int p_format_version) {
        MutexLock lock(mutex);
        reader = p_reader;
        format_version = p_format_version;
    }

    Ref<VehicleController>
    MaszynaFizManager::load_description(
            const String &p_data_path, const String &p_fiz_filename, const Ref<RailVehicleWipers> &p_wipers) {
        const MaszynaGameDirectory *game_directory = MaszynaGameDirectory::get_instance();
        ERR_FAIL_NULL_V(game_directory, Ref<VehicleController>());
        const String game_dir = game_directory->get_game_dir();
        return load_description_at(game_dir.path_join(
                MaszynaDataPath::resolve(game_dir, p_data_path.path_join(p_fiz_filename + String(".fiz")))),
                p_wipers);
    }

    Ref<VehicleController>
    MaszynaFizManager::load_description_at(const String &p_fiz_path, const Ref<RailVehicleWipers> &p_wipers) {
        // a vehicle whose file is not there is told on every load, its description cached or not
        if (!FileAccess::file_exists(p_fiz_path)) {
            if (MaszynaFileIndexer *file_indexer = MaszynaFileIndexer::get_instance(); file_indexer != nullptr) {
                file_indexer->missing_file_add(p_fiz_path, MaszynaFileIndexer::MASZYNA_FILE_FIZ);
            }
        }
        Callable fiz_reader;
        int version = 0;
        {
            MutexLock lock(mutex);
            fiz_reader = reader;
            version = format_version;
        }
        ERR_FAIL_COND_V_MSG(!fiz_reader.is_valid(), Ref<VehicleController>(), "No reader of the FIZ files");
        const MaszynaGameDirectory *game_directory = MaszynaGameDirectory::get_instance();
        const String game_dir = game_directory != nullptr ? game_directory->get_game_dir() : String();
        const String cache_path = p_fiz_path.trim_prefix(game_dir.path_join("")) + String(".res");
        // a change of the wipers the game's patch gives the file is a change of what it describes
        const String wipers = p_wipers.is_valid() ? UtilityFunctions::var_to_str(p_wipers) : String();
        const String cache_hash =
                vformat("%d:%d:%s:%s", FileAccess::get_modified_time(p_fiz_path), version, p_fiz_path, wipers)
                        .md5_text();
        Ref<VehicleController> description = cache->get(cache_path, cache_hash);
        if (description.is_valid()) {
            return description;
        }
        description = fiz_reader.call(p_fiz_path, p_wipers);
        if (description.is_valid()) {
            cache->set(cache_path, description, cache_hash);
        }
        return description;
    }
} // namespace godot
