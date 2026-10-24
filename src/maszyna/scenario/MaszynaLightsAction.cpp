#include "MaszynaLightsAction.hpp"
#include "maszyna/signalling/MaszynaSignallingImplementation.hpp"
#include "signalling/SignallingServer.hpp"

namespace godot {
    void MaszynaLightsAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_signal_heads", "signal_heads"), &MaszynaLightsAction::set_signal_heads);
        ClassDB::bind_method(D_METHOD("get_signal_heads"), &MaszynaLightsAction::get_signal_heads);
        ClassDB::bind_method(D_METHOD("set_aspects", "aspects"), &MaszynaLightsAction::set_aspects);
        ClassDB::bind_method(D_METHOD("get_aspects"), &MaszynaLightsAction::get_aspects);

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
} // namespace godot
