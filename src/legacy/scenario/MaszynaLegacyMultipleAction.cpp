#include "MaszynaLegacyMultipleAction.hpp"
#include "scenario/ScenarioEventServer.hpp"

namespace godot {
    void MaszynaLegacyMultipleAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_events", "events"), &MaszynaLegacyMultipleAction::set_events);
        ClassDB::bind_method(D_METHOD("get_events"), &MaszynaLegacyMultipleAction::get_events);
        ClassDB::bind_method(D_METHOD("set_else_events", "events"), &MaszynaLegacyMultipleAction::set_else_events);
        ClassDB::bind_method(D_METHOD("get_else_events"), &MaszynaLegacyMultipleAction::get_else_events);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "events", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_events", "get_events");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "else_events", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_else_events",
                "get_else_events");
    }

    void MaszynaLegacyMultipleAction::run(const RID &p_event, const RID &p_activator) {
        ScenarioEventServer *server = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < events.size(); i++) {
            server->event_queue(events[i], p_activator);
        }
    }

    void MaszynaLegacyMultipleAction::run_else(const RID &p_event, const RID &p_activator) {
        ScenarioEventServer *server = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < else_events.size(); i++) {
            server->event_queue(else_events[i], p_activator);
        }
    }

    void MaszynaLegacyMultipleAction::set_events(const TypedArray<RID> &p_events) {
        events = p_events;
    }

    TypedArray<RID> MaszynaLegacyMultipleAction::get_events() const {
        return events;
    }

    void MaszynaLegacyMultipleAction::set_else_events(const TypedArray<RID> &p_events) {
        else_events = p_events;
    }

    TypedArray<RID> MaszynaLegacyMultipleAction::get_else_events() const {
        return else_events;
    }
} // namespace godot
