#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// Resources needed only while something near the camera uses them - a scenery's models, its
    /// terrain, a vehicle's models.
    ///
    /// A resource is registered under a key with the Callable that loads it; one key is one
    /// resource, shared by everybody who registers it. resource_hold() holds a loaded copy and
    /// resource_release() lets it go.
    ///
    /// With lazy loading (maszyna/resources/lazy_loading, or --enable-lazy-loading on the command
    /// line) a resource is loaded when it is wanted and let go when nobody holds it any more, so its
    /// memory, RAM and the VRAM of a mesh or a texture, goes back. Without it - the default - a
    /// resource is loaded when it is registered, while a scenery loads, and kept until its last
    /// registration is freed; holding and releasing it only says who builds from it.
    ///
    /// Thread-safe: the streaming loads on its worker threads (resource_load()) and holds what it
    /// builds on the main thread (resource_hold()).
    class ResourceLazyLoader : public Object {
            GDCLASS(ResourceLazyLoader, Object)

        private:
            struct Entry {
                    String key;
                    Callable loader; // () -> Resource
                    int registrations = 0;
                    int holders = 0;
                    /// Loaded at its registration and kept until the last one is freed - the mode
                    /// lazy_loading had when the key was registered first
                    bool resident_by_registration = false;
                    Ref<Resource> resource; // while held, or while registered when resident by it
            };

            static ResourceLazyLoader *singleton;

            mutable Mutex mutex;
            HashMap<RID, Entry> entries;
            HashMap<String, RID> keys;
            int load_count = 0;
            bool lazy_loading = false;

            void _on_data_unload_requested();

        protected:
            static void _bind_methods();

        public:
            /// Read once, when the loader is created - after UserSettings applied the player's values
            static constexpr const char *LAZY_LOADING_SETTING = "maszyna/resources/lazy_loading";
            static constexpr bool DEFAULT_LAZY_LOADING = false;
            /// Command-line switch: lazy loading whatever the setting says
            static constexpr const char *ARG_ENABLE_LAZY_LOADING = "--enable-lazy-loading";

            static ResourceLazyLoader *get_instance();

            ResourceLazyLoader();
            ~ResourceLazyLoader() override;

            /* Takes effect on the keys registered from now on; one registered already keeps its mode */
            void set_lazy_loading(bool p_lazy_loading);
            bool get_lazy_loading() const;

            /* The same key gives the same RID; each registration is freed by its own resource_free() */
            RID resource_register(const String &p_key, const Callable &p_loader);
            void resource_free(const RID &p_resource);
            /* The held resource, or a new copy loaded without holding it */
            Ref<Resource> resource_load(const RID &p_resource);
            /* Holds the resource until the matching resource_release(): the copy already held, or
               else the given one, loaded by resource_load() */
            Ref<Resource> resource_hold(const RID &p_resource, const Ref<Resource> &p_loaded);
            void resource_release(const RID &p_resource);
            /* Somebody holds the resource */
            bool resource_is_resident(const RID &p_resource) const;
            /* registered, resident, loads - for a debug view */
            Dictionary resource_get_statistics() const;
    };
} // namespace godot
