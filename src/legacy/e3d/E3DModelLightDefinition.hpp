#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/wrapped.hpp>

namespace godot {
    class E3DModelLightDefinition : public Resource {
            GDCLASS(E3DModelLightDefinition, Resource)

        private:
            NodePath on_submodel_path;
            NodePath off_submodel_path;
            /// "<name>_xon": what the light shows dimmed (TButton, TurnxOnWithOnAsFallback)
            NodePath xon_submodel_path;

        protected:
            static void _bind_methods();

        public:
            NodePath get_on_submodel_path() const;
            void set_on_submodel_path(const NodePath &p_path);

            NodePath get_off_submodel_path() const;
            void set_off_submodel_path(const NodePath &p_path);

            NodePath get_xon_submodel_path() const;
            void set_xon_submodel_path(const NodePath &p_path);
    };
} // namespace godot
