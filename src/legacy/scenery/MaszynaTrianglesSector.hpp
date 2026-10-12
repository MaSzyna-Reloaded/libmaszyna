#pragma once

#include "legacy/scenery/MaszynaTrianglesChunkGeometry.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The terrain of one section of the 1 km grid as one file, as the original keeps it
    /// (basic_section, scene.cpp:811): the section's own chunks - a texture each - and its cells'
    /// - a texture and a range in a 250 m cell each, its shapes merged within 25 m its groups
    /// (MaszynaTrianglesChunkGeometry). Each chunk is streamed and built on its own
    /// (MaszynaSceneryChunkRenderingServer), by its index here; the file is read once for all of
    /// them (ResourceLazyLoader, keyed by its path).
    class MaszynaTrianglesSector : public Resource {
            GDCLASS(MaszynaTrianglesSector, Resource)

        private:
            TypedArray<MaszynaTrianglesChunkGeometry> chunks;

        protected:
            static void _bind_methods();

        public:
            void set_chunks(const TypedArray<MaszynaTrianglesChunkGeometry> &p_chunks);
            TypedArray<MaszynaTrianglesChunkGeometry> get_chunks() const;
    };
} // namespace godot
