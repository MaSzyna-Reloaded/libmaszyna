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
            /// Keep the spelling from the data first, then try its lowercase relative form; on a
            /// case-sensitive filesystem any other difference in case is found by MaszynaFileIndexer,
            /// which also counts the files found for a scenery's loading
            /// (the data's `przejazdy/plyty3_l.t3d` for `plyty3_L.e3d`). Not found at all, the
            /// spelling from the data, for the error.
            static String resolve(const String &p_base_dir, const String &p_relative_path);
            /// The relative path as written or in lowercase when it is on disk, empty otherwise
            static String find_as_written(const String &p_base_dir, const String &p_relative_path);
    };
} // namespace godot
