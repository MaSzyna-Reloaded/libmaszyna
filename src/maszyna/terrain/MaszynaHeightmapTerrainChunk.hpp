#pragma once

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// A chunk of the original's heightmap terrain (heightmap::chunk_data, scene/heightmapterrain.h):
    /// a square of CHUNK_SIZE_M, a grid of heights of its own point spacing and the weights of the
    /// layers painted over it - as read from a pack file (MaszynaHeightmapTerrainPack) - and the
    /// textures of its weights.
    class MaszynaHeightmapTerrainChunk : public RefCounted {
            GDCLASS(MaszynaHeightmapTerrainChunk, RefCounted)

        public:
            /// heightmap::chunk_size: metres, the side of a chunk
            static constexpr double CHUNK_SIZE_M = 250.0;
            /// heightmap::max_chunk_layers: the layers painted over one chunk
            static constexpr int MAX_LAYERS = 8;
            /// heightmap::pack_side: the chunks along the side of a pack - the textures repeat over it
            static constexpr int PACK_SIDE = 16;

        private:
            Vector2i key;
            int cells = 0;
            PackedFloat32Array heights;
            int paint = 0;
            PackedInt32Array layers;
            PackedByteArray weights;

        protected:
            static void _bind_methods();

        public:
            /// The corner with the lowest coordinates of the chunk of p_key (heightmap::chunk_corner())
            static Vector2 key_get_corner(const Vector2i &p_key);
            /// The pack holding the chunk of p_key (heightmap::pack_of())
            static Vector2i key_get_pack(const Vector2i &p_key);

            /// Takes what a pack file holds of the chunk: its grid of (p_cells + 1)^2 heights in rows
            /// of growing z, its paint samples along a side less one, the palette indices of its layers,
            /// the weights of each layer in use, planar
            void set_data(
                    const Vector2i &p_key, int p_cells, const PackedFloat32Array &p_heights, int p_paint,
                    const PackedInt32Array &p_layers, const PackedByteArray &p_weights);

            Vector2i get_key() const;
            int get_cells() const;
            /// Metres between the points of the grid
            double get_spacing() const;
            PackedFloat32Array get_heights() const;
            int get_paint() const;
            /// The palette indices of the layers in use, the used ones first
            PackedInt32Array get_layers() const;
            PackedByteArray get_weights() const;
            /// The height of the grid point (p_x, p_z), clamped to the chunk
            float get_height(int p_x, int p_z) const;

            /// The weight of each layer in use over the whole chunk, 0..1 - how it looks from afar;
            /// MAX_LAYERS of them
            PackedFloat32Array get_average_weights() const;
            /// The weights of the first four layers and of the next four as the channels of two
            /// images, p_side samples along a side (the chunk's own when 0); one painted layer: a texel
            /// of it alone
            Array build_weight_images(int p_side) const;
    };
} // namespace godot
