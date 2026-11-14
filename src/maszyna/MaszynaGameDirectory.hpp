#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* Where the original's data is - the MaSzyna game directory the player chose ([maszyna]
     * game_dir of UserSettings) or the one found - and the search for an installation. A
     * different directory is different data: the game's data is read again (GameDataServer) and
     * the translations take its lang/ (ReloadedTranslationServer).
     *
     * A data path of it may be mounted from a directory elsewhere (data_path_mount()): the files
     * below that path are read from that directory, the rest from the game directory. A path of
     * the data is made a file on disk by data_path_resolve(), the one road from a path the data
     * names to its file. */
    class MaszynaGameDirectory : public Object {
            GDCLASS(MaszynaGameDirectory, Object)

        public:
            static constexpr const char *GAME_DIR_SECTION = "maszyna";
            static constexpr const char *GAME_DIR_KEY = "game_dir";
            /* The original's translations, below the game directory (translation.cpp:18) */
            static constexpr const char *LANG_DIR = "lang";
            static const char *game_dir_changed_signal;
            static const char *data_path_mounts_changed_signal;

        private:
            static MaszynaGameDirectory *singleton;
            /* The game directory in use, absolute */
            String game_dir;
            /* Data paths read from another directory: the path, in lower case without slashes
             * around it -> the directory, absolute. Read by the loaders' threads */
            HashMap<String, String> mounts;
            mutable Mutex mounts_mutex;

            /* Resolves the game directory again, and tells when it is another one */
            void _update();
            void _on_setting_changed(const String &p_section, const String &p_key);

        protected:
            static void _bind_methods();

        public:
            static MaszynaGameDirectory *get_instance();

            MaszynaGameDirectory();
            ~MaszynaGameDirectory() override;

            String get_game_dir() const;
            void save_game_dir(const String &p_path);
            /* The directory holds the original's data (scenery/, dynamic/, textures/) */
            bool is_game_dir(const String &p_path) const;
            bool is_game_dir_valid() const;
            /* Game directories found in the places an installation is usually kept: the game's
             * own directory first, then the one above it, Steam libraries, drives and home */
            PackedStringArray find_game_dirs() const;
            /* The files below the data path (`dynamic/pkp/ep07_v1`) are read from the directory,
             * whatever the letter case the data spells the path in; mounted again, from the new
             * one. The data is read again, as for another game directory */
            void data_path_mount(const String &p_data_path, const String &p_directory);
            void data_path_unmount(const String &p_data_path);
            /* The file of a path the data names, relative to the game directory: below the
             * longest data path mounted, in its directory, otherwise in the game directory - as
             * MaszynaDataPath::resolve() finds it on disk, as written when it is nowhere */
            String data_path_resolve(const String &p_relative_path) const;
    };
} // namespace godot
