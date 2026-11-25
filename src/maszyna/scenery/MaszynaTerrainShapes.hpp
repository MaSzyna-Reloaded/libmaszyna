#pragma once

#include "maszyna/scenery/MaszynaTrianglesChunkGeometry.hpp"

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <limits>
#include <vector>

namespace godot {
    /// The little-endian values of one section of the original's binary scenery files - a region
    /// file (.sbt, sn_utils) and a terrain file (.btf) - read out of one buffer. A read past its end
    /// marks an overrun and gives 0.
    class MaszynaSectionCursor {
            const PackedByteArray &data;
            int64_t position = 0;

            /// The offset of the next p_bytes, marking an overrun when they are not there
            int64_t take(int64_t p_bytes);

        public:
            bool overrun = false;

            explicit MaszynaSectionCursor(const PackedByteArray &p_data);

            void skip(int64_t p_bytes);
            uint32_t u32();
            float f32();
            double f64();
            /// sn_utils::d_bool(): a uint16 that is 1
            bool boolean();
            /// sn_utils::d_str(): bytes up to a zero, as the scenery parser reads them
            String string();
            Vector3 dvec3();
            Vector3 vec3();
    };

    struct MaszynaTerrainVertex {
            Vector3 position;
            Vector3 normal;
            Vector2 uv;
    };

    /// One shape of a binary scenery file, in world space
    struct MaszynaTerrainShape {
            /// A visibility range squared at this or above is no limit (shape_node::import(),
            /// scenenode.cpp:157)
            static constexpr double NO_RANGE_SQUARED = std::numeric_limits<double>::max();
            /// The range a shape without a limit is drawn with
            static constexpr float NO_RANGE = -1.0F;

            String texture;
            float range_min = 0.0F;
            float range_max = 0.0F;
            /// What the original measures the shape's range from (bounding_area, scenenode.cpp:41)
            Vector3 centre;
            std::vector<MaszynaTerrainVertex> vertices;

            /// The ranges in metres of the squared ones a file keeps
            void set_ranges_squared(double p_range_squared_min, double p_range_squared_max);
    };

    /// The vectors of a chunk of terrain, relative to its origin, as floats, and its groups: each
    /// one shape or one texture of a section (MaszynaTrianglesChunkGeometry)
    struct MaszynaTerrainGeometry {
            std::vector<float> vertices;
            std::vector<float> normals;
            std::vector<float> uvs;
            PackedVector3Array group_centres;
            PackedInt32Array group_ends;

            /// The shape's triangles relative to p_origin, turned around as MaszynaTrianglesImporter
            /// turns a "triangles" node
            void append_shape(const MaszynaTerrainShape &p_shape, const Vector3 &p_origin);
            /// The shape appended as a group of its own, its centre relative to p_origin
            void append_group(const MaszynaTerrainShape &p_shape, const Vector3 &p_origin);
            /// Everything appended as one group, drawn whatever its range - a section's texture
            /// (Render(shape, true), opengl33renderer.cpp:2495)
            void close_group();
            Ref<MaszynaTrianglesChunkGeometry> to_chunk_geometry(
                    const String &p_texture, const Vector3 &p_origin, float p_range_min, float p_range_max) const;
    };
} // namespace godot
