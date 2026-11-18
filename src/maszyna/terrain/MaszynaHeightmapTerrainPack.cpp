#include "maszyna/terrain/MaszynaHeightmapTerrainPack.hpp"

#include "maszyna/scenery/MaszynaBTFTerrainWriter.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    static constexpr uint32_t fourcc(const char p_a, const char p_b, const char p_c, const char p_d) {
        return static_cast<uint32_t>(p_a) | (static_cast<uint32_t>(p_b) << 8) | (static_cast<uint32_t>(p_c) << 16) |
               (static_cast<uint32_t>(p_d) << 24);
    }

    namespace {
        /// The slices of a chunk (heightmapterrain.cpp:37-41); the base heights and the touch-ups
        /// are the editor's, the heights hold what they make
        constexpr uint32_t TAG_CHUNK = fourcc('C', 'H', 'N', 'K');
        constexpr uint32_t TAG_HEIGHTS = fourcc('H', 'G', 'H', 'T');
        constexpr uint32_t TAG_PAINT = fourcc('S', 'P', 'L', 'T');
        constexpr uint32_t CODEC_STORED = 0;
        constexpr uint32_t CODEC_ZSTD = 1;
        constexpr int64_t HEADER_BYTES = 64;
        constexpr int64_t INDEX_ENTRY_BYTES = 16;
        constexpr int64_t SLICE_ENTRY_BYTES = 32;
        /// pack_file::open(): no more slices to a chunk, nor bytes to a slice
        constexpr uint32_t MAX_SLICES = 64;
        constexpr uint32_t MAX_SLICE_BYTES = 256U * 1024U * 1024U;
        /// pack_file::read(): no more cells to a chunk
        constexpr int MAX_CELLS = 4096;
        /// heightmap::no_layer
        constexpr uint16_t NO_LAYER = 0xffff;
        /// heightmap::height_step: metres, the quantum of the stored heights
        constexpr double HEIGHT_STEP_M = 0.005;
        /// The encodings of the heights (encode_heights()): steps above a base, delta coded; floats
        constexpr uint8_t HEIGHTS_STEPS = 0;
        constexpr uint8_t HEIGHTS_FLOATS = 1;
    } // namespace

    /// decode_heights() (heightmapterrain.cpp:123): p_count heights; empty when the data is damaged
    static PackedFloat32Array decode_heights(const PackedByteArray &p_data, const int64_t p_count) {
        PackedFloat32Array heights;
        if (p_data.is_empty()) {
            return heights;
        }
        const uint8_t encoding = p_data[0];
        if (encoding == HEIGHTS_FLOATS && p_data.size() == 1 + (p_count * static_cast<int64_t>(sizeof(float)))) {
            heights.resize(p_count);
            for (int64_t index = 0; index < p_count; index++) {
                heights[index] =
                        static_cast<float>(p_data.decode_float(1 + (index * static_cast<int64_t>(sizeof(float)))));
            }
            return heights;
        }
        constexpr int64_t STEPS_START = 1 + sizeof(int32_t);
        if (encoding != HEIGHTS_STEPS ||
            p_data.size() != STEPS_START + (p_count * static_cast<int64_t>(sizeof(uint16_t)))) {
            return heights;
        }
        const int64_t base = p_data.decode_s32(1);
        heights.resize(p_count);
        uint16_t value = 0;
        for (int64_t index = 0; index < p_count; index++) {
            value = static_cast<uint16_t>(
                    value + static_cast<uint16_t>(
                                    p_data.decode_u16(STEPS_START + (index * static_cast<int64_t>(sizeof(uint16_t))))));
            heights[index] = static_cast<float>(static_cast<double>(base + value) * HEIGHT_STEP_M);
        }
        return heights;
    }

    void MaszynaHeightmapTerrainPack::_bind_methods() {
        ClassDB::bind_method(D_METHOD("open", "path"), &MaszynaHeightmapTerrainPack::open);
        ClassDB::bind_method(D_METHOD("get_key"), &MaszynaHeightmapTerrainPack::get_key);
        ClassDB::bind_method(D_METHOD("get_chunk_keys"), &MaszynaHeightmapTerrainPack::get_chunk_keys);
        ClassDB::bind_method(D_METHOD("read_chunk", "key"), &MaszynaHeightmapTerrainPack::read_chunk);
    }

    bool MaszynaHeightmapTerrainPack::open(const String &p_path) {
        path = p_path;
        for (Vector<Slice> &chunk_slices: slices) {
            chunk_slices.clear();
        }
        const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
        if (file.is_null() || file->get_length() < static_cast<uint64_t>(HEADER_BYTES)) {
            return false;
        }
        const uint32_t magic = file->get_32();
        const uint32_t version = file->get_32();
        const auto pack_x = static_cast<int32_t>(file->get_32());
        const auto pack_z = static_cast<int32_t>(file->get_32());
        const uint32_t side = file->get_32();
        const float size = file->get_float();
        const uint64_t index_offset = file->get_64();
        if (magic != FILE_MAGIC || version != FILE_VERSION ||
            side != static_cast<uint32_t>(MaszynaHeightmapTerrainChunk::PACK_SIDE) ||
            Math::abs(size - MaszynaHeightmapTerrainChunk::CHUNK_SIZE_M) > CMP_EPSILON) {
            UtilityFunctions::push_error("Bad file: \"" + p_path + "\" is no terrain chunks file of a known version");
            return false;
        }
        key = Vector2i(pack_x, pack_z);
        file->seek(index_offset);
        const PackedByteArray index = file->get_buffer(static_cast<int64_t>(slices.size()) * INDEX_ENTRY_BYTES);
        if (index.size() != static_cast<int64_t>(slices.size()) * INDEX_ENTRY_BYTES) {
            UtilityFunctions::push_error("Bad file: \"" + p_path + "\" has a damaged chunk index");
            return false;
        }
        for (size_t chunk = 0; chunk < slices.size(); chunk++) {
            const int64_t entry = static_cast<int64_t>(chunk) * INDEX_ENTRY_BYTES;
            const uint64_t table_offset = index.decode_u64(entry);
            const uint32_t count =
                    static_cast<uint32_t>(index.decode_u32(entry + static_cast<int64_t>(sizeof(uint64_t))));
            if (table_offset == 0 || count == 0) {
                continue;
            }
            file->seek(table_offset);
            const PackedByteArray table = file->get_buffer(static_cast<int64_t>(count) * SLICE_ENTRY_BYTES);
            if (count > MAX_SLICES || table.size() != static_cast<int64_t>(count) * SLICE_ENTRY_BYTES) {
                UtilityFunctions::push_error("Bad file: \"" + p_path + "\" has a damaged slice table");
                return false;
            }
            for (uint32_t index_in_table = 0; index_in_table < count; index_in_table++) {
                const int64_t at = static_cast<int64_t>(index_in_table) * SLICE_ENTRY_BYTES;
                Slice slice;
                slice.tag = static_cast<uint32_t>(table.decode_u32(at));
                slice.codec = static_cast<uint32_t>(table.decode_u32(at + 4));
                slice.offset = table.decode_u64(at + 8);
                slice.stored = static_cast<uint32_t>(table.decode_u32(at + 16));
                slice.size = static_cast<uint32_t>(table.decode_u32(at + 20));
                slice.crc = static_cast<uint32_t>(table.decode_u32(at + 24));
                if (slice.stored > MAX_SLICE_BYTES || slice.size > MAX_SLICE_BYTES) {
                    UtilityFunctions::push_error("Bad file: \"" + p_path + "\" has a damaged slice table");
                    return false;
                }
                slices[chunk].push_back(slice);
            }
        }
        return true;
    }

    Vector2i MaszynaHeightmapTerrainPack::get_key() const {
        return key;
    }

    TypedArray<Vector2i> MaszynaHeightmapTerrainPack::get_chunk_keys() const {
        TypedArray<Vector2i> keys;
        for (int z = 0; z < MaszynaHeightmapTerrainChunk::PACK_SIDE; z++) {
            for (int x = 0; x < MaszynaHeightmapTerrainChunk::PACK_SIDE; x++) {
                if (!slices[(static_cast<size_t>(z) * MaszynaHeightmapTerrainChunk::PACK_SIDE) + x].is_empty()) {
                    keys.append(key * MaszynaHeightmapTerrainChunk::PACK_SIDE + Vector2i(x, z));
                }
            }
        }
        return keys;
    }

    PackedByteArray
    MaszynaHeightmapTerrainPack::_load_slice(const Ref<FileAccess> &p_file, const Slice &p_slice) const {
        p_file->seek(p_slice.offset);
        PackedByteArray data = p_file->get_buffer(p_slice.stored);
        if (data.size() != static_cast<int64_t>(p_slice.stored)) {
            return {};
        }
        if (p_slice.codec == CODEC_ZSTD) {
            data = data.decompress(p_slice.size, FileAccess::COMPRESSION_ZSTD);
        } else if (p_slice.codec != CODEC_STORED) {
            return {};
        }
        if (data.size() != static_cast<int64_t>(p_slice.size) ||
            MaszynaBTFTerrainWriter::checksum(data) != p_slice.crc) {
            return {};
        }
        return data;
    }

    Ref<MaszynaHeightmapTerrainChunk> MaszynaHeightmapTerrainPack::read_chunk(const Vector2i &p_key) const {
        const Vector2i local = p_key - (key * MaszynaHeightmapTerrainChunk::PACK_SIDE);
        if (local.x < 0 || local.y < 0 || local.x >= MaszynaHeightmapTerrainChunk::PACK_SIDE ||
            local.y >= MaszynaHeightmapTerrainChunk::PACK_SIDE) {
            return {};
        }
        const Vector<Slice> &chunk_slices =
                slices[(static_cast<size_t>(local.y) * MaszynaHeightmapTerrainChunk::PACK_SIDE) + local.x];
        if (chunk_slices.is_empty()) {
            return {};
        }
        const Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        ERR_FAIL_COND_V_MSG(file.is_null(), {}, "Bad file: can't open \"" + path + "\"");
        const Slice *heights_slice = nullptr;
        const Slice *paint_slice = nullptr;
        int cells = 0;
        int paint = 0;
        PackedInt32Array layers;
        bool header = false;
        for (const Slice &slice: chunk_slices) {
            if (slice.tag == TAG_CHUNK) {
                const PackedByteArray data = _load_slice(file, slice);
                constexpr int64_t CHUNK_HEADER_BYTES =
                        (2 + MaszynaHeightmapTerrainChunk::MAX_LAYERS) * sizeof(uint16_t);
                ERR_FAIL_COND_V_MSG(
                        data.size() < CHUNK_HEADER_BYTES, {}, "Bad file: \"" + path + "\" has a damaged chunk header");
                cells = static_cast<int>(data.decode_u16(0));
                paint = static_cast<int>(data.decode_u16(2));
                // the layers in use come first (chunk_data::layer_count())
                for (int layer = 0; layer < MaszynaHeightmapTerrainChunk::MAX_LAYERS; layer++) {
                    const auto index = static_cast<uint16_t>(data.decode_u16(4 + (layer * 2)));
                    if (index == NO_LAYER) {
                        break;
                    }
                    layers.push_back(index);
                }
                header = true;
            } else if (slice.tag == TAG_HEIGHTS) {
                heights_slice = &slice;
            } else if (slice.tag == TAG_PAINT) {
                paint_slice = &slice;
            }
        }
        ERR_FAIL_COND_V_MSG(
                !header || heights_slice == nullptr || cells <= 0 || cells > MAX_CELLS, {},
                "Bad file: \"" + path + "\" has an incomplete chunk");
        // a chunk names its first layer at least (pack_file::read())
        if (layers.is_empty()) {
            layers.push_back(0);
        }
        const int64_t count = static_cast<int64_t>(cells + 1) * (cells + 1);
        const PackedFloat32Array heights = decode_heights(_load_slice(file, *heights_slice), count);
        ERR_FAIL_COND_V_MSG(heights.size() != count, {}, "Bad file: \"" + path + "\" has damaged heights");
        PackedByteArray weights;
        if (paint > 0) {
            ERR_FAIL_COND_V_MSG(paint_slice == nullptr, {}, "Bad file: \"" + path + "\" has a chunk without its paint");
            weights = _load_slice(file, *paint_slice);
            ERR_FAIL_COND_V_MSG(
                    weights.size() != static_cast<int64_t>(paint + 1) * (paint + 1) * layers.size(), {},
                    "Bad file: \"" + path + "\" has damaged paint");
        }
        Ref<MaszynaHeightmapTerrainChunk> chunk;
        chunk.instantiate();
        chunk->set_data(p_key, cells, heights, paint, layers, weights);
        return chunk;
    }
} // namespace godot
