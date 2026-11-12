#include "MaszynaLightsAction.hpp"
#include "maszyna/e3d/MaszynaE3DRenderingServer.hpp"
#include "maszyna/e3d/MaszynaLightMode.hpp"
#include "maszyna/signalling/MaszynaSignallingImplementation.hpp"
#include "signalling/SignallingServer.hpp"

namespace godot {
    void MaszynaLightsAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_signal_heads", "signal_heads"), &MaszynaLightsAction::set_signal_heads);
        ClassDB::bind_method(D_METHOD("get_signal_heads"), &MaszynaLightsAction::get_signal_heads);
        ClassDB::bind_method(D_METHOD("set_aspects", "aspects"), &MaszynaLightsAction::set_aspects);
        ClassDB::bind_method(D_METHOD("get_aspects"), &MaszynaLightsAction::get_aspects);
        ClassDB::bind_method(D_METHOD("read_lights", "lights"), &MaszynaLightsAction::read_lights);
        ClassDB::bind_method(D_METHOD("get_daylight_lights"), &MaszynaLightsAction::get_daylight_lights);
        ClassDB::bind_method(D_METHOD("get_daylight_modes"), &MaszynaLightsAction::get_daylight_modes);
        ClassDB::bind_method(D_METHOD("get_daylight_thresholds"), &MaszynaLightsAction::get_daylight_thresholds);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "signal_heads", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_signal_heads",
                "get_signal_heads");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "aspects", PROPERTY_HINT_ARRAY_TYPE, "StringName"), "set_aspects",
                "get_aspects");
    }

    void MaszynaLightsAction::run(const RID &p_event, const RID &p_activator) {
        ERR_FAIL_COND_MSG(!(signal_heads.size() == aspects.size()), "Every signal head needs its aspect.");
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < signal_heads.size(); i++) {
            const RID signal_head = signal_heads[i];
            Dictionary arguments;
            arguments["signal_head"] = signal_head;
            arguments["aspect"] = aspects[i];
            server->system_send_event(
                    server->signal_head_get_system(signal_head), MaszynaSignallingImplementation::LIGHTS_EVENT,
                    arguments);
        }
        MaszynaE3DRenderingServer *models = MaszynaE3DRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        for (int i = 0; i < signal_heads.size(); i++) {
            const RID instance = models->signal_head_get_instance(signal_heads[i]);
            if (!instance.is_valid()) {
                continue;
            }
            for (int light = 0; light < daylight_lights.size(); light++) {
                models->instance_set_light_mode(
                        instance, daylight_lights[light],
                        static_cast<MaszynaE3DRenderingServer::LightMode>(daylight_modes[light]),
                        daylight_thresholds[light]);
            }
        }
    }

    void MaszynaLightsAction::set_signal_heads(const TypedArray<RID> &p_signal_heads) {
        signal_heads = p_signal_heads;
    }

    TypedArray<RID> MaszynaLightsAction::get_signal_heads() const {
        return signal_heads;
    }

    void MaszynaLightsAction::set_aspects(const TypedArray<StringName> &p_aspects) {
        aspects = p_aspects;
    }

    TypedArray<StringName> MaszynaLightsAction::get_aspects() const {
        return aspects;
    }

    void MaszynaLightsAction::read_lights(const PackedFloat32Array &p_lights) {
        daylight_lights.clear();
        daylight_modes.clear();
        daylight_thresholds.clear();
        for (int light = 0; light < p_lights.size(); light++) {
            const MaszynaLightMode parsed = MaszynaLightMode::parse(p_lights[light]);
            if (parsed.mode == MaszynaLightMode::MODE_DARK) {
                daylight_modes.push_back(MaszynaE3DRenderingServer::LIGHT_MODE_DARK);
            } else if (parsed.mode == MaszynaLightMode::MODE_HOME) {
                daylight_modes.push_back(MaszynaE3DRenderingServer::LIGHT_MODE_HOME);
            } else {
                continue;
            }
            daylight_lights.push_back(light);
            daylight_thresholds.push_back(parsed.threshold);
        }
    }

    PackedInt32Array MaszynaLightsAction::get_daylight_lights() const {
        return daylight_lights;
    }

    PackedInt32Array MaszynaLightsAction::get_daylight_modes() const {
        return daylight_modes;
    }

    PackedFloat32Array MaszynaLightsAction::get_daylight_thresholds() const {
        return daylight_thresholds;
    }
} // namespace godot
