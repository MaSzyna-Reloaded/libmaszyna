#include "MaszynaE3DModelLightDefinition.hpp"

namespace godot {
    void MaszynaE3DModelLightDefinition::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_on_submodel_path", "p_path"), &MaszynaE3DModelLightDefinition::set_on_submodel_path);
        ClassDB::bind_method(D_METHOD("get_on_submodel_path"), &MaszynaE3DModelLightDefinition::get_on_submodel_path);
        ADD_PROPERTY(
                PropertyInfo(Variant::NODE_PATH, "on_submodel_path"), "set_on_submodel_path", "get_on_submodel_path");

        ClassDB::bind_method(
                D_METHOD("set_off_submodel_path", "p_path"), &MaszynaE3DModelLightDefinition::set_off_submodel_path);
        ClassDB::bind_method(D_METHOD("get_off_submodel_path"), &MaszynaE3DModelLightDefinition::get_off_submodel_path);
        ADD_PROPERTY(
                PropertyInfo(Variant::NODE_PATH, "off_submodel_path"), "set_off_submodel_path",
                "get_off_submodel_path");

        ClassDB::bind_method(
                D_METHOD("set_xon_submodel_path", "p_path"), &MaszynaE3DModelLightDefinition::set_xon_submodel_path);
        ClassDB::bind_method(D_METHOD("get_xon_submodel_path"), &MaszynaE3DModelLightDefinition::get_xon_submodel_path);
        ADD_PROPERTY(
                PropertyInfo(Variant::NODE_PATH, "xon_submodel_path"), "set_xon_submodel_path",
                "get_xon_submodel_path");
    }

    NodePath MaszynaE3DModelLightDefinition::get_on_submodel_path() const {
        return on_submodel_path;
    }

    void MaszynaE3DModelLightDefinition::set_on_submodel_path(const NodePath &p_path) {
        on_submodel_path = p_path;
    }

    NodePath MaszynaE3DModelLightDefinition::get_off_submodel_path() const {
        return off_submodel_path;
    }

    void MaszynaE3DModelLightDefinition::set_off_submodel_path(const NodePath &p_path) {
        off_submodel_path = p_path;
    }

    NodePath MaszynaE3DModelLightDefinition::get_xon_submodel_path() const {
        return xon_submodel_path;
    }

    void MaszynaE3DModelLightDefinition::set_xon_submodel_path(const NodePath &p_path) {
        xon_submodel_path = p_path;
    }

} // namespace godot
