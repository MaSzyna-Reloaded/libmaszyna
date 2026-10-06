#pragma once
#include "vehicles/base/VehiclePhysicsNode.hpp"

namespace godot {
    /* A rail vehicle's presence in the scene tree: a VehiclePhysicsNode whose vehicle is a rail
     * one - placed on the route and stepped there (RailVehicleServer), with the type and the load
     * the scenery gives it. */
    class RailVehiclePhysicsNode : public VehiclePhysicsNode {
            GDCLASS(RailVehiclePhysicsNode, VehiclePhysicsNode)

        private:
            String type_name;
            String load_name;
            double load_amount = 0.0;

        protected:
            static void _bind_methods();
            void _prepare_vehicle(const RID &p_vehicle) override;

        public:
            /* The name of the vehicle's type - the original's CHK/MMD name TMoverParameters keeps
             * as TypeName (DynObj.cpp:2019) */
            void set_type_name(const String &p_type_name);
            String get_type_name() const;
            /* What the vehicle carries when the scenery places it (`loadtype`, `loadcount`) */
            void set_load_name(const String &p_load_name);
            String get_load_name() const;
            void set_load_amount(double p_load_amount);
            double get_load_amount() const;
    };
} // namespace godot
