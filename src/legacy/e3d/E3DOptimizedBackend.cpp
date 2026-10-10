#include "E3DInstanceTypes.hpp"
#include "E3DOptimizedBackend.hpp"
#include <godot_cpp/classes/rendering_server.hpp>

namespace godot {
    void E3DOptimizedBackend::build(E3DInstanceData &p_instance, E3DMaterialResolver &p_material_resolver) {
        // which submodels belong to which light was found by E3DLightFactory, not here
        for (const E3DModelLight &light: p_instance.model_lights.lights) {
            E3DInstanceData::LightSubmodels light_submodels;
            light_submodels.on = light.on;
            light_submodels.off = light.off;
            light_submodels.xon = light.xon;
            p_instance.light_submodels[light.name] = light_submodels;
        }

        _add_submodels(
                p_instance, p_instance.model->get_submodels(), Transform3D(), Vector<E3DSubModel *>(),
                _get_force_alpha_submodels(p_instance), E3DInstanceTypes::TRANSLUCENCY_CUTOUT, p_material_resolver);
        update(p_instance);
    }

    void E3DOptimizedBackend::clear(E3DInstanceData &p_instance) {
        RenderingServer *rs = RenderingServer::get_singleton();
        if (rs != nullptr) {
            for (const RID &rid: p_instance.rids) {
                rs->free_rid(rid);
            }
        }
        p_instance.rids.clear();
        p_instance.local_transforms.clear();
        p_instance.chains.clear();
        p_instance.materials.clear();
        p_instance.light_submodels.clear();
    }

    void E3DOptimizedBackend::update(const E3DInstanceData &p_instance) {
        // Submodel visibility overrides from the light state, same as E3DNodesBackend::update()
        HashMap<E3DSubModel *, bool> overrides;
        for (const KeyValue<String, E3DInstanceData::LightSubmodels> &light: p_instance.light_submodels) {
            if (!p_instance.lights_state.has(light.key)) {
                continue;
            }
            const bool has_xon = light.value.xon != nullptr;
            if (light.value.on != nullptr) {
                overrides[light.value.on] = _light_part_visible(p_instance, light.key, LIGHT_PART_ON, has_xon);
            }
            if (light.value.off != nullptr) {
                overrides[light.value.off] = _light_part_visible(p_instance, light.key, LIGHT_PART_OFF, has_xon);
            }
            if (has_xon) {
                overrides[light.value.xon] = _light_part_visible(p_instance, light.key, LIGHT_PART_XON, has_xon);
            }
        }

        RenderingServer *rs = RenderingServer::get_singleton();
        const RID overlay = p_instance.material_overlay.is_valid() ? p_instance.material_overlay->get_rid() : RID();
        for (int i = 0; i < p_instance.rids.size(); i++) {
            // A submodel is rendered only if all its ancestors are visible (node hierarchy semantics)
            bool visible = p_instance.visible;
            for (E3DSubModel *submodel: p_instance.chains[i]) {
                const HashMap<E3DSubModel *, bool>::ConstIterator override = overrides.find(submodel);
                visible = visible &&
                          (override == overrides.end() ? _is_submodel_shown(p_instance, submodel) : override->value);
            }
            rs->instance_set_transform(p_instance.rids[i], p_instance.transform * p_instance.local_transforms[i]);
            rs->instance_set_visible(p_instance.rids[i], visible);
            rs->instance_set_layer_mask(p_instance.rids[i], p_instance.layer_mask);
            rs->instance_geometry_set_material_overlay(p_instance.rids[i], overlay);
            // the chain ends with the submodel itself (instance_set_submodel_material_override())
            if (const Ref<Material> *material =
                        p_instance.submodel_materials.getptr(p_instance.chains[i][p_instance.chains[i].size() - 1]);
                material != nullptr) {
                rs->instance_geometry_set_material_override(p_instance.rids[i], (*material)->get_rid());
            }
        }
    }

    void E3DOptimizedBackend::apply_transform(const E3DInstanceData &p_instance) {
        RenderingServer *rs = RenderingServer::get_singleton();
        ERR_FAIL_NULL(rs);
        for (int i = 0; i < p_instance.rids.size(); i++) {
            rs->instance_set_transform(p_instance.rids[i], p_instance.transform * p_instance.local_transforms[i]);
        }
    }

    bool E3DOptimizedBackend::intersect_segment(
            const E3DInstanceData &p_instance, const Vector3 &p_from, const Vector3 &p_to, double &p_r_distance,
            Vector3 &p_r_point) const {
        bool hit = false;
        for (int i = 0; i < p_instance.rids.size(); i++) {
            // the chain ends with the submodel itself; a free spotlight's point quad is no geometry
            const E3DSubModel *submodel = p_instance.chains[i][p_instance.chains[i].size() - 1];
            if (submodel->get_submodel_type() != E3DSubModel::SUBMODEL_GL_TRIANGLES) {
                continue;
            }
            hit = _intersect_mesh(
                          submodel->get_mesh(), p_instance.transform * p_instance.local_transforms[i], p_from, p_to,
                          p_r_distance, p_r_point) ||
                  hit;
        }
        return hit;
    }

    /// The chain's transforms again, each submodel with its pose on top (TSubModel::RaAnimation(),
    /// Model3d.cpp:1130-1160)
    void E3DOptimizedBackend::apply_poses(E3DInstanceData &p_instance) {
        RenderingServer *rs = RenderingServer::get_singleton();
        ERR_FAIL_NULL(rs);
        for (int i = 0; i < p_instance.rids.size(); i++) {
            Transform3D local_transform;
            for (E3DSubModel *submodel: p_instance.chains[i]) {
                local_transform = local_transform * submodel->get_transform();
                if (const Transform3D *pose = p_instance.submodel_poses.getptr(submodel); pose != nullptr) {
                    local_transform = local_transform * *pose;
                }
            }
            p_instance.local_transforms.write[i] = local_transform;
            rs->instance_set_transform(p_instance.rids[i], p_instance.transform * local_transform);
        }
    }

    void E3DOptimizedBackend::_add_submodels(
            E3DInstanceData &p_instance, const TypedArray<E3DSubModel> &p_submodels,
            const Transform3D &p_parent_transform, const Vector<E3DSubModel *> &p_parent_chain,
            const Vector<E3DSubModel *> &p_force_alpha_submodels, const int p_parent_translucency,
            E3DMaterialResolver &p_material_resolver) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<E3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null() || !_is_submodel_valid(submodel.ptr(), p_instance.exclude_node_names)) {
                continue;
            }

            const Transform3D local_transform = p_parent_transform * submodel->get_transform();
            Vector<E3DSubModel *> chain = p_parent_chain;
            chain.push_back(submodel.ptr());
            const int translucency = _submodel_translucency(
                    p_instance, submodel.ptr(), p_force_alpha_submodels, p_parent_translucency,
                    E3DInstanceTypes::TRANSLUCENCY_OPAQUE);

            if (submodel->get_submodel_type() == E3DSubModel::SUBMODEL_GL_TRIANGLES &&
                submodel->get_mesh().is_valid()) {
                const RID rid = _add_submodel(
                        p_instance, submodel.ptr(), submodel->get_mesh()->get_rid(),
                        p_material_resolver.resolve(p_instance, submodel.ptr(), translucency), local_transform, chain);
                RenderingServer::get_singleton()->instance_set_custom_aabb(
                        rid, _visibility_aabb(submodel->get_mesh()->get_aabb(), local_transform));
            }
            // A free spotlight's light is E3DRenderingServer's (streamed with a range of its own);
            // here it is only the point (and the glare) the original draws where the light is
            // (opengl33renderer.cpp:4375-4500), when the resolver gives it a material
            if (submodel->get_submodel_type() == E3DSubModel::SUBMODEL_FREE_SPOTLIGHT) {
                const Ref<Material> material =
                        p_material_resolver.resolve(p_instance, submodel.ptr(), E3DInstanceTypes::TRANSLUCENCY_CUTOUT);
                if (material.is_valid()) {
                    const RID rid = _add_submodel(
                            p_instance, submodel.ptr(), point_mesh->get_rid(), material, local_transform, chain);
                    RenderingServer *rs = RenderingServer::get_singleton();
                    // the quad is placed on the screen by its shaders - in a shadow map or in GI it
                    // would be a stray square
                    rs->instance_geometry_set_cast_shadows_setting(rid, RenderingServer::SHADOW_CASTING_SETTING_OFF);
                    rs->instance_geometry_set_flag(rid, RenderingServer::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
                    // the light of the nearest "on" ancestor, as E3DLightFactory assigns it
                    String light_name;
                    for (int ancestor = static_cast<int>(chain.size() - 1); ancestor >= 0 && light_name.is_empty();
                         ancestor--) {
                        for (const E3DModelLight &light: p_instance.model_lights.lights) {
                            if (light.on == chain[ancestor]) {
                                light_name = light.name;
                            }
                        }
                    }
                    const Dictionary parameters = _free_spotlight_parameters(p_instance, submodel.ptr(), light_name);
                    const Array names = parameters.keys();
                    for (int i = 0; i < names.size(); i++) {
                        rs->instance_geometry_set_shader_parameter(rid, names[i], parameters[names[i]]);
                    }
                }
            }

            _add_submodels(
                    p_instance, submodel->get_submodels(), local_transform, chain, p_force_alpha_submodels,
                    translucency, p_material_resolver);
        }
    }

    RID E3DOptimizedBackend::_add_submodel(
            E3DInstanceData &p_instance, E3DSubModel *p_submodel, const RID &p_mesh, const Ref<Material> &p_material,
            const Transform3D &p_local_transform, const Vector<E3DSubModel *> &p_chain) {
        RenderingServer *rs = RenderingServer::get_singleton();
        const RID rid = rs->instance_create();
        if (p_instance.node_id.is_valid()) {
            rs->instance_attach_object_instance_id(rid, p_instance.node_id);
        }
        rs->instance_set_base(rid, p_mesh);
        rs->instance_set_scenario(rid, p_instance.scenario);

        // the instance range (scenery node range_min/range_max) limits the submodel's own range
        float range_begin = MAX(p_submodel->get_visibility_range_begin(), p_instance.visibility_range_begin);
        float range_end = p_submodel->get_visibility_range_end();
        if (p_instance.visibility_range_end > 0.0 &&
            (range_end <= 0.0 || p_instance.visibility_range_end < range_end)) {
            range_end = p_instance.visibility_range_end;
        }
        rs->instance_geometry_set_visibility_range(
                rid, range_begin, range_end, 0.0, 0.0, RenderingServer::VISIBILITY_RANGE_FADE_DISABLED);

        if (p_material.is_valid()) {
            p_instance.materials.push_back(p_material);
            rs->instance_geometry_set_material_override(rid, p_material->get_rid());
            // same as MeshInstance3D.sorting_offset = -1 in E3DNodesBackend
            if (_requires_alpha_depth_prepass_sorting(p_material)) {
                rs->instance_set_pivot_data(rid, -1.0, false);
            }
        }
        if (p_submodel->get_material_colored()) {
            rs->instance_geometry_set_shader_parameter(rid, "albedo_color", p_submodel->get_diffuse_color());
        }

        p_instance.rids.push_back(rid);
        p_instance.local_transforms.push_back(p_local_transform);
        p_instance.chains.push_back(p_chain);
        return rid;
    }
} // namespace godot
