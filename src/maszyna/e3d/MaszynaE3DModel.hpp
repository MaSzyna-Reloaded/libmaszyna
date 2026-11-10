#pragma once
#include "MaszynaE3DModelGeometry.hpp"
#include "MaszynaE3DModelLightDefinition.hpp"
#include "MaszynaE3DModelSmokeSourceDefinition.hpp"
#include "MaszynaE3DSubModel.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

namespace godot {
    class MaszynaE3DModel : public Resource {
            GDCLASS(MaszynaE3DModel, Resource)
        private:
            TypedDictionary<String, MaszynaE3DModelLightDefinition> lights;
            /// An array, not a dictionary: a model may carry several emitters of one template
            TypedArray<MaszynaE3DModelSmokeSourceDefinition> smoke_sources;
            TypedArray<MaszynaE3DSubModel> submodels;
            /* The meshes are made of the geometry once (apply_geometry()); a model is shared by
             * everyone who loads its file, on any thread */
            mutable Mutex geometry_mutex;
            bool geometry_applied = false;

        public:
            static constexpr int FORMAT_VERSION = 20261110; // must be incremented when public API of MaszynaE3DModel or
                                                            // MaszynaE3DSubModel is changing

            TypedDictionary<String, MaszynaE3DModelLightDefinition> get_lights() const;
            void set_lights(const TypedDictionary<String, MaszynaE3DModelLightDefinition> &p_lights);

            TypedArray<MaszynaE3DModelSmokeSourceDefinition> get_smoke_sources() const;
            void set_smoke_sources(const TypedArray<MaszynaE3DModelSmokeSourceDefinition> &p_smoke_sources);

            TypedArray<MaszynaE3DSubModel> get_submodels() const;
            void set_submodels(const TypedArray<MaszynaE3DSubModel> &p_submodels);

            void add_child(const Ref<MaszynaE3DSubModel> &p_sub_model);
            void register_light(const String &p_name, const Ref<MaszynaE3DModelLightDefinition> &p_entry);
            void register_smoke_source(const Ref<MaszynaE3DModelSmokeSourceDefinition> &p_entry);
            Ref<MaszynaE3DSubModel> get_node(const NodePath &p_path) const;
            Ref<MaszynaE3DSubModel> get_node_or_null(const NodePath &p_path) const;
            /* The model's description becomes a model to draw: every submodel of triangles gets its
             * mesh, made of p_geometry in the GPU, once; the geometry is not kept */
            void apply_geometry(const Ref<MaszynaE3DModelGeometry> &p_geometry);
            /* Whether its meshes were made (apply_geometry()) - a description alone has none */
            bool has_geometry() const;

        protected:
            static void _bind_methods();

        private:
            Ref<MaszynaE3DSubModel> _get_node_or_null(const PackedStringArray &p_segments) const;
            static void _apply_geometry(
                    const TypedArray<MaszynaE3DSubModel> &p_submodels, const Ref<MaszynaE3DModelGeometry> &p_geometry);
            PackedStringArray _split_path(const NodePath &p_path) const;
    };
} // namespace godot
