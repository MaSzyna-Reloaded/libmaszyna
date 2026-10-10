#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"

namespace godot {
    class VehicleController;
    class RailVehicleDoors : public RailVehicleComponent {
            GDCLASS(RailVehicleDoors, RailVehicleComponent)


        public:
            /// A side's doors came fully open (side: Side)
            static const char *doors_opened_signal;
            /// A side's doors came fully closed (side: Side)
            static const char *doors_closed_signal;

            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_DOORS;
            }

        private:
            static void _bind_methods();

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_locked() const = 0;
            /* The selected door permit preset (Doors.permit_preset), 0-based */
            virtual int get_permit_preset() const = 0;
            virtual bool get_lock_enabled() const = 0;
            virtual bool get_step_enabled() const = 0;
            virtual int get_open_control() const = 0;
            virtual bool get_left_open() const = 0;
            virtual bool get_left_closed() const = 0;
            /* The door alone fully closed, its step whatever (is_door_closed, MOVER.h:965) */
            virtual bool get_left_door_closed() const = 0;
            virtual bool get_left_open_permit() const = 0;
            virtual bool get_left_local_open() const = 0;
            virtual bool get_left_remote_open() const = 0;
            virtual double get_left_position() const = 0;
            virtual double get_left_position_normalized() const = 0;
            virtual bool get_left_operating() const = 0;
            virtual double get_left_step_position() const = 0;
            virtual bool get_left_step_operating() const = 0;
            virtual bool get_right_open() const = 0;
            virtual bool get_right_closed() const = 0;
            virtual bool get_right_door_closed() const = 0;
            virtual bool get_right_open_permit() const = 0;
            virtual bool get_right_local_open() const = 0;
            virtual bool get_right_remote_open() const = 0;
            virtual double get_right_position() const = 0;
            virtual double get_right_position_normalized() const = 0;
            virtual bool get_right_operating() const = 0;
            virtual double get_right_step_position() const = 0;
            virtual bool get_right_step_operating() const = 0;
            /* The departure signal is given (DepartureSignal, Mover.cpp:7899) */
            virtual bool get_departure_signal() const = 0;
            /* The vehicle's door warning sounds: it has one, the signal is given and the low
             * voltage is there (DynObj.cpp:4768-4787) */
            virtual bool get_departure_signal_sounding() const = 0;
            /* The doors are worked only from the cab (Doors.remote_only, ChangeDoorControlMode) */
            virtual bool get_remote_only() const = 0;
            enum PermitLight {
                PERMIT_LIGHT_CONTINUOUS,
                PERMIT_LIGHT_FLASHING_ON_PERMISSION_WITH_STEP,
                PERMIT_LIGHT_FLASHING_ON_PERMISSION,
                PERMIT_LIGHT_FLASHING_ALWAYS
            };
            enum Side { SIDE_RIGHT, SIDE_LEFT };
            enum Voltage {
                VOLTAGE_AUTO,
                VOLTAGE_0,
                VOLTAGE_12,
                VOLTAGE_24,
                VOLTAGE_110,
            };
            enum Type {
                TYPE_SHIFT,
                TYPE_ROTATE,
                TYPE_FOLD,
                TYPE_PLUG,
            };
            enum PlatformType { PLATFORM_TYPE_SHIFT, PLATFORM_TYPE_ROTATE };
            enum Controls {
                CONTROLS_PASSENGER,
                CONTROLS_AUTOMATIC,
                CONTROLS_DRIVER,
                CONTROLS_CONDUCTOR,
                CONTROLS_MIXED,
            };
            virtual void permit_step(bool p_state) = 0;
            /* The side comes last, so a command binds it and takes the state (doors_left_permit) */
            virtual void permit_doors(bool p_state, Side p_side) = 0;
            virtual void operate_doors(bool p_state, Side p_side) = 0;
            /* The door handle of the vehicle itself, as a passenger or the guard works it - nothing
             * is sent along the trainset (OperateDoors(range_t::local), Driver.cpp:4340-4341) */
            virtual void operate_doors_locally(bool p_state, Side p_side) = 0;
            virtual void door_lock(bool p_state) = 0;
            virtual void door_remote_control(bool p_state) = 0;
            /* The departure signal given or taken back, along the trainset (signal_departure) */
            virtual void signal_departure(bool p_state) = 0;
            virtual void next_permit_preset() = 0;
            virtual void previous_permit_preset() = 0;
            /* The cab mirrors (DynObj.cpp:4207-4236): how far each side is unfolded, 0 folded .. 1 out */
            virtual double get_mirror_left_position() const = 0;
            virtual double get_mirror_right_position() const = 0;
            /* The mirrors are kept folded (MirrorForbidden, the cab's mirrors_sw: - Train.cpp:7726) */
            virtual bool get_mirrors_forbidden() const = 0;
            virtual void forbid_mirrors(bool p_state) = 0;

        private:
            MAKE_MEMBER_GS_NR(Type, type, Type::TYPE_ROTATE);
            MAKE_MEMBER_GS_NR(Controls, open_method, Controls::CONTROLS_PASSENGER);
            MAKE_MEMBER_GS_NR(Controls, close_method, Controls::CONTROLS_PASSENGER);
            MAKE_MEMBER_GS(float, open_time, -1.0f);
            MAKE_MEMBER_GS(float, open_speed, 1.0f);
            MAKE_MEMBER_GS(float, close_speed, 1.0f);
            /* DoorMaxShiftL/R=, DoorMaxShiftPlug= absent: no movement (range, range_out, MOVER.h:1434-1435) */
            MAKE_MEMBER_GS(float, max_shift, 0.0f);
            MAKE_MEMBER_GS_NR(Voltage, voltage, Voltage::VOLTAGE_AUTO);
            MAKE_MEMBER_GS(bool, close_warning, false);
            MAKE_MEMBER_GS(bool, close_auto_close_warning, false);
            MAKE_MEMBER_GS(float, close_delay, 0.0f);
            MAKE_MEMBER_GS(float, open_delay, 0.0f);
            MAKE_MEMBER_GS(float, open_with_permit, -1.0f);
            MAKE_MEMBER_GS(bool, has_lock, false);
            MAKE_MEMBER_GS(float, max_shift_plug, 0.0f);
            MAKE_MEMBER_GS_NO_DEF(Array, permit_list);
            MAKE_MEMBER_GS(int, permit_default, 1);
            MAKE_MEMBER_GS(bool, close_auto_close_remote, false);
            MAKE_MEMBER_GS(float, close_auto_close_velocity, -1.0f);
            MAKE_MEMBER_GS(double, platform_max_speed, 0.0);
            MAKE_MEMBER_GS(float, platform_max_shift, 0.0f);
            MAKE_MEMBER_GS(float, platform_speed, 0.5f);
            MAKE_MEMBER_GS(double, mirror_max_shift, 90.0);
            MAKE_MEMBER_GS(double, mirror_close_velocity, 5.0);
            MAKE_MEMBER_GS(bool, permit_required, false);
            MAKE_MEMBER_GS_NR(PermitLight, permit_light_blinking, PermitLight::PERMIT_LIGHT_CONTINUOUS);
            MAKE_MEMBER_GS_NR(PlatformType, platform_type, PlatformType::PLATFORM_TYPE_ROTATE);
            MAKE_MEMBER_GS_NR(Side, side, Side::SIDE_LEFT);
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleDoors::PermitLight)
VARIANT_ENUM_CAST(RailVehicleDoors::PlatformType)
VARIANT_ENUM_CAST(RailVehicleDoors::Side)
VARIANT_ENUM_CAST(RailVehicleDoors::Controls)
VARIANT_ENUM_CAST(RailVehicleDoors::Voltage)
VARIANT_ENUM_CAST(RailVehicleDoors::Type)
