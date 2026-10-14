#pragma once

#include "legacy/scenery/MaszynaTrianglesChunkGeometry.hpp"
#include "scenery/SceneryStreamingProvider.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The terrain of the original's binary region file (.sbt, basic_region::serialize(),
    /// scene.cpp:1141), supplied section by section as the camera comes near - a section is 1 km, the
    /// streaming's own cell. The file holds the "triangles" shapes of a scenery the original dumped,
    /// or the only copy of a terrain the datapack ships without its text (MASZYNA_ORIGINAL_QUIRKS.md,
    /// "Terrain that exists only as an SBT"); its lines are left out, as "lines" nodes are.
    class MaszynaLegacySBTTerrainProvider : public SceneryStreamingProvider {
            GDCLASS(MaszynaLegacySBTTerrainProvider, SceneryStreamingProvider)

        public:
            /// MAKE_ID4('E','U','0','7') and MAKE_ID4('S','B','T','2') (scene.cpp:28-29)
            static constexpr uint32_t FILE_HEADER = 0x37305545;
            static constexpr uint32_t FILE_VERSION = 0x32544253;
            /// Cells of a section, (EU07_SECTIONSIZE / EU07_CELLSIZE)^2 (scene.h:34-35, :344)
            static constexpr int SECTION_CELL_COUNT = 16;
            /// EU07_REGIONSIDESECTIONCOUNT (scene.h:36): a section's index is row * this + column
            static constexpr int REGION_SIDE_SECTION_COUNT = 500;

        private:
            struct Section {
                    uint64_t offset = 0;
                    int64_t size = 0;
                    float overhang = 0.0F;
            };
            mutable Mutex mutex;
            Ref<FileAccess> file;
            HashMap<Vector2i, Section> sections;

        protected:
            static void _bind_methods();

        public:
            /// basic_region::is_scene() (scene.cpp:1110): the file is there and a region file of the
            /// version the original writes
            static bool is_region(const String &p_path);
            /// Lists the file's sections; false when it is no region file
            bool open(const String &p_path);

            TypedArray<Vector2i> get_chunk_cells() const override;
            float chunk_get_overhang(const Vector2i &p_cell) const override;
            PackedInt32Array get_content_kinds() const override;
            Array chunk_load(const Vector2i &p_cell, ContentKind p_kind) override;
    };
} // namespace godot
