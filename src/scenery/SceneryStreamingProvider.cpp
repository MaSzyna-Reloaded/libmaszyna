#include "scenery/SceneryStreamingProvider.hpp"

namespace godot {
    void SceneryStreamingProvider::_bind_methods() {
        GDVIRTUAL_BIND(_get_chunk_cells)
        GDVIRTUAL_BIND(_chunk_get_overhang, "cell")
        GDVIRTUAL_BIND(_get_content_kinds)
        GDVIRTUAL_BIND(_chunk_load, "cell", "kind")
        ClassDB::bind_method(D_METHOD("get_chunk_cells"), &SceneryStreamingProvider::get_chunk_cells);
        ClassDB::bind_method(D_METHOD("chunk_get_overhang", "cell"), &SceneryStreamingProvider::chunk_get_overhang);
        ClassDB::bind_method(D_METHOD("get_content_kinds"), &SceneryStreamingProvider::get_content_kinds);
        ClassDB::bind_method(D_METHOD("chunk_load", "cell", "kind"), &SceneryStreamingProvider::chunk_load);

        BIND_ENUM_CONSTANT(CONTENT_TERRAIN);
        BIND_ENUM_CONSTANT(CONTENT_MODELS);
        BIND_ENUM_CONSTANT(CONTENT_SOUNDS);
        BIND_ENUM_CONSTANT(CONTENT_KIND_MAX);
    }

    TypedArray<Vector2i> SceneryStreamingProvider::get_chunk_cells() const {
        TypedArray<Vector2i> cells;
        GDVIRTUAL_CALL(_get_chunk_cells, cells);
        return cells;
    }

    float SceneryStreamingProvider::chunk_get_overhang(const Vector2i &p_cell) const {
        float overhang = 0.0F;
        GDVIRTUAL_CALL(_chunk_get_overhang, p_cell, overhang);
        return overhang;
    }

    PackedInt32Array SceneryStreamingProvider::get_content_kinds() const {
        PackedInt32Array kinds;
        GDVIRTUAL_CALL(_get_content_kinds, kinds);
        return kinds;
    }

    Array SceneryStreamingProvider::chunk_load(const Vector2i &p_cell, const ContentKind p_kind) {
        Array items;
        GDVIRTUAL_CALL(_chunk_load, p_cell, static_cast<int>(p_kind), items);
        return items;
    }
} // namespace godot
