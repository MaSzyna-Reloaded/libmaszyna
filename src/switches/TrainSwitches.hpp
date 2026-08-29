#pragma once
#include "../core/TrainPart.hpp"
#include "macros.hpp"
#include "resources/switches/DimmerListItem.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class TrainController;

    /* Wraps the FIZ Switches: and DimmerList: sections.
     *
     * NOTE: PantSwitchType/ConvSwitchType/StLinSwitchType exist as std::string fields on
     * TMoverParameters (see MOVER.h, marked "TODO: move these switch types where they belong")
     * but are not read anywhere else in this codebase, so setting them currently has no observable
     * effect on the simulation. RelayResetButtonX=, PantographPresets=, PantographPresetDefault=,
     * ModernDimmer= and DimmerList: have no backing field at all - they are stored on this node
     * only, ready to be wired up if support is ever added to the mover. */
    class TrainSwitches : public TrainPart {
            GDCLASS(TrainSwitches, TrainPart);

        private:
            static void _bind_methods();

        protected:
            void _do_update_internal_mover(TMoverParameters *p_mover) override;
            void _do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) override;

        public:
            MAKE_MEMBER_GS(bool, pantograph_impulse, false);
            MAKE_MEMBER_GS(bool, converter_impulse, false);
            MAKE_MEMBER_GS(bool, motor_connectors_impulse, true);
            MAKE_MEMBER_GS(int, relay_reset_button_1, 0);
            MAKE_MEMBER_GS(int, relay_reset_button_2, 0);
            MAKE_MEMBER_GS(int, relay_reset_button_3, 0);
            MAKE_MEMBER_GS(PackedInt32Array, pantograph_presets, PackedInt32Array());
            MAKE_MEMBER_GS(int, pantograph_preset_default, 0);
            MAKE_MEMBER_GS(bool, modern_dimmer, false);
            MAKE_MEMBER_GS(bool, dimmer_list_cycle, false);
            MAKE_MEMBER_GS(int, dimmer_list_default_position, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<DimmerListItem>, dimmer_list)
    };
} // namespace godot
