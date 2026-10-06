#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// WORKAROUND for E3D cab models made for the original renderer: a lamp of several bulbs (a row
    /// of indicator dots, a gauge's backlight) is one "_on" submodel there, since the original lights
    /// a cab only with its ambient term and never needed the bulbs apart. This detector splits such
    /// a submodel's mesh back into its separate lamps, so that MmdCabinInstancer can put a light at
    /// each (MmdSemanticCatalog.IslandLights). Nothing here is the original's; it guesses the bulbs
    /// from the geometry of models that do not say where they are.
    class LegacyCabinLampIslands : public Object {
            GDCLASS(LegacyCabinLampIslands, Object)

        public:
            /// Pieces of one lamp mesh closer than this are one lamp (a gauge's overlay split at a
            /// UV seam)
            static constexpr double MERGE_DISTANCE = 0.02;
            /// No lamp gets more lights than this, whatever its mesh - the nearest pieces are
            /// merged first
            static constexpr int LIGHT_MAX_COUNT = 16;
            /// Texture samples per axis over a piece's UV bounds when its colour is taken
            static constexpr int COLOR_SAMPLES = 6;
            /// WORKAROUND for the worst of those models: a mesh of more pieces than this is a
            /// backlight of unwelded triangles, not bulbs (36WEa's dashboard light: 7237 pieces,
            /// merged pairwise into one light in 281 ms), and gets one light at all of it at once
            static constexpr int BACKLIGHT_PIECE_COUNT = 1024;

        protected:
            static void _bind_methods();

        public:
            /// The pieces of `lamp`'s mesh and of the meshes under it: triangles joined by a shared
            /// vertex are one piece, pieces nearer than MERGE_DISTANCE are one lamp, and at most
            /// LIGHT_MAX_COUNT are kept by merging the nearest; more than BACKLIGHT_PIECE_COUNT pieces
            /// are one. Each is {position (global centre),
            /// color}: the average of the piece's texture over its UV bounds, weighted by alpha and
            /// tinted by the material's albedo, brought to full brightness - the light's energy
            /// says how bright.
            static TypedArray<Dictionary> submodel_islands(Node3D *p_lamp);
    };
} // namespace godot
