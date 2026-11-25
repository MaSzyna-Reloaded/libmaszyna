#include "maszyna/terrain/MaszynaHeightmapTerrainChunk.hpp"

#include <godot_cpp/core/math.hpp>

#include <algorithm>

namespace godot {
    namespace {
        /// The channels of an RGBA weight image
        constexpr int CHANNELS = 4;
        constexpr uint8_t FULL_WEIGHT = 255;
    } // namespace

    void MaszynaHeightmapTerrainChunk::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaHeightmapTerrainChunk", D_METHOD("key_get_corner", "key"),
                &MaszynaHeightmapTerrainChunk::key_get_corner);
        ClassDB::bind_static_method(
                "MaszynaHeightmapTerrainChunk", D_METHOD("key_get_pack", "key"),
                &MaszynaHeightmapTerrainChunk::key_get_pack);
        ClassDB::bind_method(
                D_METHOD("set_data", "key", "cells", "heights", "paint", "layers", "weights"),
                &MaszynaHeightmapTerrainChunk::set_data);
        ClassDB::bind_method(D_METHOD("get_key"), &MaszynaHeightmapTerrainChunk::get_key);
        ClassDB::bind_method(D_METHOD("get_cells"), &MaszynaHeightmapTerrainChunk::get_cells);
        ClassDB::bind_method(D_METHOD("get_spacing"), &MaszynaHeightmapTerrainChunk::get_spacing);
        ClassDB::bind_method(D_METHOD("get_heights"), &MaszynaHeightmapTerrainChunk::get_heights);
        ClassDB::bind_method(D_METHOD("get_paint"), &MaszynaHeightmapTerrainChunk::get_paint);
        ClassDB::bind_method(D_METHOD("get_layers"), &MaszynaHeightmapTerrainChunk::get_layers);
        ClassDB::bind_method(D_METHOD("get_weights"), &MaszynaHeightmapTerrainChunk::get_weights);
        ClassDB::bind_method(D_METHOD("get_height", "x", "z"), &MaszynaHeightmapTerrainChunk::get_height);
        ClassDB::bind_method(D_METHOD("get_average_weights"), &MaszynaHeightmapTerrainChunk::get_average_weights);
        ClassDB::bind_method(
                D_METHOD("build_weight_images", "side"), &MaszynaHeightmapTerrainChunk::build_weight_images);
    }

    Vector2 MaszynaHeightmapTerrainChunk::key_get_corner(const Vector2i &p_key) {
        return {static_cast<real_t>(p_key.x * CHUNK_SIZE_M), static_cast<real_t>(p_key.y * CHUNK_SIZE_M)};
    }

    Vector2i MaszynaHeightmapTerrainChunk::key_get_pack(const Vector2i &p_key) {
        // floor_div() of heightmapterrain.cpp
        const auto floor_div = [](const int p_value) {
            return p_value >= 0 ? p_value / PACK_SIDE : -((-p_value + PACK_SIDE - 1) / PACK_SIDE);
        };
        return {floor_div(p_key.x), floor_div(p_key.y)};
    }

    void MaszynaHeightmapTerrainChunk::set_data(
            const Vector2i &p_key, const int p_cells, const PackedFloat32Array &p_heights, const int p_paint,
            const PackedInt32Array &p_layers, const PackedByteArray &p_weights) {
        key = p_key;
        cells = p_cells;
        heights = p_heights;
        paint = p_paint;
        layers = p_layers;
        weights = p_weights;
    }

    Vector2i MaszynaHeightmapTerrainChunk::get_key() const {
        return key;
    }

    int MaszynaHeightmapTerrainChunk::get_cells() const {
        return cells;
    }

    double MaszynaHeightmapTerrainChunk::get_spacing() const {
        return cells > 0 ? CHUNK_SIZE_M / cells : CHUNK_SIZE_M;
    }

    PackedFloat32Array MaszynaHeightmapTerrainChunk::get_heights() const {
        return heights;
    }

    int MaszynaHeightmapTerrainChunk::get_paint() const {
        return paint;
    }

    PackedInt32Array MaszynaHeightmapTerrainChunk::get_layers() const {
        return layers;
    }

    PackedByteArray MaszynaHeightmapTerrainChunk::get_weights() const {
        return weights;
    }

    float MaszynaHeightmapTerrainChunk::get_height(const int p_x, const int p_z) const {
        const int side = cells + 1;
        const int x = CLAMP(p_x, 0, cells);
        const int z = CLAMP(p_z, 0, cells);
        const int64_t index = (static_cast<int64_t>(z) * side) + x;
        return index < heights.size() ? heights[index] : 0.0F;
    }

    PackedFloat32Array MaszynaHeightmapTerrainChunk::get_average_weights() const {
        PackedFloat32Array averages;
        averages.resize(MAX_LAYERS);
        averages.fill(0.0F);
        const int64_t samples = static_cast<int64_t>(paint + 1) * (paint + 1);
        const int64_t used = MIN(layers.size(), static_cast<int64_t>(MAX_LAYERS));
        if (paint <= 0 || weights.size() != samples * layers.size()) {
            // a chunk without paint is covered with its first layer alone
            averages[0] = 1.0F;
            return averages;
        }
        for (int64_t layer = 0; layer < used; layer++) {
            uint64_t sum = 0;
            for (int64_t sample = 0; sample < samples; sample++) {
                sum += weights[(layer * samples) + sample];
            }
            averages[layer] =
                    static_cast<float>(static_cast<double>(sum) / (static_cast<double>(samples) * FULL_WEIGHT));
        }
        return averages;
    }

    Array MaszynaHeightmapTerrainChunk::build_weight_images(const int p_side) const {
        const int own_side = paint + 1;
        const int64_t own_samples = static_cast<int64_t>(own_side) * own_side;
        const int64_t used = layers.size();
        const bool painted = paint > 0 && used > 1 && weights.size() == own_samples * used;
        // one texel of the first layer for a chunk without paint
        int side = 1;
        if (painted) {
            side = p_side > 0 ? MIN(p_side, own_side) : own_side;
        }
        Array images;
        for (int texture = 0; texture < MAX_LAYERS / CHANNELS; texture++) {
            PackedByteArray texels;
            texels.resize(static_cast<int64_t>(side) * side * CHANNELS);
            texels.fill(0);
            for (int channel = 0; channel < CHANNELS; channel++) {
                const int layer = (texture * CHANNELS) + channel;
                if (layer >= used) {
                    break;
                }
                if (!painted) {
                    // a chunk without paint is covered with its first layer alone
                    texels[channel] = layer == 0 ? FULL_WEIGHT : 0;
                    continue;
                }
                for (int z = 0; z < side; z++) {
                    // the nearest of the chunk's own samples
                    const int source_z = side == own_side ? z : (z * (own_side - 1)) / MAX(side - 1, 1);
                    for (int x = 0; x < side; x++) {
                        const int source_x = side == own_side ? x : (x * (own_side - 1)) / MAX(side - 1, 1);
                        texels[(((static_cast<int64_t>(z) * side) + x) * CHANNELS) + channel] =
                                weights[(layer * own_samples) + (static_cast<int64_t>(source_z) * own_side) + source_x];
                    }
                }
            }
            images.push_back(Image::create_from_data(side, side, false, Image::FORMAT_RGBA8, texels));
        }
        return images;
    }
} // namespace godot
