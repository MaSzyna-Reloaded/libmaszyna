#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"

namespace godot {
    class VehicleController;
    class RailVehicleBuffCoupl : public RailVehicleComponent {
            GDCLASS(RailVehicleBuffCoupl, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_BUFFERS;
            }

        private:
            static void _bind_methods();

        protected:
        public:
            enum CouplerType {
                COUPLER_TYPE_AUTOMATIC,
                COUPLER_TYPE_SCREW,
                COUPLER_TYPE_CHAIN,
                COUPLER_TYPE_BARE,
                COUPLER_TYPE_ARTICULATED
            };
            enum BufferLocation { BUFFER_LOCATION_FRONT, BUFFER_LOCATION_BACK, BUFFER_LOCATION_BOTH };
            MAKE_MEMBER_GS(double, buffer_stiffness_k, 1.0);
            MAKE_MEMBER_GS(double, buffer_max_compression_tolerance, 0.1);
            MAKE_MEMBER_GS(double, buffer_max_tension_tolerance, 1000.0);
            MAKE_MEMBER_GS(double, coupler_stiffness_k, 1.0);
            MAKE_MEMBER_GS(double, coupler_max_compression_tolerance, 0.1);
            MAKE_MEMBER_GS(double, coupler_max_tension_tolerance, 1000.0);
            MAKE_MEMBER_GS(double, damping_beta, 0.0);
            // masks of RailVehicleController::CouplingFlags
            MAKE_MEMBER_GS(int, allowed_flag, 0);
            MAKE_MEMBER_GS(int, automatic_flag, 0);
            // TCoupling::PowerFlag (MOVER.h:773): 24V and 110V pass unless the FIZ says otherwise
            MAKE_MEMBER_GS(
                    int, power_flag,
                    RailVehicleController::COUPLING_FLAG_POWER_24V | RailVehicleController::COUPLING_FLAG_POWER_110V);
            // TCoupling::PowerCoupling (MOVER.h:774): the coupling power passes by
            MAKE_MEMBER_GS(int, power_coupling, RailVehicleController::COUPLING_FLAG_PERMANENT);
            MAKE_MEMBER_GS(String, control_type, "");
            MAKE_MEMBER_GS_NR(CouplerType, coupler_type, CouplerType::COUPLER_TYPE_AUTOMATIC);
            MAKE_MEMBER_GS_NR(BufferLocation, buffer_location, BufferLocation::BUFFER_LOCATION_FRONT);
            /* What is attached at an end. The coupler owns the coupling, so it answers for it -
             * a consumer picking a submodel to show has no business reading the simulation. */
            virtual bool is_coupled(RailVehicleController::CouplerEnd p_end) const = 0;
            virtual bool is_brake_hose_connected(RailVehicleController::CouplerEnd p_end) const = 0;
            virtual bool is_main_hose_connected(RailVehicleController::CouplerEnd p_end) const = 0;
            /// Which of the two coupled vehicles draws the coupler itself.
            virtual bool is_coupling_owner(RailVehicleController::CouplerEnd p_end) const = 0;
            /// The end of the neighbour this end is attached to.
            virtual RailVehicleController::CouplerEnd
            get_connected_end(RailVehicleController::CouplerEnd p_end) const = 0;
            /* The coupler's strength at an end [N], as the simulation made it of the configuration
             * (FmaxC) */
            virtual double get_coupler_max_force(RailVehicleController::CouplerEnd p_end) const = 0;
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleBuffCoupl::CouplerType)
VARIANT_ENUM_CAST(RailVehicleBuffCoupl::BufferLocation)
