#include "E3DSmokeSourceFactory.hpp"
#include <godot_cpp/templates/hash_set.hpp>

namespace godot {
    /// Depth first walk accumulating the transform, collecting every registered emitter
    static void collect_placements(
            const TypedArray<E3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
            const HashSet<String> &p_names, Vector<E3DSmokeSourcePlacement> &p_placements) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<E3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent_transform * submodel->get_transform();
            if (submodel->get_submodel_type() == E3DSubModel::SUBMODEL_TRANSFORM) {
                const String name = submodel->get_name().to_lower();
                if (p_names.has(name)) {
                    E3DSmokeSourcePlacement placement;
                    placement.template_name = name;
                    placement.offset = transform.origin;
                    p_placements.push_back(placement);
                }
            }
            collect_placements(submodel->get_submodels(), transform, p_names, p_placements);
        }
    }

    Vector<E3DSmokeSourcePlacement> E3DSmokeSourceFactory::discover(const Ref<E3DModel> &p_model) {
        Vector<E3DSmokeSourcePlacement> placements;
        if (p_model.is_null()) {
            return placements;
        }

        const TypedArray<E3DModelSmokeSourceDefinition> definitions = p_model->get_smoke_sources();
        if (definitions.is_empty()) {
            return placements;
        }

        HashSet<String> names;
        for (int i = 0; i < definitions.size(); i++) {
            const Ref<E3DModelSmokeSourceDefinition> definition = definitions[i];
            if (definition.is_valid()) {
                names.insert(definition->get_template_name());
            }
        }

        collect_placements(p_model->get_submodels(), Transform3D(), names, placements);
        return placements;
    }
} // namespace godot
