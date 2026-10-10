#include "legacy/scenery/MaszynaTrianglesImporter.hpp"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MaszynaTrianglesImporter::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaTrianglesImporter",
                D_METHOD("import_triangles", "parser", "rotate", "origin", "sink", "range_min", "range_max"),
                &MaszynaTrianglesImporter::import_triangles);
    }

    /// The triangles go to the sink at once, in world space - nothing of the node is kept here
    bool MaszynaTrianglesImporter::import_triangles(
            const Ref<MaszynaParser> &p_parser, const Vector3 &p_rotate, const Vector3 &p_origin,
            const Ref<SceneryTrianglesSink> &p_sink, const float p_range_min, const float p_range_max) {
        if (p_parser.is_null() || p_sink.is_null()) {
            return false;
        }

        String texture;
        const String first_token = p_parser->next_token();
        if (first_token == "material") {
            p_parser->get_tokens_until("endmaterial");
            texture = p_parser->next_token();
        } else {
            texture = first_token;
        }

        const Basis basis = Basis::from_euler(p_rotate);
        PackedVector3Array vertices;
        PackedVector3Array normals;
        PackedVector2Array uvs;

        while (!p_parser->eof_reached()) {
            const String x_token = p_parser->next_token();
            if (x_token.is_empty()) {
                break;
            }
            if (x_token == "endtri") {
                break;
            }

            const String y_token = p_parser->next_token();
            const String z_token = p_parser->next_token();
            const String nx_token = p_parser->next_token();
            const String ny_token = p_parser->next_token();
            const String nz_token = p_parser->next_token();
            const String u_token = p_parser->next_token();
            const String v_token = p_parser->next_token();
            const String stop_token = p_parser->next_token();

            if (!(stop_token == "end" || stop_token == "endtri")) {
                UtilityFunctions::push_error("!!! Incorrect triangle format ");
                return false;
            }

            const Vector3 vertex =
                    basis.xform(Vector3(x_token.to_float(), y_token.to_float(), z_token.to_float())) + p_origin;
            const Vector3 normal =
                    basis.xform(Vector3(nx_token.to_float(), ny_token.to_float(), nz_token.to_float())).normalized();

            vertices.append(vertex);
            normals.append(normal);
            uvs.append(Vector2(u_token.to_float(), v_token.to_float()));

            if (stop_token == "endtri") {
                break;
            }
        }

        vertices.reverse();
        normals.reverse();
        uvs.reverse();

        p_sink->add_triangles(texture, vertices, normals, uvs, p_range_min, p_range_max);
        return true;
    }
} // namespace godot
