#include "MaszynaLegacyLightsAction.hpp"
#include "legacy/signalling/MaszynaLegacySignallingImplementation.hpp"
#include "signalling/SignallingServer.hpp"

namespace godot {
    void MaszynaLegacyLightsAction::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_signal_heads", "signal_heads"), &MaszynaLegacyLightsAction::set_signal_heads);
        ClassDB::bind_method(D_METHOD("get_signal_heads"), &MaszynaLegacyLightsAction::get_signal_heads);
        ClassDB::bind_method(D_METHOD("set_aspects", "aspects"), &MaszynaLegacyLightsAction::set_aspects);
        ClassDB::bind_method(D_METHOD("get_aspects"), &MaszynaLegacyLightsAction::get_aspects);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "signal_heads", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_signal_heads",
                "get_signal_heads");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "aspects", PROPERTY_HINT_ARRAY_TYPE, "StringName"), "set_aspects",
                "get_aspects");
    }

    void MaszynaLegacyLightsAction::run(const RID &p_event, const RID &p_activator) {
        ERR_FAIL_COND_MSG(!(signal_heads.size() == aspects.size()), "Every signal head needs its aspect.");
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < signal_heads.size(); i++) {
            const RID signal_head = signal_heads[i];
            Dictionary arguments;
            arguments["signal_head"] = signal_head;
            arguments["aspect"] = aspects[i];
            server->system_send_event(
                    server->signal_head_get_system(signal_head), MaszynaLegacySignallingImplementation::LIGHTS_EVENT,
                    arguments);
        }
    }

    void MaszynaLegacyLightsAction::set_signal_heads(const TypedArray<RID> &p_signal_heads) {
        signal_heads = p_signal_heads;
    }

    TypedArray<RID> MaszynaLegacyLightsAction::get_signal_heads() const {
        return signal_heads;
    }

    void MaszynaLegacyLightsAction::set_aspects(const TypedArray<StringName> &p_aspects) {
        aspects = p_aspects;
    }

    TypedArray<StringName> MaszynaLegacyLightsAction::get_aspects() const {
        return aspects;
    }
} // namespace godot
