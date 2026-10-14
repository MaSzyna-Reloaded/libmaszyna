#include "MaszynaLegacyAnimationAction.hpp"
#include "legacy/e3d/E3DRenderingServer.hpp"
#include "macros.hpp"

namespace godot {
    void MaszynaLegacyAnimationAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_instances", "instances"), &MaszynaLegacyAnimationAction::set_instances);
        ClassDB::bind_method(D_METHOD("get_instances"), &MaszynaLegacyAnimationAction::get_instances);
        ClassDB::bind_method(D_METHOD("set_submodel", "submodel"), &MaszynaLegacyAnimationAction::set_submodel);
        ClassDB::bind_method(D_METHOD("get_submodel"), &MaszynaLegacyAnimationAction::get_submodel);
        ClassDB::bind_method(D_METHOD("set_mode", "mode"), &MaszynaLegacyAnimationAction::set_mode);
        ClassDB::bind_method(D_METHOD("get_mode"), &MaszynaLegacyAnimationAction::get_mode);
        ClassDB::bind_method(D_METHOD("set_target", "target"), &MaszynaLegacyAnimationAction::set_target);
        ClassDB::bind_method(D_METHOD("get_target"), &MaszynaLegacyAnimationAction::get_target);
        ClassDB::bind_method(D_METHOD("set_speed", "speed"), &MaszynaLegacyAnimationAction::set_speed);
        ClassDB::bind_method(D_METHOD("get_speed"), &MaszynaLegacyAnimationAction::get_speed);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "instances", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_instances",
                "get_instances");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "submodel"), "set_submodel", "get_submodel");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "mode", PROPERTY_HINT_ENUM,
                        enum_hint({{"Rotate", MODE_ROTATE}, {"Translate", MODE_TRANSLATE}})),
                "set_mode", "get_mode");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "target"), "set_target", "get_target");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed"), "set_speed", "get_speed");

        BIND_ENUM_CONSTANT(MODE_ROTATE);
        BIND_ENUM_CONSTANT(MODE_TRANSLATE);
    }

    void MaszynaLegacyAnimationAction::run(const RID &p_event, const RID &p_activator) {
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        for (int i = 0; i < instances.size(); i++) {
            if (mode == MODE_ROTATE) {
                rendering->instance_set_submodel_rotation(instances[i], submodel, target, speed);
            } else {
                rendering->instance_set_submodel_translation(instances[i], submodel, target, speed);
            }
        }
    }

    void MaszynaLegacyAnimationAction::set_instances(const TypedArray<RID> &p_instances) {
        instances = p_instances;
    }

    TypedArray<RID> MaszynaLegacyAnimationAction::get_instances() const {
        return instances;
    }

    void MaszynaLegacyAnimationAction::set_submodel(const String &p_submodel) {
        submodel = p_submodel;
    }

    String MaszynaLegacyAnimationAction::get_submodel() const {
        return submodel;
    }

    void MaszynaLegacyAnimationAction::set_mode(const Mode p_mode) {
        mode = p_mode;
    }

    MaszynaLegacyAnimationAction::Mode MaszynaLegacyAnimationAction::get_mode() const {
        return mode;
    }

    void MaszynaLegacyAnimationAction::set_target(const Vector3 &p_target) {
        target = p_target;
    }

    Vector3 MaszynaLegacyAnimationAction::get_target() const {
        return target;
    }

    void MaszynaLegacyAnimationAction::set_speed(const double p_speed) {
        speed = p_speed;
    }

    double MaszynaLegacyAnimationAction::get_speed() const {
        return speed;
    }
} // namespace godot
