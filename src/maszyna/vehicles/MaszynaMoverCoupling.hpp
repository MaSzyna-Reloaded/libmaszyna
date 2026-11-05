#pragma once
#include "vehicles/rail/RailVehicleController.hpp"
#include <godot_cpp/classes/object.hpp>

namespace godot {
    /* The core's coupling flags and vehicle ends as the Mover's coupling:: bits and end:: (MOVER.h:157-176),
     * and back. MaSzyna's data writes its coupling numbers in the same bits - a FIZ's coupler masks, a
     * .scn's couplingdata, a scenario's Shunt - so its readers take the flags from here too. */
    class MaszynaMoverCoupling : public Object {
            GDCLASS(MaszynaMoverCoupling, Object)

        protected:
            static void _bind_methods();

        public:
            static int flags_to_mover(BitField<RailVehicleController::CouplingFlags> p_flags);
            static BitField<RailVehicleController::CouplingFlags> flags_from_mover(int p_mover_flags);
            /* Whether a number has a bit that stands for no coupling flag */
            static bool has_unknown_bits(int p_mover_flags);
            static int end_to_mover(RailVehicleController::CouplerEnd p_end);
            static RailVehicleController::CouplerEnd end_from_mover(int p_mover_end);
    };
} // namespace godot
