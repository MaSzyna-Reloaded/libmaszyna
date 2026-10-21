#pragma once
#include "macros.hpp"
#include "maszyna/vehicles/MaszynaMoverMotorParameter.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"

#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* What the Mover's model of any engine is configured with beyond the engine's facts: the motors' parameter table,
     * the blowers' speed and the EIM controller's and the series controller's settings (LoadFIZ_Engine, LoadFIZ_Cntrl).
     * Shared by every engine adapter and read by MaszynaMoverDriveUnit. */
    class MaszynaMoverEngineConfiguration : public Resource {
            GDCLASS(MaszynaMoverEngineConfiguration, Resource);

        public:
            /* EIMCtrlType= : traction lever variant, for vehicles with an EIM-style controller */
            enum EimControlType {
                EIM_CONTROL_TYPE_0,
                EIM_CONTROL_TYPE_1,
                EIM_CONTROL_TYPE_2,
                EIM_CONTROL_TYPE_3,
            };

        protected:
            static void _bind_methods();

        private:
            /* Cntrl. (the fields of the master controller and the starter every engine has) */
            bool cntrl_eim_control_additional_zeros = false;
            bool cntrl_eim_control_emergency = false;
            EimControlType cntrl_eim_control_type = EIM_CONTROL_TYPE_0;
            bool cntrl_series_shunt_on_series_position = false;
            bool cntrl_fast_series_circuit = false;
            double motor_blowers_speed = 0.0;

        public:
            MAKE_GS(bool, cntrl_eim_control_additional_zeros);
            MAKE_GS(bool, cntrl_eim_control_emergency);
            MAKE_GS_NR(EimControlType, cntrl_eim_control_type);
            MAKE_GS(bool, cntrl_series_shunt_on_series_position);
            MAKE_GS(bool, cntrl_fast_series_circuit);
            MAKE_GS(double, motor_blowers_speed);

        private:
            TypedArray<MaszynaMoverMotorParameter> motor_param_table;

        public:
            TypedArray<MaszynaMoverMotorParameter> get_motor_param_table() const {
                return motor_param_table;
            }

            void set_motor_param_table(const TypedArray<MaszynaMoverMotorParameter> &p_motor_param_table) {
                motor_param_table.clear();
                motor_param_table.append_array(p_motor_param_table);
            }
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaMoverEngineConfiguration::EimControlType);
