#pragma once
#include "E3DModel.hpp"
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// One emitter of a model, already placed relative to the model root
    struct E3DSmokeSourcePlacement {
            String template_name; // also the name of the parameter file under data/
            Vector3 offset;       // relative to the model root
    };

    /// Finds where a model emits particles from. Only the origin of the emitter submodel is kept:
    /// the original does the same (TSubModel::offset(), Model3d.cpp:1583) and always launches the
    /// particles along the owner's own up axis, ignoring the bone's rotation (particles.cpp:58).
    class E3DSmokeSourceFactory {
        public:
            /// Walks the model once, matching the emitters E3DParser registered on it. Returns
            /// nothing - without walking at all - for the models that declare none.
            static Vector<E3DSmokeSourcePlacement> discover(const Ref<E3DModel> &p_model);
    };
} // namespace godot
