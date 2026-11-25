#pragma once

#include "maszyna/terrain/MaszynaHeightmapTerrainChunk.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <array>

namespace godot {
    /// A pack file of the original's heightmap terrain (.tch, heightmap::pack_file,
    /// scene/heightmapterrain.h): 16 x 16 chunks, each a list of slices - its header, its heights, its
    /// paint - stored or zstd-compressed, with a CRC-32. Opening it reads its index; a chunk is read
    /// when asked for, on any thread.
    class MaszynaHeightmapTerrainPack : public RefCounted {
            GDCLASS(MaszynaHeightmapTerrainPack, RefCounted)

        public:
            /// fourcc('T','C','H','1') and the version read (heightmapterrain.cpp:43-44)
            static constexpr uint32_t FILE_MAGIC = 0x31484354;
            static constexpr uint32_t FILE_VERSION = 1;

        private:
            struct Slice {
                    uint32_t tag = 0;
                    uint32_t codec = 0;
                    uint64_t offset = 0;
                    uint32_t stored = 0;
                    uint32_t size = 0;
                    uint32_t crc = 0;
            };
            String path;
            Vector2i key;
            std::array<
                    Vector<Slice>, static_cast<size_t>(MaszynaHeightmapTerrainChunk::PACK_SIDE) *
                                           MaszynaHeightmapTerrainChunk::PACK_SIDE>
                    slices;

            /// The slice's data, decompressed and checked; empty when it is damaged
            PackedByteArray _load_slice(const Ref<FileAccess> &p_file, const Slice &p_slice) const;

        protected:
            static void _bind_methods();

        public:
            /// The pack's index read; false when the file is no pack of the version and layout the
            /// original writes (pack_file::open())
            bool open(const String &p_path);
            /// Where the pack lies, in packs (pack_file::key())
            Vector2i get_key() const;
            /// The chunks the pack holds (pack_file::chunks())
            TypedArray<Vector2i> get_chunk_keys() const;
            /// The chunk read (pack_file::read()); null when the pack has none there or it is damaged
            Ref<MaszynaHeightmapTerrainChunk> read_chunk(const Vector2i &p_key) const;
    };
} // namespace godot
