#include "HUDServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *HUDServer::panel_visibility_changed_signal = "panel_visibility_changed";
    const char *HUDServer::hud_visibility_changed_signal = "hud_visibility_changed";
    const char *HUDServer::card_changed_signal = "card_changed";

    void HUDServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("panel_set_visible", "panel", "visible"), &HUDServer::panel_set_visible);
        ClassDB::bind_method(D_METHOD("panel_is_visible", "panel"), &HUDServer::panel_is_visible);
        ClassDB::bind_method(D_METHOD("panel_toggle", "panel"), &HUDServer::panel_toggle);
        ClassDB::bind_method(D_METHOD("hud_set_visible", "visible"), &HUDServer::hud_set_visible);
        ClassDB::bind_method(D_METHOD("hud_is_visible"), &HUDServer::hud_is_visible);
        ClassDB::bind_method(D_METHOD("card_open", "vehicle"), &HUDServer::card_open);
        ClassDB::bind_method(D_METHOD("card_close"), &HUDServer::card_close);
        ClassDB::bind_method(D_METHOD("card_get_vehicle"), &HUDServer::card_get_vehicle);

        ADD_SIGNAL(MethodInfo(
                panel_visibility_changed_signal, PropertyInfo(Variant::STRING_NAME, "panel"),
                PropertyInfo(Variant::BOOL, "visible")));
        ADD_SIGNAL(MethodInfo(hud_visibility_changed_signal, PropertyInfo(Variant::BOOL, "visible")));
        ADD_SIGNAL(MethodInfo(card_changed_signal, PropertyInfo(Variant::RID, "vehicle")));
    }

    /// The card of a freed vehicle closes. No explicit disconnect: callable_mp reports this instance
    /// as the callable's object, so the engine drops the connection when it dies.
    HUDServer::HUDServer() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(VehicleServer::vehicle_freed_signal, callable_mp(this, &HUDServer::_on_vehicle_freed));
    }

    void HUDServer::_on_vehicle_freed(const RID &p_vehicle) {
        if (p_vehicle == card_vehicle) {
            card_close();
        }
    }

    void HUDServer::panel_set_visible(const StringName &p_panel, const bool p_visible) {
        if (panel_is_visible(p_panel) == p_visible) {
            return;
        }
        panels[p_panel] = p_visible;
        emit_signal(panel_visibility_changed_signal, p_panel, p_visible);
    }

    bool HUDServer::panel_is_visible(const StringName &p_panel) const {
        const bool *visible = panels.getptr(p_panel);
        return visible != nullptr && *visible;
    }

    void HUDServer::panel_toggle(const StringName &p_panel) {
        panel_set_visible(p_panel, !panel_is_visible(p_panel));
    }

    void HUDServer::hud_set_visible(const bool p_visible) {
        if (hud_visible == p_visible) {
            return;
        }
        hud_visible = p_visible;
        emit_signal(hud_visibility_changed_signal, hud_visible);
    }

    bool HUDServer::hud_is_visible() const {
        return hud_visible;
    }

    void HUDServer::card_open(const RID &p_vehicle) {
        if (p_vehicle == card_vehicle) {
            return;
        }
        card_vehicle = p_vehicle;
        emit_signal(card_changed_signal, card_vehicle);
    }

    void HUDServer::card_close() {
        card_open(RID());
    }

    RID HUDServer::card_get_vehicle() const {
        return card_vehicle;
    }
} // namespace godot
