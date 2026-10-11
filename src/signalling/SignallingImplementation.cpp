#include "SignallingImplementation.hpp"

namespace godot {
    void SignallingImplementation::_bind_methods() {
        GDVIRTUAL_BIND(_system_attached, "system");
        GDVIRTUAL_BIND(_system_detached, "system");
        GDVIRTUAL_BIND(_signal_head_added, "system", "signal_head");
        GDVIRTUAL_BIND(_signal_head_removed, "system", "signal_head");
        GDVIRTUAL_BIND(_source_added, "system", "source");
        GDVIRTUAL_BIND(_source_removed, "system", "source");
        GDVIRTUAL_BIND(_signal_head_aspect_changed, "system", "signal_head", "aspect");
        GDVIRTUAL_BIND(_handle_event, "system", "event", "arguments");
        GDVIRTUAL_BIND(_handle_source_event, "system", "source", "event", "arguments");
        GDVIRTUAL_BIND(_switch_added, "system", "switch");
        GDVIRTUAL_BIND(_switch_removed, "system", "switch");
        GDVIRTUAL_BIND(_switch_created, "system", "switch");
        GDVIRTUAL_BIND(_switch_freed, "system", "switch");
        GDVIRTUAL_BIND(_switch_movement_finished, "system", "switch", "active_track");
        GDVIRTUAL_BIND(_system_linked, "system", "other");
        GDVIRTUAL_BIND(_system_unlinked, "system", "other");
        GDVIRTUAL_BIND(_route_added, "system", "route");
        GDVIRTUAL_BIND(_route_removed, "system", "route");
        GDVIRTUAL_BIND(_route_set_requested, "system", "route");
        GDVIRTUAL_BIND(_route_cancel_requested, "system", "route");
        GDVIRTUAL_BIND(_route_release_requested, "system", "route");
        GDVIRTUAL_BIND(_route_emergency_release_requested, "system", "route");
    }

    void SignallingImplementation::system_attached(const RID &p_system) {
        GDVIRTUAL_CALL(_system_attached, p_system);
    }

    void SignallingImplementation::system_detached(const RID &p_system) {
        GDVIRTUAL_CALL(_system_detached, p_system);
    }

    void SignallingImplementation::signal_head_added(const RID &p_system, const RID &p_signal_head) {
        GDVIRTUAL_CALL(_signal_head_added, p_system, p_signal_head);
    }

    void SignallingImplementation::signal_head_removed(const RID &p_system, const RID &p_signal_head) {
        GDVIRTUAL_CALL(_signal_head_removed, p_system, p_signal_head);
    }

    void SignallingImplementation::source_added(const RID &p_system, const RID &p_source) {
        GDVIRTUAL_CALL(_source_added, p_system, p_source);
    }

    void SignallingImplementation::source_removed(const RID &p_system, const RID &p_source) {
        GDVIRTUAL_CALL(_source_removed, p_system, p_source);
    }

    void SignallingImplementation::signal_head_aspect_changed(
            const RID &p_system, const RID &p_signal_head, const StringName &p_aspect) {
        GDVIRTUAL_CALL(_signal_head_aspect_changed, p_system, p_signal_head, p_aspect);
    }

    void SignallingImplementation::handle_event(
            const RID &p_system, const StringName &p_event, const Dictionary &p_arguments) {
        GDVIRTUAL_CALL(_handle_event, p_system, p_event, p_arguments);
    }

    void SignallingImplementation::handle_source_event(
            const RID &p_system, const RID &p_source, const StringName &p_event, const Dictionary &p_arguments) {
        GDVIRTUAL_CALL(_handle_source_event, p_system, p_source, p_event, p_arguments);
    }
    void SignallingImplementation::switch_movement_finished(
            const RID &p_system, const StringName &p_switch, const int p_active_track) {
        GDVIRTUAL_CALL(_switch_movement_finished, p_system, p_switch, p_active_track);
    }

    void SignallingImplementation::switch_created(const RID &p_system, const StringName &p_switch) {
        GDVIRTUAL_CALL(_switch_created, p_system, p_switch);
    }

    void SignallingImplementation::switch_freed(const RID &p_system, const StringName &p_switch) {
        GDVIRTUAL_CALL(_switch_freed, p_system, p_switch);
    }

    void SignallingImplementation::switch_added(const RID &p_system, const StringName &p_switch) {
        GDVIRTUAL_CALL(_switch_added, p_system, p_switch);
    }

    void SignallingImplementation::switch_removed(const RID &p_system, const StringName &p_switch) {
        GDVIRTUAL_CALL(_switch_removed, p_system, p_switch);
    }

    void SignallingImplementation::system_linked(const RID &p_system, const RID &p_other) {
        GDVIRTUAL_CALL(_system_linked, p_system, p_other);
    }

    void SignallingImplementation::system_unlinked(const RID &p_system, const RID &p_other) {
        GDVIRTUAL_CALL(_system_unlinked, p_system, p_other);
    }

    void SignallingImplementation::route_added(const RID &p_system, const RID &p_route) {
        GDVIRTUAL_CALL(_route_added, p_system, p_route);
    }

    void SignallingImplementation::route_removed(const RID &p_system, const RID &p_route) {
        GDVIRTUAL_CALL(_route_removed, p_system, p_route);
    }

    void SignallingImplementation::route_set_requested(const RID &p_system, const RID &p_route) {
        GDVIRTUAL_CALL(_route_set_requested, p_system, p_route);
    }

    void SignallingImplementation::route_cancel_requested(const RID &p_system, const RID &p_route) {
        GDVIRTUAL_CALL(_route_cancel_requested, p_system, p_route);
    }

    void SignallingImplementation::route_release_requested(const RID &p_system, const RID &p_route) {
        GDVIRTUAL_CALL(_route_release_requested, p_system, p_route);
    }

    void SignallingImplementation::route_emergency_release_requested(const RID &p_system, const RID &p_route) {
        GDVIRTUAL_CALL(_route_emergency_release_requested, p_system, p_route);
    }
} // namespace godot
