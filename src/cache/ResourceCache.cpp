#include "ResourceCache.hpp"

#include "game_data/GameDataServer.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

    void ResourceCache::_bind_methods() {
        ClassDB::bind_static_method("ResourceCache", D_METHOD("create", "cache_dir"), &ResourceCache::create);
        ClassDB::bind_method(D_METHOD("has", "path", "hash"), &ResourceCache::has, DEFVAL(""));
        ClassDB::bind_method(D_METHOD("get", "path", "hash"), &ResourceCache::get, DEFVAL(""));
        ClassDB::bind_method(D_METHOD("set", "path", "resource", "hash"), &ResourceCache::set, DEFVAL(""));
        ClassDB::bind_method(D_METHOD("remove", "path"), &ResourceCache::remove);
        ClassDB::bind_method(D_METHOD("get_file_path", "path"), &ResourceCache::get_file_path);
        ClassDB::bind_method(D_METHOD("clear"), &ResourceCache::clear);
        ClassDB::bind_method(D_METHOD("get_cache_dir"), &ResourceCache::get_cache_dir);

        ADD_PROPERTY(
                PropertyInfo(
                        Variant::STRING, "cache_dir", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_DEFAULT),
                "", "get_cache_dir");
    }

    Ref<ResourceCache> ResourceCache::create(const String &p_cache_dir) {
        Ref<ResourceCache> cache;
        cache.instantiate();
        cache->cache_dir = p_cache_dir;

        // Every cache follows the one "throw the caches away" request, so a new cache is covered
        // by "Clear cache" and by a build change without its owner having to remember it.
        GameDataServer *game_data = GameDataServer::get_instance();
        if (game_data != nullptr) {
            // No explicit disconnect: callable_mp reports this instance as the callable's object,
            // so the engine drops the connection when the instance dies.
            game_data->connect(
                    GameDataServer::cache_clear_requested_signal, callable_mp(cache.ptr(), &ResourceCache::clear));
        }
        return cache;
    }

    String ResourceCache::_get_cache_path(const String &p_path) const {
        return String(RESOURCE_CACHE_DIR_PREFIX).path_join(cache_dir).path_join(p_path);
    }

    String ResourceCache::_get_hash_path(const String &p_path) const {
        return _get_cache_path(p_path) + ".hash";
    }

    bool ResourceCache::has(const String &p_path, const String &p_hash) const {
        const String cache_path = _get_cache_path(p_path);
        if (!FileAccess::file_exists(cache_path)) {
            return false;
        }

        if (p_hash == "") {
            return true;
        }

        const String hash_path = _get_hash_path(p_path);
        const Ref<FileAccess> hash_file = FileAccess::open(hash_path, FileAccess::READ);
        if (hash_file.is_null()) {
            return false;
        }

        return hash_file->get_as_text() == p_hash;
    }

    Ref<Resource> ResourceCache::get(const String &p_path, const String &p_hash) const {
        const String cache_path = _get_cache_path(p_path);
        if (!FileAccess::file_exists(cache_path)) {
            return nullptr;
        }

        if (p_hash != "") {
            const Ref<FileAccess> hash_file = FileAccess::open(_get_hash_path(p_path), FileAccess::READ);
            if (hash_file.is_null() || hash_file->get_as_text() != p_hash) {
                return nullptr;
            }
        }

        return ResourceLoader::get_singleton()->load(cache_path);
    }

    void ResourceCache::set(const String &p_path, const Ref<Resource> &p_resource, const String &p_hash) const {
        if (p_resource.is_null()) {
            return;
        }

        const String cache_path = _get_cache_path(p_path);
        const String hash_path = _get_hash_path(p_path);
        const String cache_dir = cache_path.get_base_dir();

        if (!DirAccess::dir_exists_absolute(cache_dir)) {
            DirAccess::make_dir_recursive_absolute(cache_dir);
        }

        const Error err = ResourceSaver::get_singleton()->save(p_resource, cache_path);
        if (err != OK) {
            UtilityFunctions::push_error("[ResourceCache] Failed to save cache: " + cache_path);
        } else {
            // the resource loaded from this path before stays in Godot's cache while it lives, and
            // get() would hand it out again in place of what was just saved
            p_resource->take_over_path(cache_path);
            UtilityFunctions::print_verbose("[ResourceCache] Saved to disk: " + cache_path);
        }

        if (p_hash != "") {
            const Ref<FileAccess> hash_file = FileAccess::open(hash_path, FileAccess::WRITE);
            if (hash_file.is_null()) {
                UtilityFunctions::push_error("[ResourceCache] Failed to save cache hash: " + hash_path);
            } else {
                hash_file->store_string(p_hash);
            }
        } else if (FileAccess::file_exists(hash_path)) {
            DirAccess::remove_absolute(hash_path);
        }
    }

    void ResourceCache::remove(const String &p_path) const {
        const String cache_path = _get_cache_path(p_path);
        const String hash_path = _get_hash_path(p_path);
        if (FileAccess::file_exists(cache_path)) {
            DirAccess::remove_absolute(cache_path);
        }
        if (FileAccess::file_exists(hash_path)) {
            DirAccess::remove_absolute(hash_path);
        }
    }

    String ResourceCache::get_file_path(const String &p_path) const {
        return _get_cache_path(p_path);
    }

    void ResourceCache::_clear_cache_dir(String p_path) const {
        if (DirAccess::dir_exists_absolute(p_path)) {
            const Ref<DirAccess> dir = DirAccess::open(p_path);
            if (dir.is_valid()) {
                UtilityFunctions::print_verbose("[ResourceCache] Clearing cache for " + p_path + "...");
                dir->list_dir_begin();
                String file_name = dir->get_next();
                while (file_name != "") {
                    if (!dir->current_is_dir()) {
                        dir->remove(file_name);
                    } else {
                        _clear_cache_dir(p_path.path_join(file_name));
                    }
                    file_name = dir->get_next();
                }
                DirAccess::remove_absolute(p_path);
            }
        }
    }

    void ResourceCache::clear() {
        _clear_cache_dir(String(RESOURCE_CACHE_DIR_PREFIX).path_join(cache_dir));
    }

    String ResourceCache::get_cache_dir() const {
        return cache_dir;
    }

} // namespace godot
