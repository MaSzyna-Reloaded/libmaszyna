#pragma once
#include "MaszynaE3DInstanceBackend.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>

namespace godot {
    /// Builds submodels as a Node3D/MeshInstance3D/SpotLight3D tree under the attached node.
    /// (formerly e3d_nodes_instancer.gd)
    class MaszynaE3DNodesBackend : public MaszynaE3DInstanceBackend {
        private:
            bool editable = false; // generated nodes stay editable in the editor (not internal)

            struct LightRole {
                    String light_name;
                    LightPart part = LIGHT_PART_ON;
            };

            void _add_submodels(
                    MaszynaE3DInstanceData &p_instance, Node3D *p_target, Node3D *p_parent,
                    const TypedArray<MaszynaE3DSubModel> &p_submodels, const Transform3D &p_parent_transform,
                    const HashMap<MaszynaE3DSubModel *, LightRole> &p_light_roles, const String &p_parent_light_name,
                    const Vector<MaszynaE3DSubModel *> &p_force_alpha_submodels, int p_parent_translucency,
                    MaszynaE3DMaterialResolver &p_material_resolver);
            static Node3D *
            _create_submodel_node(const MaszynaE3DInstanceData &p_instance, MaszynaE3DSubModel *p_submodel);
            static void
            _configure_spotlight(SpotLight3D *p_spotlight, const String &p_light_name, MaszynaE3DSubModel *p_submodel);
            static void _set_node_visible(const ObjectID &p_node_id, bool p_visible);

        public:
            explicit MaszynaE3DNodesBackend(bool p_editable);

            void build(MaszynaE3DInstanceData &p_instance, MaszynaE3DMaterialResolver &p_material_resolver) override;
            void clear(MaszynaE3DInstanceData &p_instance) override;
            void update(MaszynaE3DInstanceData &p_instance) override;
            void apply_poses(MaszynaE3DInstanceData &p_instance) override;
            bool intersect_segment(
                    const MaszynaE3DInstanceData &p_instance, const Vector3 &p_from, const Vector3 &p_to,
                    double &p_r_distance, Vector3 &p_r_point) const override;
            /// The generated node tree hangs under the attached node and moves with it.
            void apply_transform(const MaszynaE3DInstanceData &p_instance) override {}
    };
} // namespace godot
