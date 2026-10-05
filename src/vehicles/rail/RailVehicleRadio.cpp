#include "RailVehicleRadio.hpp"

namespace godot {
    const char *RailVehicleRadio::radio_toggled_signal = "radio_toggled";
    const char *RailVehicleRadio::channel_changed_signal = "channel_changed";

    void RailVehicleRadio::_bind_methods() {
        BIND_PROPERTY(RailVehicleRadio, Variant::INT, channel_min, "channel");
        BIND_PROPERTY(RailVehicleRadio, Variant::INT, channel_max, "channel");

        ClassDB::bind_method(D_METHOD("radio", "enabled"), &RailVehicleRadio::radio);
        ClassDB::bind_method(D_METHOD("channel_set", "channel"), &RailVehicleRadio::channel_set);
        ClassDB::bind_method(D_METHOD("channel_increase", "step"), &RailVehicleRadio::channel_increase, DEFVAL(1));
        ClassDB::bind_method(D_METHOD("channel_decrease", "step"), &RailVehicleRadio::channel_decrease, DEFVAL(1));
        ClassDB::bind_method(D_METHOD("volume_increase", "step"), &RailVehicleRadio::volume_increase, DEFVAL(1));
        ClassDB::bind_method(D_METHOD("volume_decrease", "step"), &RailVehicleRadio::volume_decrease, DEFVAL(1));
        ClassDB::bind_method(D_METHOD("radio_stop", "pressed"), &RailVehicleRadio::radio_stop);
        ClassDB::bind_method(D_METHOD("radio_call", "pressed", "call"), &RailVehicleRadio::radio_call);
        BIND_ENUM_CONSTANT(RADIO_CALL1);
        BIND_ENUM_CONSTANT(RADIO_CALL3);

        ClassDB::bind_method(D_METHOD("get_enabled"), &RailVehicleRadio::get_enabled);
        ClassDB::bind_method(D_METHOD("get_powered"), &RailVehicleRadio::get_powered);
        ClassDB::bind_method(D_METHOD("get_radio_stop_active"), &RailVehicleRadio::get_radio_stop_active);
        ClassDB::bind_method(D_METHOD("get_channel"), &RailVehicleRadio::get_channel);
        ClassDB::bind_method(D_METHOD("get_volume"), &RailVehicleRadio::get_volume);

        ADD_SIGNAL(MethodInfo(radio_toggled_signal, PropertyInfo(Variant::BOOL, "enabled")));
        ADD_SIGNAL(MethodInfo(channel_changed_signal, PropertyInfo(Variant::INT, "channel")));
    }

    int RailVehicleRadio::get_channel() const {
        return channel;
    }

    double RailVehicleRadio::get_volume() const {
        return volume;
    }

    void RailVehicleRadio::channel_set(const int p_channel) {
        const int new_channel = Math::clamp(p_channel, get_channel_min(), get_channel_max());
        if (new_channel == channel) {
            return;
        }
        channel = new_channel;
        emit_signal(channel_changed_signal, channel);
    }

    void RailVehicleRadio::channel_increase(const int p_step) {
        channel_set(channel + (p_step > 0 ? p_step : 1));
    }

    void RailVehicleRadio::channel_decrease(const int p_step) {
        channel_set(channel - (p_step != 0 ? p_step : 1));
    }

    // Original engine: TTrain::OnCommand_radiovolumeincrease/decrease -> radiovolumeset
    // (Train.cpp:8246-8289), clamped to 0..1
    void RailVehicleRadio::volume_increase(const int p_step) {
        volume = Math::clamp(volume + (VOLUME_STEP * (p_step > 0 ? p_step : 1)), 0.0, 1.0);
    }

    void RailVehicleRadio::volume_decrease(const int p_step) {
        volume = Math::clamp(volume - (VOLUME_STEP * (p_step > 0 ? p_step : 1)), 0.0, 1.0);
    }

    void RailVehicleRadio::_fill_state_dictionary(Dictionary &p_state) const {
        if (!is_simulation_ready()) {
            return;
        }
        p_state["radio_enabled"] = get_enabled();
        p_state["radio_powered"] = get_powered();
        p_state["radio_channel"] = get_channel();
        p_state["radio_volume"] = get_volume();
        p_state["radio_stop_active"] = get_radio_stop_active();
    }

    void RailVehicleRadio::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("radio", Callable(this, "radio"));
        register_command("radio_channel_set", Callable(this, "channel_set"));
        register_command("radio_channel_increase", Callable(this, "channel_increase"));
        register_command("radio_channel_decrease", Callable(this, "channel_decrease"));
        register_command("radio_volume_increase", Callable(this, "volume_increase"));
        register_command("radio_volume_decrease", Callable(this, "volume_decrease"));
        register_command("radio_stop", Callable(this, "radio_stop"));
        register_command("radio_call1", Callable(this, "radio_call").bind(RADIO_CALL1));
        register_command("radio_call3", Callable(this, "radio_call").bind(RADIO_CALL3));
    }

    void RailVehicleRadio::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("radio");
        unregister_command("radio_channel_set");
        unregister_command("radio_channel_increase");
        unregister_command("radio_channel_decrease");
        unregister_command("radio_volume_increase");
        unregister_command("radio_volume_decrease");
        unregister_command("radio_stop");
        unregister_command("radio_call1");
        unregister_command("radio_call3");
    }
} // namespace godot
