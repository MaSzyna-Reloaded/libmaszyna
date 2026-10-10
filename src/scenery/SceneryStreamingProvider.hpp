#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// Supplies scenery content by streaming chunk, the way a map tile server supplies tiles:
    /// SceneryStreamingServer asks for the content of a chunk as the camera comes within reach of it
    /// and lets it go once the camera has left. Only presentation is supplied so - what the
    /// simulation needs whatever the camera does (tracks, traction) never comes from a provider.
    ///
    /// A provider supplies the kinds of content it has and is never asked for the others. This class
    /// is only the interface - implement it in C++ by overriding the virtual methods, or in GDScript
    /// by overriding their script counterparts. chunk_load() runs on the streaming worker thread.
    class SceneryStreamingProvider : public Resource {
            GDCLASS(SceneryStreamingProvider, Resource)

        public:
            /// What a chunk's content is, each kind taken by its own consumer
            /// (SceneryStreamingServer.content_set_consumer())
            enum ContentKind {
                CONTENT_TERRAIN, ///< MaszynaTrianglesChunkGeometry
                CONTENT_MODELS,  ///< SceneryModelPlacement
                CONTENT_SOUNDS,  ///< ScenerySoundPlacement
                CONTENT_KIND_MAX,
            };

        protected:
            static void _bind_methods();

            GDVIRTUAL0RC(TypedArray<Vector2i>, _get_chunk_cells)
            GDVIRTUAL1RC(float, _chunk_get_overhang, Vector2i)
            GDVIRTUAL0RC(PackedInt32Array, _get_content_kinds)
            GDVIRTUAL2R(Array, _chunk_load, Vector2i, int)

        public:
            /// The streaming cells (SceneryStreamingServer.CHUNK_SIZE_M) the provider has content for
            virtual TypedArray<Vector2i> get_chunk_cells() const;
            /// How far the content of a cell reaches out of it [m]
            virtual float chunk_get_overhang(const Vector2i &p_cell) const;
            /// The ContentKind values the provider supplies
            virtual PackedInt32Array get_content_kinds() const;
            /// The content of one kind in a cell - items of the type its ContentKind names
            virtual Array chunk_load(const Vector2i &p_cell, ContentKind p_kind);
    };
} // namespace godot

VARIANT_ENUM_CAST(SceneryStreamingProvider::ContentKind);
