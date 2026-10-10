#include "E3DInstanceBackend.hpp"
#include "E3DInstanceTypes.hpp"
#include "rendering/MousePicking.hpp"
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
    E3DInstanceBackend::E3DInstanceBackend() {
        // a mesh resource, because RenderingServer is not reachable yet while the servers register
        PackedVector3Array vertices;
        vertices.push_back(Vector3(-1.0, -1.0, 0.0));
        vertices.push_back(Vector3(1.0, -1.0, 0.0));
        vertices.push_back(Vector3(1.0, 1.0, 0.0));
        vertices.push_back(Vector3(-1.0, 1.0, 0.0));
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = vertices;
        arrays[Mesh::ARRAY_INDEX] = PackedInt32Array({0, 2, 1, 0, 3, 2});
        point_mesh.instantiate();
        point_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    }

    bool E3DInstanceBackend::_intersect_mesh(
            const Ref<Mesh> &p_mesh, const Transform3D &p_transform, const Vector3 &p_from, const Vector3 &p_to,
            double &p_r_distance, Vector3 &p_r_point) {
        const Transform3D inverse = p_transform.affine_inverse();
        const Vector3 from = inverse.xform(p_from);
        const Vector3 to = inverse.xform(p_to);
        // the box first: it rejects nearly every mesh, and one entered farther than the nearest
        // hit so far cannot hold a nearer triangle - only then are the triangles taken out of it
        Vector3 entry;
        if (!p_mesh->get_aabb().intersects_segment(from, to, &entry) ||
            p_from.distance_to(p_transform.xform(entry)) >= p_r_distance) {
            return false;
        }
        double t = 0.0;
        if (!mouse_picking::intersect_faces(p_mesh->get_faces(), from, to, t)) {
            return false;
        }
        const Vector3 point = p_transform.xform(from + (to - from) * t);
        const double distance = p_from.distance_to(point);
        if (distance >= p_r_distance) {
            return false;
        }
        p_r_distance = distance;
        p_r_point = point;
        return true;
    }

    bool E3DInstanceBackend::_light_part_visible(
            const E3DInstanceData &p_instance, const String &p_light_name, const LightPart p_part,
            const bool p_has_xon) {
        const bool enabled = p_instance.lights_state.get(p_light_name, false);
        const bool dimmed = p_has_xon && bool(p_instance.lights_dimmed.get(p_light_name, false));
        switch (p_part) {
            case LIGHT_PART_ON:
                return enabled && !dimmed;
            case LIGHT_PART_XON:
                return enabled && dimmed;
            case LIGHT_PART_OFF:
                return !enabled;
        }
        return false;
    }

    /// The colour is the submodel's, or the scenery node's `lightcolors` for its light
    /// (DiffuseOverride, opengl33renderer.cpp:4415); the cone and the range are the submodel's
    Dictionary E3DInstanceBackend::_free_spotlight_parameters(
            const E3DInstanceData &p_instance, const E3DSubModel *p_submodel, const String &p_light_name) {
        Color color = p_submodel->get_diffuse_color();
        const E3DInstanceData::LightDeclaration *declaration = p_instance.light_declarations.getptr(p_light_name);
        if (declaration != nullptr && declaration->has_color) {
            color = declaration->color;
        }
        Dictionary parameters;
        parameters["light_color"] = color;
        parameters["cos_hotspot_angle"] = p_submodel->get_cos_hotspot_angle();
        parameters["cos_falloff_angle"] = Math::cos(Math::deg_to_rad(p_submodel->get_light_angle()));
        parameters["max_distance"] = p_submodel->get_visibility_range_end();
        parameters["lights_on_threshold"] = p_submodel->get_lights_on_threshold();
        return parameters;
    }

    bool E3DInstanceBackend::_is_submodel_valid(const E3DSubModel *p_submodel, const Array &p_exclude_node_names) {
        if (p_submodel->get_skip_rendering()) {
            return false;
        }
        switch (p_submodel->get_submodel_type()) {
            case E3DSubModel::SUBMODEL_TRANSFORM:
            case E3DSubModel::SUBMODEL_FREE_SPOTLIGHT:
                return true;
            case E3DSubModel::SUBMODEL_GL_TRIANGLES:
                return !p_exclude_node_names.has(p_submodel->get_name());
            default:
                return false;
        }
    }

    bool E3DInstanceBackend::_is_submodel_shown(const E3DInstanceData &p_instance, const E3DSubModel *p_submodel) {
        const bool dynamic_instance = p_instance.instance_kind == E3DInstanceTypes::INSTANCE_KIND_DYNAMIC;
        return p_submodel->get_visible() &&
               (!(dynamic_instance && p_submodel->get_dynamic_hidden()) ||
                p_instance.shown_submodels.has(p_submodel)) &&
               !p_instance.hidden_submodels.has(p_submodel);
    }

    Vector<E3DSubModel *> E3DInstanceBackend::_get_force_alpha_submodels(const E3DInstanceData &p_instance) {
        Vector<E3DSubModel *> result;
        for (int i = 0; i < p_instance.force_alpha_submodel_paths.size(); i++) {
            const Ref<E3DSubModel> submodel =
                    p_instance.model->get_node_or_null(p_instance.force_alpha_submodel_paths[i]);
            if (submodel.is_valid()) {
                result.push_back(submodel.ptr());
            }
        }
        return result;
    }

    int E3DInstanceBackend::_submodel_translucency(
            const E3DInstanceData &p_instance, E3DSubModel *p_submodel,
            const Vector<E3DSubModel *> &p_force_alpha_submodels, const int p_parent_translucency, const int p_forced) {
        const bool translucent = p_submodel->get_material_transparent() || p_submodel->get_skin_translucent();
        const bool forced = p_parent_translucency != E3DInstanceTypes::TRANSLUCENCY_CUTOUT ||
                            p_force_alpha_submodels.has(p_submodel) || (p_instance.force_alpha && translucent);
        return forced ? p_forced : E3DInstanceTypes::TRANSLUCENCY_CUTOUT;
    }

    bool E3DInstanceBackend::_requires_alpha_depth_prepass_sorting(const Ref<Material> &p_material) {
        const Ref<BaseMaterial3D> base_material = p_material;
        return base_material.is_valid() &&
               base_material->get_transparency() == BaseMaterial3D::TRANSPARENCY_ALPHA_DEPTH_PRE_PASS;
    }

    AABB E3DInstanceBackend::_visibility_aabb(const AABB &p_mesh_aabb, const Transform3D &p_model_transform) {
        const Vector3 origin = p_model_transform.affine_inverse().xform(Vector3());
        const Vector3 extent = (p_mesh_aabb.position - origin).abs().max((p_mesh_aabb.get_end() - origin).abs());
        return {origin - extent, extent * 2.0};
    }
} // namespace godot
