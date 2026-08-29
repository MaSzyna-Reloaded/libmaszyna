#pragma once
#include "../core/TrainPart.hpp"
#include "macros.hpp"
#include "resources/wipers/WiperListItem.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class TrainController;

    /* Wraps the FIZ WiperList: section.
     *
     * NOTE: this fork of the Mover simulation has no field anywhere for wiper state or
     * configuration - "wiper" does not appear a single time in MOVER.h, Mover.cpp, or any
     * GDScript addon. There is nothing in TMoverParameters to write these values into, so
     * _do_update_internal_mover() only stores the configuration on this node; it has no effect
     * on the simulated vehicle. This class exists so the section has a documented home, ready
     * to be wired up if/when wiper simulation is ever implemented. */
    class TrainWipers : public TrainPart {
            GDCLASS(TrainWipers, TrainPart);

        private:
            static void _bind_methods();

        protected:
            void _do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) override;

        public:
            MAKE_MEMBER_GS(double, angle, 0.0);
            MAKE_MEMBER_GS(int, default_position, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<WiperListItem>, positions)
    };
} // namespace godot
