#pragma once
#include "maszyna/e3d/MaszynaE3DModel.hpp"
#include "maszyna/e3d/MaszynaE3DModelGeometry.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {
    /* A model file (.e3d, .t3d) read by its extension's parser - its description and its geometry
     * apart: what a vehicle's data needs of its model (the submodels its MMD names) is its
     * description alone, and only a model drawn has meshes in the GPU. A pure helper; the models are
     * kept by MaszynaE3DModelManager. */
    class MaszynaE3DModelReader : public Object {
            GDCLASS(MaszynaE3DModelReader, Object)

        protected:
            static void _bind_methods();

        public:
            /* The model to draw: its description with its meshes; null when it cannot be read */
            static Ref<MaszynaE3DModel> read_model(const String &p_path);
            /* Its description alone, without its geometry - no mesh, nothing in the GPU */
            static Ref<MaszynaE3DModel> read_description(const String &p_path);
            /* Its geometry alone, to be applied to its description (MaszynaE3DModel::apply_geometry()) */
            static Ref<MaszynaE3DModelGeometry> read_geometry(const String &p_path);
            /* The vertices of the submodel of that geometry index (MaszynaE3DSubModel::geometry_index),
             * in its own frame */
            static PackedVector3Array read_submodel_positions(const String &p_path, int p_index);
    };
} // namespace godot
