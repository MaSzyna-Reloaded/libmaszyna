#include "MaszynaTrainsetClassifier.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

#include <godot_cpp/core/math.hpp>

namespace godot {
    void MaszynaTrainsetClassifier::_bind_methods() {}

    RailVehicleServer::TrainsetType MaszynaTrainsetClassifier::classify(const TypedArray<RID> &p_vehicles) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, RailVehicleServer::TRAINSET_TYPE_NONE);
        int fast = 0;
        int goods = 0;
        int passengers = 0;
        for (const Variant &vehicle: p_vehicles) {
            const Ref<RailVehicleController> controller = vehicle_server->vehicle_get_controller(vehicle);
            // a locomotive - Power > 1 - is no wagon (Driver.cpp:2156)
            if (controller.is_null() || controller->get_power() >= WAGON_POWER_MAX) {
                continue;
            }
            const Ref<RailVehicleBrake> brake =
                    controller->get_rail_component(RailVehicleComponentType::COMPONENT_BRAKES);
            const int delays = brake.is_valid() ? brake->get_cntrl_brake_delays() : RailVehicleBrake::BRAKE_DELAY_NONE;
            if ((delays & RailVehicleBrake::BRAKE_DELAY_R) != 0) {
                ++fast;
            } else if ((delays & RailVehicleBrake::BRAKE_DELAY_G) != 0) {
                ++goods;
            } else {
                ++passengers;
            }
        }
        if (fast + goods + passengers == 0) {
            return RailVehicleServer::TRAINSET_TYPE_NONE;
        }
        return goods < MIN(PASSENGER_GOODS_LIMIT, fast + passengers) ? RailVehicleServer::TRAINSET_TYPE_PASSENGER
                                                                     : RailVehicleServer::TRAINSET_TYPE_CARGO;
    }
} // namespace godot
