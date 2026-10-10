#include "MousePicking.hpp"

namespace godot::mouse_picking {
    Ref<StandardMaterial3D> outline_material(const double p_width, const float p_fill_alpha) {
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
        material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
        material->set_albedo(Color(OUTLINE_COLOR, p_fill_alpha));
        material->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
        material->set_stencil_mode(BaseMaterial3D::STENCIL_MODE_OUTLINE);
        material->set_stencil_effect_color(OUTLINE_COLOR);
        material->set_stencil_effect_outline_thickness(static_cast<float>(p_width));
        const Ref<BaseMaterial3D> outline = material->get_next_pass();
        outline->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
        return material;
    }

    bool intersect_faces(const PackedVector3Array &p_faces, const Vector3 &p_from, const Vector3 &p_to, double &p_r_t) {
        // Moller-Trumbore over the segment from..to, t in [0, 1]
        const Vector3 direction = p_to - p_from;
        double nearest_t = 2.0;
        const Vector3 *faces = p_faces.ptr();
        for (int64_t i = 0; i + 2 < p_faces.size(); i += 3) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic): the loop bound keeps i + 2 in range;
            // ptr() avoids a call through the extension interface per vertex
            const Vector3 edge_1 = faces[i + 1] - faces[i];
            const Vector3 edge_2 = faces[i + 2] - faces[i];
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const Vector3 p = direction.cross(edge_2);
            const double determinant = edge_1.dot(p);
            if (Math::is_zero_approx(determinant)) {
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): in range, see above
            const Vector3 s = p_from - faces[i];
            const double u = s.dot(p) / determinant;
            if (u < 0.0 || u > 1.0) {
                continue;
            }
            const Vector3 q = s.cross(edge_1);
            const double v = direction.dot(q) / determinant;
            if (v < 0.0 || u + v > 1.0) {
                continue;
            }
            const double t = edge_2.dot(q) / determinant;
            if (t >= 0.0 && t < nearest_t) {
                nearest_t = t;
            }
        }
        if (nearest_t > 1.0) {
            return false;
        }
        p_r_t = nearest_t;
        return true;
    }
} // namespace godot::mouse_picking
