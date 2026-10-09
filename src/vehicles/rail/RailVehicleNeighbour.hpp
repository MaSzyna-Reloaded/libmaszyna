#pragma once

#include "vehicles/rail/RailVehicleController.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* The nearest vehicle along the route from one end of a vehicle
     * (RailVehicleServer::vehicle_find_vehicle(); the original's neighbour_data, DynObj.h). */
    class RailVehicleNeighbour : public RefCounted {
            GDCLASS(RailVehicleNeighbour, RefCounted)

        private:
            /* the vehicle found */
            RID vehicle_rid = RID();
            /* its end facing the one searched from */
            RailVehicleController::CouplerEnd end = RailVehicleController::COUPLER_END_FRONT;
            /* between the two vehicles' ends [m] */
            double distance = 0.0;

        protected:
            static void _bind_methods();

        public:
            void set_vehicle_rid(const RID &p_vehicle_rid);
            RID get_vehicle_rid() const;
            void set_end(RailVehicleController::CouplerEnd p_end);
            RailVehicleController::CouplerEnd get_end() const;
            void set_distance(double p_distance);
            double get_distance() const;
    };
} // namespace godot
