#include "maszyna/scenery/MaszynaTrianglesImporter.hpp"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /* Not the original's: a flat face facing up - a platform, a square, a road, a level ground,
         * one of a scenery's few big triangles - split into ones no longer than DENSE_EDGE [m] a side,
         * for the snow to lie on in drifts the vertices raise. Flat: its normal within some 10
         * degrees of up; split DENSE_DEPTH times at most (4 to the power of it of one) */
        constexpr float DENSE_EDGE = 2.0F;
        constexpr float DENSE_FLAT_MIN = 0.985F;
        constexpr int DENSE_DEPTH = 5;

        struct DenseCorner {
                Vector3 vertex;
                Vector3 normal;
                Vector2 uv;
        };

        DenseCorner middle(const DenseCorner &p_a, const DenseCorner &p_b) {
            return {(p_a.vertex + p_b.vertex) * 0.5F, (p_a.normal + p_b.normal).normalized(), (p_a.uv + p_b.uv) * 0.5F};
        }

        /* The triangle as it is, or its four halves' halves, the same way round */
        void
        densify(const DenseCorner &p_a, const DenseCorner &p_b, const DenseCorner &p_c, const int p_depth,
                PackedVector3Array &r_vertices, PackedVector3Array &r_normals, PackedVector2Array &r_uvs) {
            const float longest =
                    MAX(p_a.vertex.distance_to(p_b.vertex),
                        MAX(p_b.vertex.distance_to(p_c.vertex), p_c.vertex.distance_to(p_a.vertex)));
            if (p_depth >= DENSE_DEPTH || longest <= DENSE_EDGE) {
                for (const DenseCorner *corner: {&p_a, &p_b, &p_c}) {
                    r_vertices.append(corner->vertex);
                    r_normals.append(corner->normal);
                    r_uvs.append(corner->uv);
                }
                return;
            }
            const DenseCorner ab = middle(p_a, p_b);
            const DenseCorner bc = middle(p_b, p_c);
            const DenseCorner ca = middle(p_c, p_a);
            densify(p_a, ab, ca, p_depth + 1, r_vertices, r_normals, r_uvs);
            densify(ab, p_b, bc, p_depth + 1, r_vertices, r_normals, r_uvs);
            densify(ca, bc, p_c, p_depth + 1, r_vertices, r_normals, r_uvs);
            densify(ab, bc, ca, p_depth + 1, r_vertices, r_normals, r_uvs);
        }
    } // namespace

    void MaszynaTrianglesImporter::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaTrianglesImporter",
                D_METHOD("import_triangles", "parser", "rotate", "origin", "sink", "range_min", "range_max", "densify"),
                &MaszynaTrianglesImporter::import_triangles);
    }

    /// The triangles go to the sink at once, in world space - nothing of the node is kept here; the
    /// flat faces facing up split for the snow's drifts when p_densify
    bool MaszynaTrianglesImporter::import_triangles(
            const Ref<MaszynaParser> &p_parser, const Vector3 &p_rotate, const Vector3 &p_origin,
            const Ref<MaszynaTrianglesSink> &p_sink, const float p_range_min, const float p_range_max,
            const bool p_densify) {
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

        // the flat faces facing up split for the snow's drifts, the rest as they are
        PackedVector3Array dense_vertices;
        PackedVector3Array dense_normals;
        PackedVector2Array dense_uvs;
        for (int64_t first = 0; first + 2 < vertices.size(); first += 3) {
            const DenseCorner a{vertices[first], normals[first], uvs[first]};
            const DenseCorner b{vertices[first + 1], normals[first + 1], uvs[first + 1]};
            const DenseCorner c{vertices[first + 2], normals[first + 2], uvs[first + 2]};
            const Vector3 face = (b.vertex - a.vertex).cross(c.vertex - a.vertex);
            const bool flat = p_densify && face.length() > 0.0F && Math::abs(face.normalized().y) >= DENSE_FLAT_MIN &&
                              (a.normal + b.normal + c.normal).y > 0.0F;
            densify(a, b, c, flat ? 0 : DENSE_DEPTH, dense_vertices, dense_normals, dense_uvs);
        }

        p_sink->add_triangles(texture, dense_vertices, dense_normals, dense_uvs, p_range_min, p_range_max);
        return true;
    }
} // namespace godot
