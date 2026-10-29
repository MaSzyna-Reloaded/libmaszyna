#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* Quirk: the original's data is made on Windows, whose filesystem ignores letter case, and
     * names its files in any case (`glinojeck/glinojeck.ctr` for `glinojeck/Glinojeck.ctr`). On a
     * filesystem that does not, MaszynaDataPath::resolve() finds here a path not found as written
     * nor in lowercase: its directory, found the same way, is listed once and its names kept in
     * memory, and the name is looked up there whatever its case. A directory is listed at most
     * once a run, until the game directory changes: a file added meanwhile is found in the next
     * run. Not on Windows, whose filesystem finds them itself.
     *
     * It also keeps what a scenery's loading found of the data since files_clear()
     * (MaszynaSceneryProvider.compile()): the files found, and the files its readers gave up on
     * (missing_file_add()) - a reader tries candidates (a model's two extensions, a texture's four
     * places), and only it knows when none was there. */
    class MaszynaFileIndexer : public Object {
            GDCLASS(MaszynaFileIndexer, Object)

        public:
            /* What a missing file is to the reader that gave up on it */
            enum FileType {
                MASZYNA_FILE_INCLUDE,
                MASZYNA_FILE_FIZ,
                MASZYNA_FILE_MMD,
                MASZYNA_FILE_MODEL,
                MASZYNA_FILE_TEXTURE,
                MASZYNA_FILE_SOUND,
            };

        private:
            static MaszynaFileIndexer *singleton;

            mutable Mutex mutex;
            /* The filesystem keeps letter case: names of another case are looked up in their
             * directory (not on Windows) */
            const bool case_matching;
            /* A directory listed, absolute -> its names in lowercase -> as they are on disk */
            HashMap<String, HashMap<String, String>> directories;
            /* Since files_clear(): the files found, absolute, and the files given up on, under the
             * game directory when they are there -> their FileType */
            HashSet<String> found_files;
            Dictionary missing_files;

            void _on_game_dir_changed();

        protected:
            static void _bind_methods();

        public:
            static MaszynaFileIndexer *get_instance();

            MaszynaFileIndexer();
            ~MaszynaFileIndexer() override;

            /* The relative path as it is on disk under the base directory, or as written when it is
             * nowhere */
            String path_resolve(const String &p_base_dir, const String &p_relative_path);

            void files_clear();
            void missing_file_add(const String &p_path, FileType p_type);
            /* Path -> FileType */
            Dictionary missing_files_get() const;
            int64_t found_files_get_count() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaFileIndexer::FileType)
