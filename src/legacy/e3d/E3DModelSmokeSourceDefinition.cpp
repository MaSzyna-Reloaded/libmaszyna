#include "E3DModelSmokeSourceDefinition.hpp"

namespace godot {
    void E3DModelSmokeSourceDefinition::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_template_name", "p_name"), &E3DModelSmokeSourceDefinition::set_template_name);
        ClassDB::bind_method(D_METHOD("get_template_name"), &E3DModelSmokeSourceDefinition::get_template_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "template_name"), "set_template_name", "get_template_name");

        ClassDB::bind_method(
                D_METHOD("set_submodel_path", "p_path"), &E3DModelSmokeSourceDefinition::set_submodel_path);
        ClassDB::bind_method(D_METHOD("get_submodel_path"), &E3DModelSmokeSourceDefinition::get_submodel_path);
        ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "submodel_path"), "set_submodel_path", "get_submodel_path");
    }

    String E3DModelSmokeSourceDefinition::get_template_name() const {
        return template_name;
    }

    void E3DModelSmokeSourceDefinition::set_template_name(const String &p_name) {
        template_name = p_name;
    }

    NodePath E3DModelSmokeSourceDefinition::get_submodel_path() const {
        return submodel_path;
    }

    void E3DModelSmokeSourceDefinition::set_submodel_path(const NodePath &p_path) {
        submodel_path = p_path;
    }
} // namespace godot
