#pragma once

#include "legacy/parsers/maszyna_parser.hpp"
#include "scenery/SceneryTrianglesSink.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    class MaszynaTrianglesImporter : public RefCounted {
            GDCLASS(MaszynaTrianglesImporter, RefCounted);

        protected:
            static void _bind_methods();

        public:
            /* Reads a "triangles" node into the sink; false when it is malformed */
            static bool import_triangles(
                    const Ref<MaszynaParser> &p_parser, const Vector3 &p_rotate, const Vector3 &p_origin,
                    const Ref<SceneryTrianglesSink> &p_sink, float p_range_min, float p_range_max);
    };
} // namespace godot
