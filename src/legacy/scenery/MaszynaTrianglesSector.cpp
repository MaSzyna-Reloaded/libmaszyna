#include "legacy/scenery/MaszynaTrianglesSector.hpp"

namespace godot {
    void MaszynaTrianglesSector::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_chunks", "chunks"), &MaszynaTrianglesSector::set_chunks);
        ClassDB::bind_method(D_METHOD("get_chunks"), &MaszynaTrianglesSector::get_chunks);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::ARRAY, "chunks", PROPERTY_HINT_ARRAY_TYPE,
                        MaszynaTrianglesChunkGeometry::get_class_static()),
                "set_chunks", "get_chunks");
    }

    void MaszynaTrianglesSector::set_chunks(const TypedArray<MaszynaTrianglesChunkGeometry> &p_chunks) {
        chunks = p_chunks;
    }

    TypedArray<MaszynaTrianglesChunkGeometry> MaszynaTrianglesSector::get_chunks() const {
        return chunks;
    }
} // namespace godot
