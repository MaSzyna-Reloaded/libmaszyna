#pragma once

#include "scenery/SceneryStreamingProvider.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The terrain of the original's binary terrain file (.btf, terrain_file, sceneterrain.cpp): the
    /// "triangles" of a text terrain file (.txtf) arranged by the original's sections, supplied
    /// section by section as the camera comes near - a section is 1 km, the streaming's own cell. The
    /// original takes it in place of its .txtf when it was made from that text, or alone
    /// (terrain_file::prepare()).
    class MaszynaBTFTerrainProvider : public SceneryStreamingProvider {
            GDCLASS(MaszynaBTFTerrainProvider, SceneryStreamingProvider)

        public:
            /// MAKE_ID4('E','U','0','7'), MAKE_ID4('B','T','F','2') and MAKE_ID4('B','T','F','S')
            /// (sceneterrain.cpp:80-82)
            static constexpr uint32_t FILE_HEADER = 0x37305545;
            static constexpr uint32_t FILE_VERSION = 0x32465442;
            static constexpr uint32_t SECTION_BLOCK = 0x53465442;
            /// EU07_SECTIONSIZE, EU07_CELLSIZE and EU07_REGIONSIDESECTIONCOUNT (scene.h:34-36): the
            /// layout the file is arranged for, which it records (read_header(), sceneterrain.cpp:238)
            static constexpr uint32_t SECTION_SIZE_M = 1000;
            static constexpr uint32_t CELL_SIZE_M = 250;
            static constexpr uint32_t REGION_SIDE_SECTION_COUNT = 500;

        private:
            struct Section {
                    uint64_t offset = 0;
                    int64_t size = 0;
                    float overhang = 0.0F;
            };
            mutable Mutex mutex;
            Ref<FileAccess> file;
            PackedStringArray materials;
            /// Tells a translucent material (MaszynaMaterialManager.is_material_translucent()):
            /// the original merges only the opaque shapes over a whole section (terrain_file::load())
            Callable translucency;
            HashMap<Vector2i, Section> sections;

        protected:
            static void _bind_methods();

        public:
            /// The length of the text the file was made from (read_header(), sceneterrain.cpp:221),
            /// what the original checks it against its .txtf by; -1 when it is no terrain file of the
            /// version and layout the original writes
            static int64_t get_source_length(const String &p_path);
            /// Lists the file's materials and sections; false when it is no terrain file
            bool open(const String &p_path, const Callable &p_translucency);

            TypedArray<Vector2i> get_chunk_cells() const override;
            float chunk_get_overhang(const Vector2i &p_cell) const override;
            PackedInt32Array get_content_kinds() const override;
            Array chunk_load(const Vector2i &p_cell, ContentKind p_kind) override;
    };
} // namespace godot
