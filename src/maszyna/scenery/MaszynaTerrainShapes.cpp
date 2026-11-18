#include "maszyna/scenery/MaszynaTerrainShapes.hpp"

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

#include <algorithm>

namespace godot {
    MaszynaSectionCursor::MaszynaSectionCursor(const PackedByteArray &p_data) : data(p_data) {}

    int64_t MaszynaSectionCursor::take(const int64_t p_bytes) {
        if (overrun || position + p_bytes > data.size()) {
            overrun = true;
            return -1;
        }
        const int64_t offset = position;
        position += p_bytes;
        return offset;
    }

    void MaszynaSectionCursor::skip(const int64_t p_bytes) {
        take(p_bytes);
    }

    uint32_t MaszynaSectionCursor::u32() {
        const int64_t offset = take(sizeof(uint32_t));
        return offset < 0 ? 0 : static_cast<uint32_t>(data.decode_u32(offset));
    }

    float MaszynaSectionCursor::f32() {
        const int64_t offset = take(sizeof(float));
        return offset < 0 ? 0.0F : static_cast<float>(data.decode_float(offset));
    }

    double MaszynaSectionCursor::f64() {
        const int64_t offset = take(sizeof(double));
        return offset < 0 ? 0.0 : data.decode_double(offset);
    }

    bool MaszynaSectionCursor::boolean() {
        const int64_t offset = take(sizeof(uint16_t));
        return offset >= 0 && data.decode_u16(offset) == 1;
    }

    String MaszynaSectionCursor::string() {
        String text;
        while (true) {
            const int64_t offset = take(1);
            if (offset < 0 || data[offset] == 0) {
                return text;
            }
            text += static_cast<char32_t>(data[offset]);
        }
    }

    Vector3 MaszynaSectionCursor::dvec3() {
        const double x = f64();
        const double y = f64();
        const double z = f64();
        return {x, y, z};
    }

    Vector3 MaszynaSectionCursor::vec3() {
        const float x = f32();
        const float y = f32();
        const float z = f32();
        return {x, y, z};
    }

    void MaszynaTerrainShape::set_ranges_squared(const double p_range_squared_min, const double p_range_squared_max) {
        range_min = static_cast<float>(Math::sqrt(p_range_squared_min));
        range_max = p_range_squared_max >= NO_RANGE_SQUARED ? NO_RANGE
                                                            : static_cast<float>(Math::sqrt(p_range_squared_max));
    }

    void MaszynaTerrainGeometry::append_shape(const MaszynaTerrainShape &p_shape, const Vector3 &p_origin) {
        for (auto vertex = p_shape.vertices.rbegin(); vertex != p_shape.vertices.rend(); ++vertex) {
            const Vector3 position = vertex->position - p_origin;
            vertices.insert(
                    vertices.end(),
                    {static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)});
            normals.insert(
                    normals.end(), {static_cast<float>(vertex->normal.x), static_cast<float>(vertex->normal.y),
                                    static_cast<float>(vertex->normal.z)});
            uvs.insert(uvs.end(), {static_cast<float>(vertex->uv.x), static_cast<float>(vertex->uv.y)});
        }
    }

    void MaszynaTerrainGeometry::append_group(const MaszynaTerrainShape &p_shape, const Vector3 &p_origin) {
        append_shape(p_shape, p_origin);
        group_centres.push_back(p_shape.centre - p_origin);
        group_ends.push_back(static_cast<int32_t>(vertices.size() / MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS));
    }

    void MaszynaTerrainGeometry::close_group() {
        group_centres.push_back(Vector3());
        group_ends.push_back(static_cast<int32_t>(vertices.size() / MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS));
    }

    static PackedFloat32Array to_packed(const std::vector<float> &p_values) {
        PackedFloat32Array packed;
        packed.resize(static_cast<int64_t>(p_values.size()));
        std::copy(p_values.begin(), p_values.end(), packed.ptrw());
        return packed;
    }

    Ref<MaszynaTrianglesChunkGeometry> MaszynaTerrainGeometry::to_chunk_geometry(
            const String &p_texture, const Vector3 &p_origin, const float p_range_min, const float p_range_max) const {
        Ref<MaszynaTrianglesChunkGeometry> geometry;
        geometry.instantiate();
        geometry->set_texture(p_texture);
        geometry->set_origin(p_origin);
        geometry->set_range_min(p_range_min);
        geometry->set_range_max(p_range_max);
        geometry->set_vertices(to_packed(vertices));
        geometry->set_normals(to_packed(normals));
        geometry->set_uvs(to_packed(uvs));
        geometry->set_group_centres(group_centres);
        geometry->set_group_ends(group_ends);
        return geometry;
    }
} // namespace godot
