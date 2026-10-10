#pragma once
#include "E3DModelLightDefinition.hpp"
#include "E3DModelSmokeSourceDefinition.hpp"
#include "E3DSubModel.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>
#include <macros.hpp>

namespace godot {
    class E3DModel : public Resource {
            GDCLASS(E3DModel, Resource)
        private:
            TypedDictionary<String, E3DModelLightDefinition> lights;
            /// An array, not a dictionary: a model may carry several emitters of one template
            TypedArray<E3DModelSmokeSourceDefinition> smoke_sources;
            TypedArray<E3DSubModel> submodels;

        public:
            static constexpr int FORMAT_VERSION = 20261004; // must be incremented when public API of E3DModel or
                                                            // E3DSubModel is changing
            ~E3DModel() override;

            TypedDictionary<String, E3DModelLightDefinition> get_lights() const;
            void set_lights(const TypedDictionary<String, E3DModelLightDefinition> &p_lights);

            TypedArray<E3DModelSmokeSourceDefinition> get_smoke_sources() const;
            void set_smoke_sources(const TypedArray<E3DModelSmokeSourceDefinition> &p_smoke_sources);

            TypedArray<E3DSubModel> get_submodels() const;
            void set_submodels(const TypedArray<E3DSubModel> &p_submodels);

            void clear();
            void add_child(const Ref<E3DSubModel> &p_sub_model);
            void register_light(const String &p_name, const Ref<E3DModelLightDefinition> &p_entry);
            void register_smoke_source(const Ref<E3DModelSmokeSourceDefinition> &p_entry);
            Ref<E3DSubModel> get_node(const NodePath &p_path) const;
            Ref<E3DSubModel> get_node_or_null(const NodePath &p_path) const;

        protected:
            static void _bind_methods();

        private:
            Ref<E3DSubModel> _get_node_or_null(const PackedStringArray &p_segments) const;
            PackedStringArray _split_path(const NodePath &p_path) const;
    };
} // namespace godot
