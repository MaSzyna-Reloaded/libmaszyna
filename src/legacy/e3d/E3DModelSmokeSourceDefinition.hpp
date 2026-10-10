#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/wrapped.hpp>

namespace godot {
    /// One particle emitter the model declares: a transform submodel named "smokesource_<template>"
    /// (TSubModel::is_emitter(), Model3d.cpp:1417). The name is also the name of the parameter file
    /// the original reads from data/ (particle_manager::find(), particles.cpp:465).
    class E3DModelSmokeSourceDefinition : public Resource {
            GDCLASS(E3DModelSmokeSourceDefinition, Resource)

        private:
            String template_name;
            NodePath submodel_path;

        protected:
            static void _bind_methods();

        public:
            String get_template_name() const;
            void set_template_name(const String &p_name);

            NodePath get_submodel_path() const;
            void set_submodel_path(const NodePath &p_path);
    };
} // namespace godot
