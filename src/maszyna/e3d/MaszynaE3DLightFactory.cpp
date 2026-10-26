#include "MaszynaE3DLightFactory.hpp"
#include "MaszynaE3DSubModelLookup.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <array>

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

    static bool has_material(const MaszynaE3DSubModel *p_submodel, const char *p_name) {
        return p_submodel->get_material_name().to_lower().ends_with(p_name);
    }

    /// Depth first walk accumulating the transform, collecting both anchors of a street lamp
    static void collect_street_lamp_anchors(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
            StreetLampAnchors &p_anchors) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[i];
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

    const MaszynaE3DModelLight *MaszynaE3DModelLights::find(const String &p_name) const {
        for (const MaszynaE3DModelLight &light: lights) {
            if (light.name == p_name) {
                return &light;
            }
        }
        return nullptr;
    }

    /// Collects the real lights of the model, carrying down the light of the nearest "on"
    /// ancestor and the transform relative to the model root
    static void collect_placements(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
            const HashMap<MaszynaE3DSubModel *, String> &p_light_owners, const String &p_parent_light_name,
            Vector<MaszynaE3DModelLightPlacement> &p_placements,
            HashMap<MaszynaE3DSubModel *, Transform3D> &p_on_transforms) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent_transform * submodel->get_transform();
            String light_name = p_parent_light_name;
            if (const HashMap<MaszynaE3DSubModel *, String>::ConstIterator owner = p_light_owners.find(submodel.ptr());
                owner != p_light_owners.end()) {
                light_name = owner->value;
                p_on_transforms[submodel.ptr()] = transform;
            }
            if (submodel->get_submodel_type() == MaszynaE3DSubModel::SUBMODEL_FREE_SPOTLIGHT) {
                MaszynaE3DModelLightPlacement placement;
                placement.light_name = light_name;
                placement.params = MaszynaE3DLightFactory::from_submodel(submodel.ptr(), light_name);
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

    /// Collapses every group of lights that belong to one MaszynaE3DModelLight into a single one.
    /// A five-armed lamp is otherwise five RenderingServer lights with five shadow maps.
    static void merge_placements(Vector<MaszynaE3DModelLightPlacement> &p_placements) {
        const ProjectSettings *settings = ProjectSettings::get_singleton();
        const float height_offset = settings->get_setting(
                MaszynaE3DLightFactory::ECONOMY_HEIGHT_OFFSET_SETTING,
                MaszynaE3DLightFactory::DEFAULT_ECONOMY_HEIGHT_OFFSET);
        const float cone_scale =
                MAX(0.01f, static_cast<float>(settings->get_setting(
                                   MaszynaE3DLightFactory::ECONOMY_CONE_SCALE_SETTING,
                                   MaszynaE3DLightFactory::DEFAULT_ECONOMY_CONE_SCALE)));

        Vector<MaszynaE3DModelLightPlacement> merged;
        HashMap<String, int> group_of; // light name -> index in merged
        Vector<Vector<MaszynaE3DModelLightPlacement>> groups;
        for (const MaszynaE3DModelLightPlacement &placement: p_placements) {
            if (!group_of.has(placement.light_name)) {
                group_of[placement.light_name] = static_cast<int>(groups.size());
                groups.push_back(Vector<MaszynaE3DModelLightPlacement>());
            }
            groups.write[group_of[placement.light_name]].push_back(placement);
        }

        for (const Vector<MaszynaE3DModelLightPlacement> &group: groups) {
            if (group.size() == 1) {
                merged.push_back(group[0]);
                continue;
            }
            Vector3 origin;
            Vector3 axis;
            for (const MaszynaE3DModelLightPlacement &placement: group) {
                origin += placement.params.transform.origin;
                axis += light_axis(placement.params.transform);
            }
            origin /= static_cast<real_t>(group.size());
            origin.y += height_offset;
            axis = axis.normalized();
            if (axis.is_zero_approx()) {
                axis = Vector3(0, -1, 0);
            }

            MaszynaE3DModelLightPlacement result = group[0];
            // wide enough to cover every cone it replaces: the angle to each original axis
            // plus that cone's own half angle
            float angle = 0.0;
            float range = 0.0;
            float energy = 0.0;
            float size = 0.0;
            bool omni = false;
            for (const MaszynaE3DModelLightPlacement &placement: group) {
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
            result.params.spot_angle = MIN(angle, MaszynaE3DLightFactory::MAX_SPOT_ANGLE);
            result.params.omni = omni || angle > MaszynaE3DLightFactory::MAX_SPOT_ANGLE;
            result.params.range = range;
            result.params.energy = energy;
            result.params.size = size;
            merged.push_back(result);
        }
        p_placements = merged;
    }

    MaszynaE3DModelLights
    MaszynaE3DLightFactory::discover(const Ref<MaszynaE3DModel> &p_model, const String &p_model_filename) {
        MaszynaE3DModelLights model_lights;
        if (p_model.is_null()) {
            return model_lights;
        }

        HashMap<MaszynaE3DSubModel *, String> light_owners;
        const TypedDictionary<String, MaszynaE3DModelLightDefinition> lights = p_model->get_lights();
        const Array light_names = lights.keys();
        for (int i = 0; i < light_names.size(); i++) {
            const String light_name = light_names[i];
            const Ref<MaszynaE3DModelLightDefinition> light_info = lights[light_name];
            if (light_info.is_null()) {
                continue;
            }
            MaszynaE3DModelLight light;
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

        HashMap<MaszynaE3DSubModel *, Transform3D> on_transforms;
        collect_placements(
                p_model->get_submodels(), Transform3D(), light_owners, String(), model_lights.placements,
                on_transforms);
        // a passenger interior's own lights, whatever the scenery's lights are set to
        make_interior_lights(p_model, model_lights.placements);
        if (!model_lights.placements.is_empty() && model_lights.placements[0].params.role == LIGHT_ROLE_INTERIOR) {
            return model_lights;
        }
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

        for (const MaszynaE3DModelLight &light: model_lights.lights) {
            if (light.on == nullptr) {
                continue;
            }
            const HashMap<MaszynaE3DSubModel *, Transform3D>::ConstIterator on_transform = on_transforms.find(light.on);
            Vector<MaszynaE3DLightParams> lamps;
            make_street_lamps(
                    light.on, on_transform == on_transforms.end() ? Transform3D() : on_transform->value, lamps);
            for (const MaszynaE3DLightParams &params: lamps) {
                MaszynaE3DModelLightPlacement placement;
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

    MaszynaE3DLightParams
    MaszynaE3DLightFactory::from_submodel(MaszynaE3DSubModel *p_submodel, const String &p_light_name) {
        MaszynaE3DLightParams params;
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

    namespace {
        /* The passenger sections' names, by prefix, numbered from 0 or 1 (DynObj.cpp:2448-2453, 2570-2575) */
        constexpr std::array<const char *, 4> SECTION_PREFIXES = {"corridor", "korytarz", "compartment", "przedzial"};
        constexpr std::array<const char *, 4> SECTION_FIRST_NUMBERS = {"0", "00", "1", "01"};
        constexpr std::array<float, 2> INTERIOR_LIGHT_SPOTS = {0.25F, 0.75F};
        constexpr float INTERIOR_LIGHT_RANGE_SHARE = 0.5F;
        /// How far below the interior's top the lights hang [m]
        constexpr float INTERIOR_LIGHT_CEILING_OFFSET = 0.3F;
        /// The radius of the source [m]: a ceiling lamp's diffuser, so the shadows inside are soft
        constexpr float INTERIOR_LIGHT_SIZE = 0.5F;
        constexpr Color INTERIOR_LIGHT_COLOR(0.9F, 0.9F * 216.0F / 255.0F, 0.9F * 176.0F / 255.0F);
    } // namespace

    /// The bounds of every mesh under p_submodels, in the model's space
    static void merge_mesh_bounds(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const Transform3D &p_parent, AABB &p_bounds,
            bool &p_found) {
        for (int64_t index = 0; index < p_submodels.size(); index++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent * submodel->get_transform();
            if (submodel->get_mesh().is_valid()) {
                const AABB bounds = transform.xform(submodel->get_mesh()->get_aabb());
                p_bounds = p_found ? p_bounds.merge(bounds) : bounds;
                p_found = true;
            }
            merge_mesh_bounds(submodel->get_submodels(), transform, p_bounds, p_found);
        }
    }

    void MaszynaE3DLightFactory::make_interior_lights(
            const Ref<MaszynaE3DModel> &p_model, Vector<MaszynaE3DModelLightPlacement> &p_placements) {
        bool passenger_interior = false;
        for (const char *prefix: SECTION_PREFIXES) {
            for (const char *number: SECTION_FIRST_NUMBERS) {
                passenger_interior = passenger_interior ||
                                     find_submodel(p_model->get_submodels(), String(prefix) + number) != nullptr;
            }
        }
        AABB bounds;
        bool found = false;
        merge_mesh_bounds(p_model->get_submodels(), Transform3D(), bounds, found);
        if (!passenger_interior || !found) {
            return;
        }
        for (const float spot: INTERIOR_LIGHT_SPOTS) {
            MaszynaE3DModelLightPlacement placement;
            placement.light_name = INTERIOR_LIGHT_NAME;
            placement.synthesized = true;
            placement.params.omni = true;
            placement.params.role = LIGHT_ROLE_INTERIOR;
            placement.params.transform.origin =
                    Vector3(bounds.get_center().x, bounds.get_end().y - INTERIOR_LIGHT_CEILING_OFFSET,
                            bounds.position.z + (bounds.size.z * spot));
            placement.params.color = INTERIOR_LIGHT_COLOR;
            // its energy is the setting's, taken as the light is built (MaszynaE3DRenderingServer::_light_build())
            placement.params.energy = 1.0F;
            placement.params.range = static_cast<float>(bounds.size.z) * INTERIOR_LIGHT_RANGE_SHARE;
            placement.params.size = INTERIOR_LIGHT_SIZE;
            p_placements.push_back(placement);
            // the glow, at the windows' height; its energy, range and falloff are the settings'
            placement.params.role = LIGHT_ROLE_INTERIOR_GLOW;
            placement.params.transform.origin.y = bounds.get_center().y;
            placement.params.size = 0.0F;
            p_placements.push_back(placement);
        }
    }

    bool MaszynaE3DLightFactory::is_street_lamp(const String &p_model_filename) {
        return p_model_filename.get_file().to_lower().begins_with(STREET_LAMP_PREFIX);
    }

    void MaszynaE3DLightFactory::make_street_lamps(
            MaszynaE3DSubModel *p_on_submodel, const Transform3D &p_on_transform,
            Vector<MaszynaE3DLightParams> &p_lamps) {
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
            MaszynaE3DLightParams params;
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
