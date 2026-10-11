#pragma once

#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    // Ports the original engine's horn model (Train.cpp's OnCommand_hornlowactivate/
    // OnCommand_hornhighactivate/OnCommand_whistleactivate, DynObj.cpp's per-frame
    // WarningSignal -> sHorn1/sHorn2/sHorn3 dispatch): exactly 3 fixed slots (low/high/
    // whistle), the original's warning signal bits 1/2/4 - the same bit convention
    // RailVehicleSecuritySystem::emergency_signal already uses for the emergency brake warning
    // signal. The original engine has no FIZ-level config for
    // horn count - a vehicle's 0-3 horn complement is implied entirely by which MMD cabin
    // button (horn_bt:/hornlow_bt:/hornhigh_bt:/whistle_bt:) and sound (horn1:/horn2:/
    // horn3:) labels it declares. Whether a cab's key reaches a horn is the cab's business - the
    // original refuses it in a cab without the horn's button (Train.cpp:7936, 7980, 8024); the
    // vehicle sounds whatever it is told.
    class RailVehicleHorns : public RailVehicleComponent {
            GDCLASS(RailVehicleHorns, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_HORNS;
            }

        private:
            static void _bind_methods();

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_low_pressed() const = 0;
            virtual bool get_high_pressed() const = 0;
            virtual bool get_whistle_pressed() const = 0;
            virtual bool get_low_active() const = 0;
            virtual bool get_high_active() const = 0;
            virtual bool get_whistle_active() const = 0;
            virtual int get_horn() const = 0;
            virtual void set_horn_low(bool p_state) = 0;
            virtual void set_horn_high(bool p_state) = 0;
            virtual void set_whistle(bool p_state) = 0;
    };
} // namespace godot
