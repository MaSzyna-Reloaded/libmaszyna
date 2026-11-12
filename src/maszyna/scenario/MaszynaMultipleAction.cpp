#include "MaszynaMultipleAction.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
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
        _queue(events, p_event, p_activator);
    }

    void MaszynaMultipleAction::run_else(const RID &p_event, const RID &p_activator) {
        _queue(else_events, p_event, p_activator);
    }

    /// multi_event::run_() (Event.cpp:1290-1307): the event queues itself again only with a
    /// sensible period of repetition
    void MaszynaMultipleAction::_queue(const TypedArray<RID> &p_events, const RID &p_event, const RID &p_activator) {
        const ScenarioEventServer *server = ScenarioEventServer::get_instance();
        MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance();
        ERR_FAIL_NULL(server);
        ERR_FAIL_NULL(events);
        for (int i = 0; i < p_events.size(); i++) {
            const RID child = p_events[i];
            if (child == p_event && server->event_get_delay(p_event) < SELF_QUEUE_MIN_DELAY) {
                continue;
            }
            events->event_add_to_query(child, p_activator);
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
