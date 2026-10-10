#include "E3DInstanceTypes.hpp"
#include "E3DLightFactory.hpp"
#include "E3DNodesBackend.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    E3DNodesBackend::E3DNodesBackend(const bool p_editable) : editable(p_editable) {}

    void E3DNodesBackend::build(E3DInstanceData &p_instance, E3DMaterialResolver &p_material_resolver) {
        Node3D *target = Object::cast_to<Node3D>(ObjectDB::get_instance(p_instance.node_id));
        ERR_FAIL_NULL_MSG(target, "NODES instancer requires a node attached with instance_attach_object_instance_id()");

        // which submodels belong to which light was found by E3DLightFactory, not here
        HashMap<E3DSubModel *, LightRole> light_roles;
        for (const E3DModelLight &light: p_instance.model_lights.lights) {
            if (light.on != nullptr && !light_roles.has(light.on)) {
                light_roles[light.on] = {light.name, LIGHT_PART_ON};
            }
            if (light.off != nullptr && !light_roles.has(light.off)) {
                light_roles[light.off] = {light.name, LIGHT_PART_OFF};
            }
            if (light.xon != nullptr && !light_roles.has(light.xon)) {
                light_roles[light.xon] = {light.name, LIGHT_PART_XON};
            }
        }

        _add_submodels(
                p_instance, target, target, p_instance.model->get_submodels(), Transform3D(), light_roles, String(),
                _get_force_alpha_submodels(p_instance), E3DInstanceTypes::TRANSLUCENCY_CUTOUT, p_material_resolver);
        update(p_instance);
    }

    void E3DNodesBackend::clear(E3DInstanceData &p_instance) {
        for (const ObjectID &node_id: p_instance.root_nodes) {
            Node *node = Object::cast_to<Node>(ObjectDB::get_instance(node_id));
            if (node == nullptr) {
                continue;
            }
            if (Node *parent = node->get_parent(); parent != nullptr) {
                parent->remove_child(node);
            }
            node->queue_free();
        }
        p_instance.root_nodes.clear();
        p_instance.light_nodes.clear();
        p_instance.submodel_nodes.clear();
        p_instance.emissive_materials.clear();
    }

    void E3DNodesBackend::apply_poses(E3DInstanceData &p_instance) {
        for (const KeyValue<E3DSubModel *, Transform3D> &pose: p_instance.submodel_poses) {
            const ObjectID *node_id = p_instance.submodel_nodes.getptr(pose.key);
            if (node_id == nullptr) {
                continue;
            }
            if (Node3D *node = Object::cast_to<Node3D>(ObjectDB::get_instance(*node_id)); node != nullptr) {
                const Transform3D rest = p_instance.root_nodes.has(*node_id)
                                                 ? p_instance.node_transform * pose.key->get_transform()
                                                 : pose.key->get_transform();
                node->set_transform(rest * pose.value);
            }
        }
    }

    bool E3DNodesBackend::intersect_segment(
            const E3DInstanceData &p_instance, const Vector3 &p_from, const Vector3 &p_to, double &p_r_distance,
            Vector3 &p_r_point) const {
        bool hit = false;
        for (const KeyValue<E3DSubModel *, ObjectID> &submodel_node: p_instance.submodel_nodes) {
            const MeshInstance3D *mesh = Object::cast_to<MeshInstance3D>(ObjectDB::get_instance(submodel_node.value));
            if (mesh == nullptr || mesh->get_mesh().is_null() || !mesh->is_visible_in_tree()) {
                continue;
            }
            hit = _intersect_mesh(
                          mesh->get_mesh(), mesh->get_global_transform(), p_from, p_to, p_r_distance, p_r_point) ||
                  hit;
        }
        return hit;
    }

    void E3DNodesBackend::update(const E3DInstanceData &p_instance) {
        const Array light_names = p_instance.lights_state.keys();
        for (int i = 0; i < light_names.size(); i++) {
            const String light_name = light_names[i];
            const HashMap<String, E3DInstanceData::LightNodes>::ConstIterator light =
                    p_instance.light_nodes.find(light_name);
            if (light == p_instance.light_nodes.end()) {
                const Node *target = Object::cast_to<Node>(ObjectDB::get_instance(p_instance.node_id));
                UtilityFunctions::push_warning(
                        "[", target != nullptr ? String(target->get_name()) : String(),
                        "] LightInfo not found for light: ", light_name);
                continue;
            }
            const bool enabled = p_instance.lights_state[light_name];
            const bool has_xon = light->value.xon.is_valid();
            _set_node_visible(light->value.on, _light_part_visible(p_instance, light_name, LIGHT_PART_ON, has_xon));
            _set_node_visible(light->value.off, _light_part_visible(p_instance, light_name, LIGHT_PART_OFF, has_xon));
            _set_node_visible(light->value.xon, _light_part_visible(p_instance, light_name, LIGHT_PART_XON, has_xon));
            _set_node_visible(light->value.spotlight, enabled);
            // a dimmed light shines at the vehicle's DimmedMultiplier (lightarray.cpp:77-78)
            if (SpotLight3D *spotlight = Object::cast_to<SpotLight3D>(ObjectDB::get_instance(light->value.spotlight));
                spotlight != nullptr) {
                const bool dimmed = p_instance.lights_dimmed.get(light_name, false);
                spotlight->set_param(
                        Light3D::PARAM_ENERGY,
                        light->value.spotlight_energy * (dimmed ? p_instance.lights_dimmed_multiplier : 1.0f));
            }
        }
        for (const KeyValue<E3DSubModel *, ObjectID> &submodel_node: p_instance.submodel_nodes) {
            if (GeometryInstance3D *geometry =
                        Object::cast_to<GeometryInstance3D>(ObjectDB::get_instance(submodel_node.value));
                geometry != nullptr) {
                geometry->set_layer_mask(p_instance.layer_mask);
                geometry->set_material_overlay(p_instance.material_overlay);
            }
        }
        // what a client set on named submodels (E3DRenderingServer::instance_set_submodel_*())
        for (const KeyValue<String, E3DInstanceData::SubmodelSettings> &settings: p_instance.submodel_settings) {
            const ObjectID *node_id = p_instance.submodel_nodes.getptr(settings.value.submodel);
            if (node_id == nullptr) {
                continue;
            }
            _set_node_visible(*node_id, _is_submodel_shown(p_instance, settings.value.submodel));
            if (GeometryInstance3D *geometry = Object::cast_to<GeometryInstance3D>(ObjectDB::get_instance(*node_id));
                geometry != nullptr && settings.value.material_override.is_valid()) {
                geometry->set_material_override(settings.value.material_override);
            }
        }
        for (const E3DInstanceData::EmissiveMaterial &emissive: p_instance.emissive_materials) {
            const float *submodel_energy = p_instance.submodel_emission_energies.getptr(emissive.submodel);
            const float energy = submodel_energy != nullptr ? *submodel_energy : p_instance.emission_energy;
            if (energy >= 0.0) {
                emissive.material->set_shader_parameter("emission_energy", energy);
            }
        }
    }

    void E3DNodesBackend::_add_submodels(
            E3DInstanceData &p_instance, Node3D *p_target, Node3D *p_parent, const TypedArray<E3DSubModel> &p_submodels,
            const Transform3D &p_parent_transform, const HashMap<E3DSubModel *, LightRole> &p_light_roles,
            const String &p_parent_light_name, const Vector<E3DSubModel *> &p_force_alpha_submodels,
            const int p_parent_translucency, E3DMaterialResolver &p_material_resolver) {
        const bool is_editor = Engine::get_singleton()->is_editor_hint();

        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<E3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null() || !_is_submodel_valid(submodel.ptr(), p_instance.exclude_node_names)) {
                continue;
            }
            Node3D *child = _create_submodel_node(p_instance, submodel.ptr());
            if (child == nullptr) {
                continue;
            }

            // A spotlight belongs to the light of its nearest "on" ancestor
            String light_name = p_parent_light_name;
            if (const HashMap<E3DSubModel *, LightRole>::ConstIterator role = p_light_roles.find(submodel.ptr());
                role != p_light_roles.end()) {
                E3DInstanceData::LightNodes &light_nodes = p_instance.light_nodes[role->value.light_name];
                if (light_nodes.submodel == nullptr) {
                    light_nodes.submodel = submodel.ptr();
                }
                switch (role->value.part) {
                    case LIGHT_PART_ON:
                        light_nodes.on = ObjectID(child->get_instance_id());
                        light_name = role->value.light_name;
                        break;
                    case LIGHT_PART_XON:
                        light_nodes.xon = ObjectID(child->get_instance_id());
                        light_name = role->value.light_name;
                        break;
                    case LIGHT_PART_OFF:
                        light_nodes.off = ObjectID(child->get_instance_id());
                        break;
                }
            }

            const Transform3D model_transform = p_parent_transform * submodel->get_transform();
            const int translucency = _submodel_translucency(
                    p_instance, submodel.ptr(), p_force_alpha_submodels, p_parent_translucency,
                    E3DInstanceTypes::TRANSLUCENCY_BLENDED);
            if (GeometryInstance3D *geometry = Object::cast_to<GeometryInstance3D>(child); geometry != nullptr) {
                Ref<Material> material = p_material_resolver.resolve(p_instance, submodel.ptr(), translucency);
                // the instance drives its self-illumination (instance_set_emission_energy(),
                // instance_set_submodel_emission_energy()), so it draws with copies of its own
                // rather than the materials every instance shares
                if (const Ref<ShaderMaterial> shader_material = material;
                    (p_instance.emission_energy >= 0.0 || p_instance.submodel_emission) && shader_material.is_valid() &&
                    bool(shader_material->get_shader_parameter("emission_enabled"))) {
                    const Ref<ShaderMaterial> own = shader_material->duplicate();
                    p_instance.emissive_materials.push_back({own, submodel.ptr()});
                    material = own;
                }
                if (material.is_valid()) {
                    geometry->set_material_override(material);
                }
                if (submodel->get_material_colored()) {
                    geometry->set_instance_shader_parameter("albedo_color", submodel->get_diffuse_color());
                }
                if (Object::cast_to<MeshInstance3D>(child) != nullptr &&
                    _requires_alpha_depth_prepass_sorting(material)) {
                    geometry->set_sorting_offset(-1.0);
                }
                if (submodel->get_mesh().is_valid()) {
                    geometry->set_custom_aabb(_visibility_aabb(submodel->get_mesh()->get_aabb(), model_transform));
                }
            }

            p_parent->add_child(child, false, editable ? Node::INTERNAL_MODE_DISABLED : Node::INTERNAL_MODE_BACK);
            p_instance.submodel_nodes[submodel.ptr()] = ObjectID(child->get_instance_id());
            if (p_parent == p_target) {
                p_instance.root_nodes.push_back(ObjectID(child->get_instance_id()));
            }

            // IMPORTANT: applying transform **after** adding to the tree
            // Applying transform before adding may cause issues (especially on windows)
            // a submodel at the top of the model sits where the model sits in the attached node
            const Transform3D transform = p_parent == p_target ? p_instance.node_transform * submodel->get_transform()
                                                               : submodel->get_transform();
            if (SpotLight3D *spotlight = Object::cast_to<SpotLight3D>(child); spotlight != nullptr) {
                // Do not scale SpotLight3D to avoid configuration warnings
                spotlight->set_position(transform.origin);
                spotlight->set_basis(transform.basis.orthonormalized());
                if (!p_parent_light_name.is_empty()) {
                    E3DInstanceData::LightNodes &light_nodes = p_instance.light_nodes[p_parent_light_name];
                    light_nodes.spotlight = ObjectID(spotlight->get_instance_id());
                    _configure_spotlight(spotlight, p_parent_light_name, light_nodes.submodel);
                    light_nodes.spotlight_energy = static_cast<float>(spotlight->get_param(Light3D::PARAM_ENERGY));
                }
                // the point (and the glare) the original draws where the light is
                // (opengl33renderer.cpp:4375-4500); it goes on and off with the spotlight node
                const Ref<Material> point_material =
                        p_material_resolver.resolve(p_instance, submodel.ptr(), E3DInstanceTypes::TRANSLUCENCY_CUTOUT);
                if (point_material.is_valid()) {
                    MeshInstance3D *point = memnew(MeshInstance3D);
                    point->set_mesh(point_mesh);
                    point->set_material_override(point_material);
                    // placed on the screen by its shaders - in a shadow map or in GI a stray square
                    point->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
                    point->set_gi_mode(GeometryInstance3D::GI_MODE_DISABLED);
                    const Dictionary parameters =
                            _free_spotlight_parameters(p_instance, submodel.ptr(), p_parent_light_name);
                    const Array names = parameters.keys();
                    for (int j = 0; j < names.size(); j++) {
                        point->set_instance_shader_parameter(names[j], parameters[names[j]]);
                    }
                    spotlight->add_child(point, false, Node::INTERNAL_MODE_BACK);
                }
            } else {
                child->set_transform(transform);
            }

            if (is_editor) {
                child->set_owner(editable ? p_target->get_owner() : p_target);
            }

            _add_submodels(
                    p_instance, p_target, child, submodel->get_submodels(), model_transform, p_light_roles, light_name,
                    p_force_alpha_submodels, translucency, p_material_resolver);
        }
    }

    Node3D *E3DNodesBackend::_create_submodel_node(const E3DInstanceData &p_instance, E3DSubModel *p_submodel) {
        Node3D *node = nullptr;
        switch (p_submodel->get_submodel_type()) {
            case E3DSubModel::SUBMODEL_TRANSFORM:
                node = memnew(Node3D);
                break;
            case E3DSubModel::SUBMODEL_GL_TRIANGLES: {
                MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
                mesh_instance->set_mesh(p_submodel->get_mesh());
                mesh_instance->set_visibility_range_begin(p_submodel->get_visibility_range_begin());
                mesh_instance->set_visibility_range_end(p_submodel->get_visibility_range_end());
                node = mesh_instance;
                break;
            }
            case E3DSubModel::SUBMODEL_FREE_SPOTLIGHT: {
                const ProjectSettings *settings = ProjectSettings::get_singleton();
                SpotLight3D *spotlight = memnew(SpotLight3D);
                Color color = p_submodel->get_diffuse_color();
                color.a = 1.0;
                spotlight->set_color(color);
                spotlight->set_param(
                        Light3D::PARAM_VOLUMETRIC_FOG_ENERGY,
                        settings->get_setting(
                                E3DLightFactory::VEHICLE_LIGHT_VOLUMETRIC_FOG_ENERGY_SETTING,
                                E3DLightFactory::DEFAULT_VEHICLE_LIGHT_VOLUMETRIC_FOG_ENERGY));
                spotlight->set_shadow(true);
                spotlight->set_shadow_reverse_cull_face(
                        settings->get_setting(E3DLightFactory::LIGHTS_SHADOW_REVERSE_CULL_FACE_SETTING, false));
                spotlight->set_enable_distance_fade(true);
                spotlight->set_distance_fade_begin(E3DLightFactory::VEHICLE_LIGHT_FADE_BEGIN);
                spotlight->set_distance_fade_shadow(E3DLightFactory::VEHICLE_LIGHT_FADE_SHADOW);
                spotlight->set_distance_fade_length(E3DLightFactory::VEHICLE_LIGHT_FADE_LENGTH);
                spotlight->set_param(Light3D::PARAM_RANGE, p_submodel->get_light_range());
                spotlight->set_param(Light3D::PARAM_SPOT_ANGLE, p_submodel->get_light_angle());
                node = spotlight;
                break;
            }
            default:
                return nullptr;
        }
        node->set_name(p_submodel->get_name());
        node->set_visible(_is_submodel_shown(p_instance, p_submodel));
        return node;
    }

    void E3DNodesBackend::_configure_spotlight(
            SpotLight3D *p_spotlight, const String &p_light_name, E3DSubModel *p_submodel) {
        const E3DLightParams params = E3DLightFactory::from_submodel(p_submodel, p_light_name);
        p_spotlight->set_param(Light3D::PARAM_ENERGY, params.energy);
        p_spotlight->set_param(Light3D::PARAM_RANGE, params.range);
        p_spotlight->set_param(Light3D::PARAM_ATTENUATION, params.attenuation);
        p_spotlight->set_param(Light3D::PARAM_SPOT_ATTENUATION, params.spot_attenuation);
    }

    void E3DNodesBackend::_set_node_visible(const ObjectID &p_node_id, const bool p_visible) {
        if (Node3D *node = Object::cast_to<Node3D>(ObjectDB::get_instance(p_node_id)); node != nullptr) {
            node->set_visible(p_visible);
        }
    }
} // namespace godot
