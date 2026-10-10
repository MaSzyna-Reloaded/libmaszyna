#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/translation.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The one catalogue Godot's i18n translates with, in its selected language: the
    /// wrapper's own .po files (`maszyna/locale/translations`) with the game's
    /// `<game_dir>/lang/<language>.po` - the original engine's `locale` (translation.cpp:18-160) -
    /// merged over them, so the game's translation of a msgid wins.
    ///
    /// It is the only translation in TranslationServer: every Control, `tr()` and `atr()` looks
    /// up through it, and re-translates itself when the language changes.
    class MaszynaTranslationServer : public Object {
            GDCLASS(MaszynaTranslationServer, Object)

        public:
            static MaszynaTranslationServer *get_instance() {
                return Object::cast_to<MaszynaTranslationServer>(
                        Engine::get_singleton()->get_singleton("MaszynaTranslationServer"));
            }

            /// The wrapper's own .po files, merged under the game's catalogue
            static constexpr const char *TRANSLATIONS_SETTING = "maszyna/locale/translations";
            /// The original's own strings, untranslated - no catalogue needed.
            static constexpr const char *DEFAULT_LANGUAGE = "en";
            /// The game's data was read again (another game directory): translation_get_languages()
            /// lists what it has now
            static constexpr const char *TRANSLATION_LANGUAGES_CHANGED_SIGNAL = "translation_languages_changed";

        private:
            /// The merged catalogue registered in TranslationServer
            Ref<Translation> translation;
            PackedStringArray languages;
            String language = DEFAULT_LANGUAGE;

            void _on_data_reload_requested();
            void _on_language_changed();

        protected:
            static void _bind_methods();

        public:
            MaszynaTranslationServer();
            ~MaszynaTranslationServer() override;

            /// Replaces the game's catalogue with the one in `p_po_path` and puts the merged catalogue
            /// in TranslationServer in place of the previous one; with no such file only the
            /// wrapper's own strings are translated, as in the original (translation.cpp:21)
            void translation_load(const String &p_po_path);
            /// The language of the game's strings, by the name of its catalogue in `lang/`.
            void set_language(const String &p_language);
            String get_language() const;
            /// The languages there is a catalogue for in the game's `lang/`, English (the msgids
            /// themselves) always first
            PackedStringArray translation_get_languages() const;
    };
} // namespace godot
