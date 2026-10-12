#pragma once

#include "macros.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/string.hpp>

#ifndef RESOURCE_CACHE_DIR_PREFIX
#define RESOURCE_CACHE_DIR_PREFIX "user://cache/"
#endif


namespace godot {

    class ResourceCache : public RefCounted {
            GDCLASS(ResourceCache, RefCounted)

        private:
            String cache_dir;
            /// resource_save()'s and scene_pack()'s; one for the whole extension
            static Mutex defaults_mutex;
            String _get_hash_path(const String &p_path) const;
            /* The project setting that switches every cache off (the tests: nothing read from a
             * previous run, nothing written) */
            static bool _enabled();

        protected:
            static void _bind_methods();
            String _get_cache_path(const String &p_path) const;
            static void _clear_cache_dir(const String &p_path);

        public:
            static Ref<ResourceCache> create(const String &p_cache_dir);
            /* The one way the extension saves a resource, from any thread. Godot fills its cache of
             * property defaults (ClassDB::class_get_default_property_value(), class_db.cpp:2172)
             * on a class's first lookup without a lock, and saving looks up every property, so two
             * at once corrupt it and Godot dies (docs/findings-archive.md, 2026-10-12): they are
             * made one at a time. */
            static Error resource_save(const Ref<Resource> &p_resource, const String &p_path);
            /* The one way the extension packs a scene, from any thread - packing looks up the
             * defaults of every node's properties (packed_scene.cpp:982), under resource_save()'s
             * lock. Null when the scene cannot be packed. */
            static Ref<PackedScene> scene_pack(Node *p_root);

            bool has(const String &p_path, const String &p_hash = "") const;
            Ref<Resource> get(const String &p_path, const String &p_hash = "") const;
            void set(const String &p_path, const Ref<Resource> &p_resource, const String &p_hash = "") const;
            void remove(const String &p_path) const;
            /* Where an entry is kept, for files a cache entry writes beside it (a scenery's chunks) */
            String get_file_path(const String &p_path) const;
            void clear();
            /* Every cache's files, whether or not a cache for them was made yet in this session */
            static void clear_all();
            String get_cache_dir() const;
    };
} // namespace godot
