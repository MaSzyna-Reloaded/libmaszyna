#include "maszyna/scenario/MaszynaScenarioEventImplementation.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"

namespace godot {
    void MaszynaScenarioEventImplementation::system_detached(const RID &p_system) {
        if (MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance(); events != nullptr) {
            events->system_release(p_system);
        }
    }

    bool MaszynaScenarioEventImplementation::event_can_queue(
            const RID & /* p_system */, const RID &p_event, const RID & /* p_activator */) const {
        const MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance();
        return events == nullptr || events->event_can_queue(p_event);
    }

    double MaszynaScenarioEventImplementation::event_get_run_time(
            const RID & /* p_system */, const RID &p_event, const RID &p_activator, const double p_now,
            const double p_run_time) const {
        const MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance();
        return events != nullptr ? events->event_get_run_time(p_event, p_activator, p_now, p_run_time) : p_run_time;
    }

    void MaszynaScenarioEventImplementation::event_ran(
            const RID & /* p_system */, const RID &p_event, const RID &p_activator) {
        if (MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance(); events != nullptr) {
            events->event_ran(p_event, p_activator);
        }
    }

    void MaszynaScenarioEventImplementation::scheduled(
            const RID & /* p_system */, const RID &p_owner, const int64_t p_ticket) {
        if (MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance(); events != nullptr) {
            events->scheduled(p_owner, p_ticket);
        }
    }
} // namespace godot
