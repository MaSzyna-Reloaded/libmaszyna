#include "TrainWipers.hpp"

namespace godot {
    void TrainWipers::_bind_methods() {
        BIND_PROPERTY(Variant::FLOAT, "angle", "angle", &TrainWipers::set_angle, &TrainWipers::get_angle, "angle");
        BIND_PROPERTY(
                Variant::INT, "default_position", "default_position", &TrainWipers::set_default_position,
                &TrainWipers::get_default_position, "default_position");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "positions", "positions", &TrainWipers::set_positions, &TrainWipers::get_positions,
                "positions", PROPERTY_HINT_TYPE_STRING, "WiperListItem");
    }

    void TrainWipers::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        // Not wired to the mover: see the class-level note in TrainWipers.hpp.
    }
} // namespace godot
