#pragma once

#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>

namespace godot {
    class Camera3D;
    class MeshInstance3D;
    class SubViewport;

    /* A planar mirror on the glass it is the child of: a camera mirrored across the glass renders
     * the scene into a texture drawn over the glass. It renders only while the glass is in view within
     * max_distance of the camera, at resolution_scale of the pixels the glass covers, and cheaply:
     * no omni/spot shadows, coarser LODs, no anti-aliasing, and the scene's environment without its
     * GI and screen-space effects, taken each time the mirror starts rendering. */
    class PlanarMirror3D : public Node3D {
            GDCLASS(PlanarMirror3D, Node3D)

        private:
            static void _bind_methods();

            double max_distance = 30.0;
            double resolution_scale = 2.0;
            MeshInstance3D *glass = nullptr;
            Ref<Material> glass_material;
            SubViewport *viewport = nullptr;
            Camera3D *camera = nullptr;
            Ref<ShaderMaterial> material;
            Ref<Environment> environment;
            // the glass's plane in the glass's own space, and how far the glass reaches from its middle
            Vector3 plane_point;
            Vector3 plane_normal;
            real_t glass_radius = 0.0;
            // no housing tells which side reflects - the side the eye looks at does
            bool facing_viewer = true;

            void _set_rendering(bool p_rendering);

        protected:
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);

        public:
            void set_max_distance(double p_value);
            double get_max_distance() const;
            void set_resolution_scale(double p_value);
            double get_resolution_scale() const;
    };
} // namespace godot
