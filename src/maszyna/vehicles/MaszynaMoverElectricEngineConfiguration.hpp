#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"

#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* What the Mover's model of an electric engine is configured with: the circuit's resistance, the tuhex relay
     * settings and the converter overload relay's behaviour. Shared by the series and the induction engine adapters and
     * read by MaszynaMoverCircuitUnit. */
    class MaszynaMoverElectricEngineConfiguration : public Resource {
            GDCLASS(MaszynaMoverElectricEngineConfiguration, Resource);

        protected:
            static void _bind_methods();

        private:
            /* Circuit: (elektryczny obwod napedowy) - defaults of MOVER.h:1740-1744, 1762-1769 */
            double circuit_resistance = 0.0;
            double circuit_tuhex_sum = 750.0;
            double circuit_tuhex_diff = 10.0;
            double circuit_tuhex_min_current = 60.0;
            double circuit_tuhex_max_current = 400.0;
            int circuit_tuhex_stages = 0;
            double circuit_tuhex_sum_1 = 750.0;
            double circuit_tuhex_sum_2 = 750.0;
            double circuit_tuhex_sum_3 = 750.0;
            bool cntrl_converter_overload_relay_off_when_main_is_off = false;

        public:
            MAKE_GS(double, circuit_resistance);
            MAKE_GS(double, circuit_tuhex_sum);
            MAKE_GS(double, circuit_tuhex_diff);
            MAKE_GS(double, circuit_tuhex_min_current);
            MAKE_GS(double, circuit_tuhex_max_current);
            MAKE_GS(int, circuit_tuhex_stages);
            MAKE_GS(double, circuit_tuhex_sum_1);
            MAKE_GS(double, circuit_tuhex_sum_2);
            MAKE_GS(double, circuit_tuhex_sum_3);
            MAKE_GS(bool, cntrl_converter_overload_relay_off_when_main_is_off);
    };
} // namespace godot
