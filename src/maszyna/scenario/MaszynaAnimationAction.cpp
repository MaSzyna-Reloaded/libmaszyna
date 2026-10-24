#include "MaszynaAnimationAction.hpp"
#include "macros.hpp"
#include "maszyna/e3d/MaszynaE3DRenderingServer.hpp"

namespace godot {
    void MaszynaAnimationAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_instances", "instances"), &MaszynaAnimationAction::set_instances);
        ClassDB::bind_method(D_METHOD("get_instances"), &MaszynaAnimationAction::get_instances);
        ClassDB::bind_method(D_METHOD("set_submodel", "submodel"), &MaszynaAnimationAction::set_submodel);
        ClassDB::bind_method(D_METHOD("get_submodel"), &MaszynaAnimationAction::get_submodel);
        ClassDB::bind_method(D_METHOD("set_mode", "mode"), &MaszynaAnimationAction::set_mode);
        ClassDB::bind_method(D_METHOD("get_mode"), &MaszynaAnimationAction::get_mode);
        ClassDB::bind_method(D_METHOD("set_target", "target"), &MaszynaAnimationAction::set_target);
        ClassDB::bind_method(D_METHOD("get_target"), &MaszynaAnimationAction::get_target);
        ClassDB::bind_method(D_METHOD("set_speed", "speed"), &MaszynaAnimationAction::set_speed);
        ClassDB::bind_method(D_METHOD("get_speed"), &MaszynaAnimationAction::get_speed);

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

    void MaszynaAnimationAction::run(const RID &p_event, const RID &p_activator) {
        MaszynaE3DRenderingServer *rendering = MaszynaE3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        for (int i = 0; i < instances.size(); i++) {
            if (mode == MODE_ROTATE) {
                rendering->instance_set_submodel_rotation(instances[i], submodel, target, speed);
            } else {
                rendering->instance_set_submodel_translation(instances[i], submodel, target, speed);
            }
        }
    }

    void MaszynaAnimationAction::set_instances(const TypedArray<RID> &p_instances) {
        instances = p_instances;
    }

    TypedArray<RID> MaszynaAnimationAction::get_instances() const {
        return instances;
    }

    void MaszynaAnimationAction::set_submodel(const String &p_submodel) {
        submodel = p_submodel;
    }

    String MaszynaAnimationAction::get_submodel() const {
        return submodel;
    }

    void MaszynaAnimationAction::set_mode(const Mode p_mode) {
        mode = p_mode;
    }

    MaszynaAnimationAction::Mode MaszynaAnimationAction::get_mode() const {
        return mode;
    }

    void MaszynaAnimationAction::set_target(const Vector3 &p_target) {
        target = p_target;
    }

    Vector3 MaszynaAnimationAction::get_target() const {
        return target;
    }

    void MaszynaAnimationAction::set_speed(const double p_speed) {
        speed = p_speed;
    }

    double MaszynaAnimationAction::get_speed() const {
        return speed;
    }
} // namespace godot
