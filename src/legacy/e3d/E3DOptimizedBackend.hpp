#pragma once
#include "E3DInstanceBackend.hpp"

namespace godot {
    /// Renders submodels as RenderingServer instances, without creating any nodes.
    class E3DOptimizedBackend : public E3DInstanceBackend {
        private:
            void _add_submodels(
                    E3DInstanceData &p_instance, const TypedArray<E3DSubModel> &p_submodels,
                    const Transform3D &p_parent_transform, const Vector<E3DSubModel *> &p_parent_chain,
                    const Vector<E3DSubModel *> &p_force_alpha_submodels, int p_parent_translucency,
                    E3DMaterialResolver &p_material_resolver);
            RID _add_submodel(
                    E3DInstanceData &p_instance, E3DSubModel *p_submodel, const RID &p_mesh,
                    const Ref<Material> &p_material, const Transform3D &p_local_transform,
                    const Vector<E3DSubModel *> &p_chain);

        public:
            void build(E3DInstanceData &p_instance, E3DMaterialResolver &p_material_resolver) override;
            void clear(E3DInstanceData &p_instance) override;
            void update(const E3DInstanceData &p_instance) override;
            void apply_transform(const E3DInstanceData &p_instance) override;
            void apply_poses(E3DInstanceData &p_instance) override;
            bool intersect_segment(
                    const E3DInstanceData &p_instance, const Vector3 &p_from, const Vector3 &p_to, double &p_r_distance,
                    Vector3 &p_r_point) const override;
    };
} // namespace godot
