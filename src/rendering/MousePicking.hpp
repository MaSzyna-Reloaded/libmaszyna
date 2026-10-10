#pragma once
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// What the mouse pickers (CabinHUDMouseSystem, SceneryHUDMouseServer) share: the outline of
    /// the thing under the cursor and the ray test against a mesh's triangles
    namespace mouse_picking {
        constexpr Color OUTLINE_COLOR = Color(1.0, 0.85, 0.3);

        /* Godot's stencil outline (BaseMaterial3D::STENCIL_MODE_OUTLINE) is meant for the mesh's
         * own material: it writes the stencil and a grown next pass draws only outside it. The
         * materials are shared between meshes, so the pair goes into the overlay instead - the
         * overlay writes the stencil and, with `p_fill_alpha`, tints the mesh. Both are
         * transparent, so the preset's render priorities (writer 0, outline 1) keep them in order,
         * and neither tests depth: the whole silhouette gets its ring, also where the mesh sinks
         * into whatever it stands in. `p_width` is how far the ring reaches out, in metres. */
        Ref<StandardMaterial3D> outline_material(double p_width, float p_fill_alpha);

        /// The nearest hit of the segment `p_from`..`p_to` on the triangles `p_faces`, all in one
        /// space: `r_t` is the hit's fraction of the segment, [0, 1]. False when it misses.
        bool
        intersect_faces(const PackedVector3Array &p_faces, const Vector3 &p_from, const Vector3 &p_to, double &p_r_t);
    } // namespace mouse_picking
} // namespace godot
