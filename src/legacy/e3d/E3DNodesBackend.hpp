#pragma once
#include "E3DInstanceBackend.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>

namespace godot {
    /// Builds submodels as a Node3D/MeshInstance3D/SpotLight3D tree under the attached node.
    /// (formerly e3d_nodes_instancer.gd)
    class E3DNodesBackend : public E3DInstanceBackend {
        private:
            bool editable = false; // generated nodes stay editable in the editor (not internal)

            struct LightRole {
                    String light_name;
                    LightPart part = LIGHT_PART_ON;
            };

            void _add_submodels(
                    E3DInstanceData &p_instance, Node3D *p_target, Node3D *p_parent,
                    const TypedArray<E3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
                    const HashMap<E3DSubModel *, LightRole> &p_light_roles, const String &p_parent_light_name,
                    const Vector<E3DSubModel *> &p_force_alpha_submodels, int p_parent_translucency,
                    E3DMaterialResolver &p_material_resolver);
            static Node3D *_create_submodel_node(const E3DInstanceData &p_instance, E3DSubModel *p_submodel);
            static void
            _configure_spotlight(SpotLight3D *p_spotlight, const String &p_light_name, E3DSubModel *p_submodel);
            static void _set_node_visible(const ObjectID &p_node_id, bool p_visible);

        public:
            explicit E3DNodesBackend(bool p_editable);

            void build(E3DInstanceData &p_instance, E3DMaterialResolver &p_material_resolver) override;
            void clear(E3DInstanceData &p_instance) override;
            void update(const E3DInstanceData &p_instance) override;
            void apply_poses(E3DInstanceData &p_instance) override;
            bool intersect_segment(
                    const E3DInstanceData &p_instance, const Vector3 &p_from, const Vector3 &p_to, double &p_r_distance,
                    Vector3 &p_r_point) const override;
            /// The generated node tree hangs under the attached node and moves with it.
            void apply_transform(const E3DInstanceData &p_instance) override {}
    };
} // namespace godot
