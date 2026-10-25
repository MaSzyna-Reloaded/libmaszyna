#include "MaszynaE3DRoadVehicleRenderingServer.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/road/RoadVehicleController.hpp"

namespace godot {
    /* Original engine: a vehicle's lamps (TDynamicObject::LoadMMediaFile(), DynObj.cpp:2371-2402) -
     * a car's head lights are its front end's left and right headlamps, its tail lights its rear
     * end's red markers, each or the pair of them */
    static constexpr const char *HEAD_LAMPS[] = {"headlamp12", "headlamp13"};
    static constexpr const char *TAIL_LAMPS[] = {"endsignal22", "endsignal23", "endsignals2"};

    MaszynaE3DRoadVehicleRenderingServer::~MaszynaE3DRoadVehicleRenderingServer() {
        // the base detaches every vehicle while these hooks still answer
        _detach_all();
    }

    void MaszynaE3DRoadVehicleRenderingServer::_vehicle_detaching(const RID &p_vehicle) {
        vehicles_lamps.erase(p_vehicle);
    }

    /* Only the lamps the model has, dark until the car's lights are looked at */
    void MaszynaE3DRoadVehicleRenderingServer::_vehicle_parts_bound(const RID &p_vehicle) {
        const ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const Visual *visual = _get_visual(p_vehicle);
        Lamps &lamps = vehicles_lamps[p_vehicle];
        lamps.state.clear();
        lamps.shown = -1;
        if (models == nullptr || visual == nullptr || !visual->model.is_valid()) {
            return;
        }
        for (const char *lamp: HEAD_LAMPS) {
            if (models->instance_has_light(visual->model, lamp)) {
                lamps.state[lamp] = false;
            }
        }
        for (const char *lamp: TAIL_LAMPS) {
            if (models->instance_has_light(visual->model, lamp)) {
                lamps.state[lamp] = false;
            }
        }
        _update_lamps(p_vehicle, lamps);
    }

    void MaszynaE3DRoadVehicleRenderingServer::_vehicle_update(const RID &p_vehicle) {
        if (Lamps *lamps = vehicles_lamps.getptr(p_vehicle); lamps != nullptr) {
            _update_lamps(p_vehicle, *lamps);
        }
    }

    /* Sent when the car's lights are others than its lamps show */
    void MaszynaE3DRoadVehicleRenderingServer::_update_lamps(const RID &p_vehicle, Lamps &p_lamps) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const Visual *visual = _get_visual(p_vehicle);
        if (models == nullptr || vehicle_server == nullptr || visual == nullptr || !visual->model.is_valid() ||
            p_lamps.state.is_empty()) {
            return;
        }
        const Ref<RoadVehicleController> controller = vehicle_server->vehicle_get_controller(p_vehicle);
        const BitField<RoadVehicleController::Lights> lights =
                controller.is_valid() ? controller->get_lights()
                                      : BitField<RoadVehicleController::Lights>(RoadVehicleController::LIGHTS_NONE);
        if (static_cast<int64_t>(lights) == p_lamps.shown) {
            return;
        }
        p_lamps.shown = static_cast<int64_t>(lights);
        for (const char *lamp: HEAD_LAMPS) {
            if (p_lamps.state.has(lamp)) {
                p_lamps.state[lamp] = lights.has_flag(RoadVehicleController::LIGHTS_HEAD);
            }
        }
        for (const char *lamp: TAIL_LAMPS) {
            if (p_lamps.state.has(lamp)) {
                p_lamps.state[lamp] = lights.has_flag(RoadVehicleController::LIGHTS_TAIL);
            }
        }
        models->instance_set_lights_state(visual->model, p_lamps.state);
    }
} // namespace godot
