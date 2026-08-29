#include "TrainAIHints.hpp"

namespace godot {
    void TrainAIHints::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                Variant::INT, "pantograph_state", "pantograph_state", &TrainAIHints::set_pantograph_state,
                &TrainAIHints::get_pantograph_state, "pantograph_state", PROPERTY_HINT_ENUM, "Front,Rear,Both");
        BIND_PROPERTY(
                Variant::BOOL, "raise_pantographs_when_idle", "raise_pantographs_when_idle",
                &TrainAIHints::set_raise_pantographs_when_idle, &TrainAIHints::get_raise_pantographs_when_idle,
                "raise_pantographs_when_idle");
        BIND_PROPERTY(
                Variant::FLOAT, "local_brake_acceleration_factor", "local_brake_acceleration_factor",
                &TrainAIHints::set_local_brake_acceleration_factor,
                &TrainAIHints::get_local_brake_acceleration_factor, "local_brake_acceleration_factor");

        BIND_ENUM_CONSTANT(PANTOGRAPH_STATE_FRONT);
        BIND_ENUM_CONSTANT(PANTOGRAPH_STATE_REAR);
        BIND_ENUM_CONSTANT(PANTOGRAPH_STATE_BOTH);
    }

    void TrainAIHints::_do_update_internal_mover(TMoverParameters *p_mover) {
        ASSERT_MOVER(p_mover);
        TrainPart::_do_update_internal_mover(p_mover);

        p_mover->AIHintPantstate = pantograph_state;
        p_mover->AIHintPantUpIfIdle = raise_pantographs_when_idle;
        p_mover->AIHintLocalBrakeAccFactor = local_brake_acceleration_factor;
    }

    void TrainAIHints::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        ASSERT_MOVER(p_mover);
    }
} // namespace godot
