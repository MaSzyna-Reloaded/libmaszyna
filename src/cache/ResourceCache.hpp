#pragma once

#include "macros.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

#ifndef RESOURCE_CACHE_DIR_PREFIX
#define RESOURCE_CACHE_DIR_PREFIX "user://cache/"
#endif


namespace godot {

    class ResourceCache : public RefCounted {
            GDCLASS(ResourceCache, RefCounted)

        private:
            String cache_dir;
            String _get_hash_path(const String &p_path) const;

        protected:
            static void _bind_methods();
            String _get_cache_path(const String &p_path) const;
            void _clear_cache_dir(String p_path) const;

        public:
            static Ref<ResourceCache> create(const String &p_cache_dir);

            bool has(const String &p_path, const String &p_hash = "") const;
            Ref<Resource> get(const String &p_path, const String &p_hash = "") const;
            void set(const String &p_path, const Ref<Resource> &p_resource, const String &p_hash = "") const;
            void remove(const String &p_path) const;
            /* Where an entry is kept, for files a cache entry writes beside it (a scenery's chunks) */
            String get_file_path(const String &p_path) const;
            void clear();
            String get_cache_dir() const;
    };
} // namespace godot
