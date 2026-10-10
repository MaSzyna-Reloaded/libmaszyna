#include "E3DLightFactory.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        constexpr const char *STREET_LAMP_PREFIX = "latarnia";
        /// The halo billboards around the lamp head, and the lit patch on the ground
        constexpr const char *HALO_MATERIAL = "poswiata";
        constexpr const char *POOL_MATERIAL_1 = "light1";
        constexpr const char *POOL_MATERIAL_2 = "light2";

        struct StreetLampAnchors {
                Vector<Vector3> heads; // one per lamp arm, in model space
                /// Half-width of the lit patch across the lamp, which is what sets the cone. It is
                /// 7.50 m in every one of the nine models, single- and double-armed alike, while
                /// the patch's other axis is stretched to cover both arms (7.55 m with one arm,
                /// 9.00-11.00 m with two) - so only the across axis describes a single head.
                float pool_extent = 0.0;
                Color color = Color(1.0, 1.0, 1.0);
                bool has_pool = false;
        };
    } // namespace

    static bool has_material(const E3DSubModel *p_submodel, const char *p_name) {
        return p_submodel->get_material_name().to_lower().ends_with(p_name);
    }

    /// Depth first walk accumulating the transform, collecting both anchors of a street lamp
    static void collect_street_lamp_anchors(
            const TypedArray<E3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
            StreetLampAnchors &p_anchors) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<E3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent_transform * submodel->get_transform();
            if (has_material(submodel.ptr(), HALO_MATERIAL)) {
                // several halo billboards sit at each head; one light per distinct position
                if (!p_anchors.heads.has(transform.origin)) {
                    p_anchors.heads.push_back(transform.origin);
                }
                p_anchors.color = submodel->get_diffuse_color();
            }
            if (!p_anchors.has_pool &&
                (has_material(submodel.ptr(), POOL_MATERIAL_1) || has_material(submodel.ptr(), POOL_MATERIAL_2)) &&
                submodel->get_mesh().is_valid()) {
                p_anchors.pool_extent = static_cast<float>(submodel->get_mesh()->get_aabb().size.x * 0.5f);
                p_anchors.has_pool = p_anchors.pool_extent > 0.0f;
            }
            collect_street_lamp_anchors(submodel->get_submodels(), transform, p_anchors);
        }
    }

    const E3DModelLight *E3DModelLights::find(const String &p_name) const {
        for (const E3DModelLight &light: lights) {
            if (light.name == p_name) {
                return &light;
            }
        }
        return nullptr;
    }

    /// Collects the real lights of the model, carrying down the light of the nearest "on"
    /// ancestor and the transform relative to the model root
    static void collect_placements(
            const TypedArray<E3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
            const HashMap<E3DSubModel *, String> &p_light_owners, const String &p_parent_light_name,
            Vector<E3DModelLightPlacement> &p_placements, HashMap<E3DSubModel *, Transform3D> &p_on_transforms) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<E3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent_transform * submodel->get_transform();
            String light_name = p_parent_light_name;
            if (const HashMap<E3DSubModel *, String>::ConstIterator owner = p_light_owners.find(submodel.ptr());
                owner != p_light_owners.end()) {
                light_name = owner->value;
                p_on_transforms[submodel.ptr()] = transform;
            }
            if (submodel->get_submodel_type() == E3DSubModel::SUBMODEL_FREE_SPOTLIGHT) {
                E3DModelLightPlacement placement;
                placement.light_name = light_name;
                placement.params = E3DLightFactory::from_submodel(submodel.ptr(), light_name);
                placement.params.transform = transform;
                p_placements.push_back(placement);
            }
            collect_placements(
                    submodel->get_submodels(), transform, p_light_owners, light_name, p_placements, p_on_transforms);
        }
    }

    static Vector3 light_axis(const Transform3D &p_transform) {
        return -p_transform.basis.get_column(Vector3::AXIS_Z).normalized();
    }

    /// Collapses every group of lights that belong to one E3DModelLight into a single one.
    /// A five-armed lamp is otherwise five RenderingServer lights with five shadow maps.
    static void merge_placements(Vector<E3DModelLightPlacement> &p_placements) {
        const ProjectSettings *settings = ProjectSettings::get_singleton();
        const float height_offset = settings->get_setting(
                E3DLightFactory::ECONOMY_HEIGHT_OFFSET_SETTING, E3DLightFactory::DEFAULT_ECONOMY_HEIGHT_OFFSET);
        const float cone_scale =
                MAX(0.01f,
                    static_cast<float>(settings->get_setting(
                            E3DLightFactory::ECONOMY_CONE_SCALE_SETTING, E3DLightFactory::DEFAULT_ECONOMY_CONE_SCALE)));

        Vector<E3DModelLightPlacement> merged;
        HashMap<String, int> group_of; // light name -> index in merged
        Vector<Vector<E3DModelLightPlacement>> groups;
        for (const E3DModelLightPlacement &placement: p_placements) {
            if (!group_of.has(placement.light_name)) {
                group_of[placement.light_name] = static_cast<int>(groups.size());
                groups.push_back(Vector<E3DModelLightPlacement>());
            }
            groups.write[group_of[placement.light_name]].push_back(placement);
        }

        for (const Vector<E3DModelLightPlacement> &group: groups) {
            if (group.size() == 1) {
                merged.push_back(group[0]);
                continue;
            }
            Vector3 origin;
            Vector3 axis;
            for (const E3DModelLightPlacement &placement: group) {
                origin += placement.params.transform.origin;
                axis += light_axis(placement.params.transform);
            }
            origin /= static_cast<real_t>(group.size());
            origin.y += height_offset;
            axis = axis.normalized();
            if (axis.is_zero_approx()) {
                axis = Vector3(0, -1, 0);
            }

            E3DModelLightPlacement result = group[0];
            // wide enough to cover every cone it replaces: the angle to each original axis
            // plus that cone's own half angle
            float angle = 0.0;
            float range = 0.0;
            float energy = 0.0;
            float size = 0.0;
            bool omni = false;
            for (const E3DModelLightPlacement &placement: group) {
                const float spread =
                        static_cast<float>(Math::rad_to_deg(axis.angle_to(light_axis(placement.params.transform))));
                const float reach = static_cast<float>(origin.distance_to(placement.params.transform.origin));
                angle = MAX(angle, spread + placement.params.spot_angle);
                range = MAX(range, placement.params.range + reach);
                energy = MAX(energy, placement.params.energy);
                // the one light stands in for a ring of heads, so it is as wide as that ring
                size = MAX(size, MAX(reach, placement.params.size));
                omni = omni || placement.params.omni;
            }
            angle *= cone_scale;
            const Vector3 up = Math::abs(axis.y) > 0.99 ? Vector3(0, 0, -1) : Vector3(0, 1, 0);
            result.params.transform = Transform3D(Basis::looking_at(axis, up), origin);
            result.params.spot_angle = MIN(angle, E3DLightFactory::MAX_SPOT_ANGLE);
            result.params.omni = omni || angle > E3DLightFactory::MAX_SPOT_ANGLE;
            result.params.range = range;
            result.params.energy = energy;
            result.params.size = size;
            merged.push_back(result);
        }
        p_placements = merged;
    }

    E3DModelLights E3DLightFactory::discover(const Ref<E3DModel> &p_model, const String &p_model_filename) {
        E3DModelLights model_lights;
        if (p_model.is_null()) {
            return model_lights;
        }

        HashMap<E3DSubModel *, String> light_owners;
        const TypedDictionary<String, E3DModelLightDefinition> lights = p_model->get_lights();
        const Array light_names = lights.keys();
        for (int i = 0; i < light_names.size(); i++) {
            const String light_name = light_names[i];
            const Ref<E3DModelLightDefinition> light_info = lights[light_name];
            if (light_info.is_null()) {
                continue;
            }
            E3DModelLight light;
            light.name = light_name;
            light.on = p_model->get_node_or_null(light_info->get_on_submodel_path()).ptr();
            light.off = p_model->get_node_or_null(light_info->get_off_submodel_path()).ptr();
            light.xon = p_model->get_node_or_null(light_info->get_xon_submodel_path()).ptr();
            model_lights.lights.push_back(light);
            // a real light belongs to the light of its nearest "on" ancestor
            if (light.on != nullptr) {
                light_owners[light.on] = light_name;
            }
        }

        HashMap<E3DSubModel *, Transform3D> on_transforms;
        collect_placements(
                p_model->get_submodels(), Transform3D(), light_owners, String(), model_lights.placements,
                on_transforms);
        const int light_mode = ProjectSettings::get_singleton()->get_setting(LIGHT_MODE_SETTING, DEFAULT_LIGHT_MODE);
        if (light_mode == SCENERY_LIGHTS_OFF) {
            model_lights.placements.clear(); // the lit submodels stay, only the real lights go
            return model_lights;
        }
        const bool economy = light_mode == SCENERY_LIGHTS_ECONOMY;
        if (!model_lights.placements.is_empty() || !is_street_lamp(p_model_filename)) {
            if (economy) {
                merge_placements(model_lights.placements);
            }
            return model_lights;
        }

        for (const E3DModelLight &light: model_lights.lights) {
            if (light.on == nullptr) {
                continue;
            }
            const HashMap<E3DSubModel *, Transform3D>::ConstIterator on_transform = on_transforms.find(light.on);
            Vector<E3DLightParams> lamps;
            make_street_lamps(
                    light.on, on_transform == on_transforms.end() ? Transform3D() : on_transform->value, lamps);
            for (const E3DLightParams &params: lamps) {
                E3DModelLightPlacement placement;
                placement.light_name = light.name;
                placement.synthesized = true;
                placement.params = params;
                model_lights.placements.push_back(placement);
            }
        }
        if (economy) {
            merge_placements(model_lights.placements);
        }
        return model_lights;
    }

    E3DLightParams E3DLightFactory::from_submodel(E3DSubModel *p_submodel, const String &p_light_name) {
        E3DLightParams params;
        const bool is_end_light = p_light_name.begins_with("endsignal") || p_light_name.begins_with("endtab");

        params.energy = DEFAULT_LIGHT_ENERGY;
        if (p_light_name.begins_with("headlamp")) {
            params.energy = DEFAULT_HEAD_LIGHT_ENERGY;
        } else if (p_light_name.begins_with("highbeam")) {
            params.energy = DEFAULT_HIGHBEAM_LIGHT_ENERGY;
        } else if (is_end_light) {
            params.energy = DEFAULT_END_LIGHT_ENERGY;
        } else if (p_submodel->get_light_energy() > 0.0) {
            params.energy = p_submodel->get_light_energy();
        }

        params.range = DEFAULT_LIGHT_SPOT_RANGE;
        if (is_end_light) {
            params.range = FORCED_END_LIGHT_SPOT_RANGE;
        } else if (p_submodel->get_light_range() > 0.0) {
            params.range = p_submodel->get_light_range();
        }

        // iFarAttenDecay: 0 no decay, 1 and 2 are powers of 1/R (Model3d.h:120)
        params.attenuation = 1.0;
        switch (p_submodel->get_far_attenuation_decay()) {
            case 0:
                params.attenuation = 0.0;
                break;
            case 2:
                params.attenuation = 2.0;
                break;
            default:
                break;
        }

        const float inner_angle = Math::rad_to_deg(Math::acos(CLAMP(p_submodel->get_cos_hotspot_angle(), -1.0f, 1.0f)));
        const float outer_angle = MAX(p_submodel->get_light_angle(), 0.001f);
        const float penumbra_ratio = CLAMP((outer_angle - inner_angle) / outer_angle, 0.0f, 1.0f);

        params.omni = outer_angle > MAX_SPOT_ANGLE;
        params.spot_angle = MIN(outer_angle, MAX_SPOT_ANGLE);
        params.spot_attenuation = Math::lerp(2.0f, 0.0f, penumbra_ratio);
        params.size = ProjectSettings::get_singleton()->get_setting(LIGHT_SIZE_SETTING, DEFAULT_LIGHT_SIZE);
        params.transform = p_submodel->get_transform();
        params.color = p_submodel->get_diffuse_color();
        params.color.a = 1.0;
        return params;
    }

    bool E3DLightFactory::is_street_lamp(const String &p_model_filename) {
        return p_model_filename.get_file().to_lower().begins_with(STREET_LAMP_PREFIX);
    }

    void E3DLightFactory::make_street_lamps(
            E3DSubModel *p_on_submodel, const Transform3D &p_on_transform, Vector<E3DLightParams> &p_lamps) {
        if (p_on_submodel == nullptr) {
            return;
        }

        StreetLampAnchors anchors;
        collect_street_lamp_anchors(p_on_submodel->get_submodels(), p_on_transform, anchors);
        if (anchors.heads.is_empty() || !anchors.has_pool) {
            return;
        }

        const ProjectSettings *settings = ProjectSettings::get_singleton();
        const float height_offset =
                settings->get_setting(LAMP_LIGHT_HEIGHT_OFFSET_SETTING, DEFAULT_LAMP_LIGHT_HEIGHT_OFFSET);
        const float cone_scale = settings->get_setting(LAMP_LIGHT_CONE_SCALE_SETTING, DEFAULT_LAMP_LIGHT_CONE_SCALE);
        // floored: the easing-curve editor reads as "output over input" and invites values near
        // zero, which here would be a light that does not fall off at all (see FINDINGS.md)
        const float attenuation =
                MAX(0.0f, static_cast<float>(settings->get_setting(
                                  LAMP_LIGHT_ATTENUATION_SETTING, DEFAULT_LAMP_LIGHT_ATTENUATION)));

        for (const Vector3 &head: anchors.heads) {
            if (head.y <= 0.0f) {
                continue; // a head at or below the ground gives no cone to compute
            }
            const Vector3 origin = head + Vector3(0.0, height_offset, 0.0);
            E3DLightParams params;
            params.omni = false;
            // pointing straight down: Godot's spot shines along -Z
            params.transform = Transform3D(Basis(Vector3(1, 0, 0), Math::PI * -0.5), origin);
            params.color = anchors.color;
            params.color.a = 1.0;
            params.energy = DEFAULT_LIGHT_ENERGY;
            // The patch says how wide the cone is, but not where the light ends - it marks where
            // the light is still meant to be *visible*. The street lamps in this data set that do
            // declare a spotlight put that at 40 m (elektryczne/lampa_parkowa01, mounted at 4.9 m)
            // or 80 m (linia053/lamp-y, lamp-5, lamp-i), so the wrapper's own default for a
            // spotlight with no declared range fits; the patch edge itself would leave the whole
            // pool in the dimmest part of the falloff.
            params.range = DEFAULT_LIGHT_SPOT_RANGE;
            // from the raised origin, so the cone still covers the patch the model draws
            params.spot_angle =
                    MIN(Math::rad_to_deg(Math::atan2(anchors.pool_extent * cone_scale, static_cast<float>(origin.y))),
                        MAX_SPOT_ANGLE);
            params.spot_attenuation = 1.0;
            params.attenuation = attenuation;
            params.size = settings->get_setting(LIGHT_SIZE_SETTING, DEFAULT_LIGHT_SIZE);
            p_lamps.push_back(params);
        }
    }
} // namespace godot
