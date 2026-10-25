#pragma once
#include "MaszynaE3DInstanceBackend.hpp"

namespace godot {
    /// Renders submodels as RenderingServer instances, without creating any nodes.
    class MaszynaE3DOptimizedBackend : public MaszynaE3DInstanceBackend {
        private:
            /// The submodels the lights state shows or hides, for the instance being updated - kept
            /// across updates, so filling it again allocates nothing
            HashMap<MaszynaE3DSubModel *, bool> light_overrides;

            void _fill_light_overrides(const MaszynaE3DInstanceData &p_instance);
            /// Shows each RID as its chain and the lights say, setting only those that change
            void _apply_visibility(MaszynaE3DInstanceData &p_instance);
            void _add_submodels(
                    MaszynaE3DInstanceData &p_instance, const TypedArray<MaszynaE3DSubModel> &p_submodels,
                    const Transform3D &p_parent_transform, const Vector<MaszynaE3DSubModel *> &p_parent_chain,
                    const Vector<MaszynaE3DSubModel *> &p_force_alpha_submodels, int p_parent_translucency,
                    MaszynaE3DMaterialResolver &p_material_resolver);
            RID _add_submodel(
                    MaszynaE3DInstanceData &p_instance, MaszynaE3DSubModel *p_submodel, const RID &p_mesh,
                    const Ref<Material> &p_material, const Transform3D &p_local_transform,
                    const Vector<MaszynaE3DSubModel *> &p_chain);

        public:
            void build(MaszynaE3DInstanceData &p_instance, MaszynaE3DMaterialResolver &p_material_resolver) override;
            void clear(MaszynaE3DInstanceData &p_instance) override;
            void update(MaszynaE3DInstanceData &p_instance) override;
            void update_visibility(MaszynaE3DInstanceData &p_instance) override;
            void apply_transform(const MaszynaE3DInstanceData &p_instance) override;
            void apply_poses(MaszynaE3DInstanceData &p_instance) override;
            bool intersect_segment(
                    const MaszynaE3DInstanceData &p_instance, const Vector3 &p_from, const Vector3 &p_to,
                    double &p_r_distance, Vector3 &p_r_point) const override;
    };
} // namespace godot
