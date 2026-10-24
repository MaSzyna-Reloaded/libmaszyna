#include "MaszynaSignalHeadKindFactory.hpp"
#include "maszyna/e3d/MaszynaLightMode.hpp"

namespace godot {
    void MaszynaSignalHeadKindFactory::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaSignalHeadKindFactory", D_METHOD("create_kind", "aspects"),
                &MaszynaSignalHeadKindFactory::create_kind);
        ClassDB::bind_static_method(
                "MaszynaSignalHeadKindFactory", D_METHOD("get_aspect_name", "event_name", "signal_head_name"),
                &MaszynaSignalHeadKindFactory::get_aspect_name);
    }

    /// The values keep the original's encoding (MaszynaLightMode) and are read once, here
    Ref<SignalHeadKind> MaszynaSignalHeadKindFactory::create_kind(const Dictionary &p_aspects) {
        Ref<SignalHeadKind> kind;
        kind.instantiate();
        Dictionary aspects;
        const Array names = p_aspects.keys();
        for (int i = 0; i < names.size(); i++) {
            const PackedFloat32Array values = p_aspects[names[i]];
            PackedInt32Array lights;
            PackedFloat32Array on_times;
            PackedFloat32Array off_times;
            PackedFloat32Array phases;
            const int count = MIN(static_cast<int>(values.size()), MAX_EVENT_LIGHTS);
            for (int light = 0; light < count; light++) {
                const MaszynaLightMode mode = MaszynaLightMode::parse(values[light]);
                SignalAspect::LightCommand command = SignalAspect::LIGHT_KEEP;
                if (values[light] == LIGHT_UNCHANGED) {
                    command = SignalAspect::LIGHT_KEEP;
                } else if (mode.mode == MaszynaLightMode::MODE_OFF) {
                    command = SignalAspect::LIGHT_OFF;
                } else if (mode.mode == MaszynaLightMode::MODE_ON) {
                    command = SignalAspect::LIGHT_ON;
                } else if (mode.mode == MaszynaLightMode::MODE_BLINK) {
                    command = SignalAspect::LIGHT_BLINK;
                } else {
                    // ls_Dark/ls_Home from an event: a signal head has no light that follows the
                    // daylight yet, see TODO.md
                    WARN_PRINT_ONCE("A `lights` event sets ls_Dark/ls_Home, which is not supported yet.");
                }
                lights.push_back(command);
                on_times.push_back(mode.on_time);
                off_times.push_back(mode.off_time);
                phases.push_back(mode.phase);
            }
            Ref<SignalAspect> aspect;
            aspect.instantiate();
            aspect->set_lights(lights);
            aspect->set_on_times(on_times);
            aspect->set_off_times(off_times);
            aspect->set_phases(phases);
            aspects[StringName(names[i])] = aspect;
        }
        kind->set_aspects(aspects);
        return kind;
    }

    /// Event names are lower case in the original (Event.cpp:2202), model names keep theirs
    StringName
    MaszynaSignalHeadKindFactory::get_aspect_name(const String &p_event_name, const String &p_signal_head_name) {
        const String event_name = p_event_name.to_lower();
        const String prefix = p_signal_head_name.to_lower() + "_";
        return event_name.begins_with(prefix) ? event_name.substr(prefix.length()) : event_name;
    }
} // namespace godot
