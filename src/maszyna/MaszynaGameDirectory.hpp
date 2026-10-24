#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* Where the original's data is - the MaSzyna game directory the player chose ([maszyna]
     * game_dir of UserSettings) or the one found - and the search for an installation. A
     * different directory is different data: the game's data is read again (GameDataServer) and
     * the translations take its lang/ (ReloadedTranslationServer). */
    class MaszynaGameDirectory : public Object {
            GDCLASS(MaszynaGameDirectory, Object)

        public:
            static constexpr const char *GAME_DIR_SECTION = "maszyna";
            static constexpr const char *GAME_DIR_KEY = "game_dir";
            /* The original's translations, below the game directory (translation.cpp:18) */
            static constexpr const char *LANG_DIR = "lang";
            static const char *game_dir_changed_signal;

        private:
            static MaszynaGameDirectory *singleton;
            /* The game directory in use, absolute */
            String game_dir;

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
    };
} // namespace godot
