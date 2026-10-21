#include "MaszynaMultipleAction.hpp"
#include "scenario/ScenarioEventServer.hpp"

namespace godot {
    void MaszynaMultipleAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_events", "events"), &MaszynaMultipleAction::set_events);
        ClassDB::bind_method(D_METHOD("get_events"), &MaszynaMultipleAction::get_events);
        ClassDB::bind_method(D_METHOD("set_else_events", "events"), &MaszynaMultipleAction::set_else_events);
        ClassDB::bind_method(D_METHOD("get_else_events"), &MaszynaMultipleAction::get_else_events);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "events", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_events", "get_events");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "else_events", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_else_events",
                "get_else_events");
    }

    void MaszynaMultipleAction::run(const RID &p_event, const RID &p_activator) {
        ScenarioEventServer *server = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < events.size(); i++) {
            server->event_queue(events[i], p_activator);
        }
    }

    void MaszynaMultipleAction::run_else(const RID &p_event, const RID &p_activator) {
        ScenarioEventServer *server = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < else_events.size(); i++) {
            server->event_queue(else_events[i], p_activator);
        }
    }

    void MaszynaMultipleAction::set_events(const TypedArray<RID> &p_events) {
        events = p_events;
    }

    TypedArray<RID> MaszynaMultipleAction::get_events() const {
        return events;
    }

    void MaszynaMultipleAction::set_else_events(const TypedArray<RID> &p_events) {
        else_events = p_events;
    }

    TypedArray<RID> MaszynaMultipleAction::get_else_events() const {
        return else_events;
    }
} // namespace godot
