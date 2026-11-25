#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// Makes the original's binary terrain file (.btf) of a text terrain file (.txtf), as the original
    /// makes it before loading the text's terrain (terrain_file::prepare(), terrain_file::write(),
    /// sceneterrain.cpp:400-640): the "triangles" of the text put in the cells of the original's
    /// sections - a triangle reaching more than 200 m out of its section cut at the section's edge
    /// (basic_region::RaTriangleDivider(), scene.cpp:1601), the shapes of one material, lighting and
    /// range within 25 m of each other merged (basic_cell::insert(), scene.cpp:264). The file is what
    /// MaszynaBTFTerrainProvider reads, so a terrain kept as text is streamed as one kept in binary.
    class MaszynaBTFTerrainWriter : public RefCounted {
            GDCLASS(MaszynaBTFTerrainWriter, RefCounted)

        protected:
            static void _bind_methods();

        public:
            /// terrain_file::checksum() (sceneterrain.cpp:359): the CRC-32 of zlib
            static uint32_t checksum(const PackedByteArray &p_data);
            /// The binary file of the text at p_text_path written to p_binary_path; false, with the
            /// reason reported, when the text holds what a binary terrain file cannot (a node of
            /// another type, a word outside a node) or the file cannot be written
            static bool write(const String &p_text_path, const String &p_binary_path);
    };
} // namespace godot
