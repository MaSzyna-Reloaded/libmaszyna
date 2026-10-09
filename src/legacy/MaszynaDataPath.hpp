#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// Resolves paths authored in a MaSzyna databack under a caller-owned base directory.
    class MaszynaDataPath : public Object {
            GDCLASS(MaszynaDataPath, Object)

        protected:
            static void _bind_methods();

        public:
            /// Quirk: MaSzyna databacks are authored for Windows' case-insensitive filesystem.
            /// Keep the spelling from the data first, then try its lowercase relative form, then
            /// match each part of it letter case aside.
            static String resolve(const String &p_base_dir, const String &p_relative_path);
    };
} // namespace godot
