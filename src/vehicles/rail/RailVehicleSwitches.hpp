#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleDimmerListItem.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class VehicleController;

    /* Wraps the FIZ Switches: and DimmerList: sections.
     *
     * The switch types are the cab's: how its pantograph, converter and line contactor switches
     * behave (Train.cpp:3178-3287, 4390-4423, 5048). RelayResetButtonX= assign the relays of the
     * cab's relay reset buttons (UniversalResetButtonFlag). PantographPresets= is the Mover's
     * PantsPreset, which the cab's pantograph selector walks. ModernDimmer= and DimmerList: are
     * the cab's headlight dimmer, not ported (TODO.md); PantographPresetDefault= is no key of the
     * original. */
    class RailVehicleSwitches : public RailVehicleComponent {
            GDCLASS(RailVehicleSwitches, RailVehicleComponent);


        public:
            /* Which pantographs a position of the cab's selector raises (a digit of
             * PantographPresets=, Train.cpp:3523-3526) - by the cab's ends, not the vehicle's:
             * the pantograph over the end the driver sits at, the one over the other, both. */
            /* The cab's customizable relay reset buttons (relayreset1..3_bt:, UniversalResetButtonFlag) */
            enum RelayResetButton {
                RELAY_RESET_BUTTON_1,
                RELAY_RESET_BUTTON_2,
                RELAY_RESET_BUTTON_3,
            };
            enum PantographPreset {
                PANTOGRAPH_PRESET_NONE = 0,
                PANTOGRAPH_PRESET_OWN_END = 1,
                PANTOGRAPH_PRESET_OTHER_END = 2,
                PANTOGRAPH_PRESET_BOTH = PANTOGRAPH_PRESET_OWN_END | PANTOGRAPH_PRESET_OTHER_END,
            };

            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_SWITCHES;
            }

        private:
            static void _bind_methods();

        protected:
            void _fill_config_dictionary(Dictionary &p_config) const override;

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_sand_active() const = 0;
            MAKE_MEMBER_GS(bool, pantograph_impulse, false);
            MAKE_MEMBER_GS(bool, converter_impulse, false);
            MAKE_MEMBER_GS(bool, motor_connectors_impulse, true);
            MAKE_MEMBER_GS(int, relay_reset_button_1, 0);
            MAKE_MEMBER_GS(int, relay_reset_button_2, 0);
            MAKE_MEMBER_GS(int, relay_reset_button_3, 0);
            /* A PantographPreset for each position of the cab's selector (PantographPresets=,
               default Mover.cpp:11402) */
            MAKE_MEMBER_GS(
                    PackedInt32Array, pantograph_presets,
                    PackedInt32Array(
                            {PANTOGRAPH_PRESET_NONE, PANTOGRAPH_PRESET_OWN_END, PANTOGRAPH_PRESET_BOTH,
                             PANTOGRAPH_PRESET_OTHER_END}));
            MAKE_MEMBER_GS(int, pantograph_preset_default, 0);
            MAKE_MEMBER_GS(bool, modern_dimmer, false);
            MAKE_MEMBER_GS(bool, dimmer_list_cycle, false);
            MAKE_MEMBER_GS(int, dimmer_list_default_position, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleDimmerListItem>, dimmer_list_positions)
            /* The position of the pantograph selector of the cab at `p_end` (PantsPreset.second) */
            virtual int get_pantograph_preset_position(RailVehicleController::CouplerEnd p_end) const = 0;
            /* The preset that position selects */
            virtual PantographPreset get_pantograph_preset(RailVehicleController::CouplerEnd p_end) const = 0;
            virtual void sand(bool p_active) = 0;
            /* Resets the relays `p_button` is assigned (RelayResetButtonN=), with the low voltage
               (UniversalResetButton(), Mover.cpp:6004; Train.cpp:5175) */
            virtual void universal_relay_reset(RelayResetButton p_button) = 0;
            /* Moves the selector of the cab at `p_end` by one position, within the presets */
            virtual void next_pantograph_preset(RailVehicleController::CouplerEnd p_end) = 0;
            virtual void previous_pantograph_preset(RailVehicleController::CouplerEnd p_end) = 0;
            void _register_commands() override;
            void _unregister_commands() override;
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleSwitches::PantographPreset);
VARIANT_ENUM_CAST(RailVehicleSwitches::RelayResetButton);
