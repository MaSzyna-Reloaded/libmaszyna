#pragma once
#include "MaszynaMoverDieselEngineUnit.hpp"
#include "MaszynaMoverDriveUnit.hpp"
#include "MaszynaMoverTractionMotorsUnit.hpp"
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverDieselConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverWWListItem.hpp"
#include "vehicles/rail/RailVehicleDieselElectricEngine.hpp"

namespace godot {
    /* RailVehicleDieselElectricEngine on the vendored Mover. It owns the units the engine is
     * composed of, and writes the diesel-electric engine's own
     * configuration into the Mover, which none of the units covers. */
    class MaszynaMoverRailVehicleDieselElectricEngine : public RailVehicleDieselElectricEngine,
                                                        public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleDieselElectricEngine, RailVehicleDieselElectricEngine);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();
            /* What the Mover's model of the engine is configured with beyond its facts. The units read it */
            Ref<MaszynaMoverEngineConfiguration> engine_configuration;
            /* What the Mover's model of the diesel engine is configured with. The units read it */
            Ref<MaszynaMoverDieselConfiguration> diesel_configuration;
            MaszynaMoverDriveUnit drive_unit_impl{*this, engine_configuration};
            MaszynaMoverDieselEngineUnit diesel_engine_unit_impl{*this, diesel_configuration};
            MaszynaMoverTractionMotorsUnit traction_motors_unit_impl{*this};


        public:
            MaszynaMoverRailVehicleDieselElectricEngine() {
                engine_configuration.instantiate();
                diesel_configuration.instantiate();
                traction_motors_unit = &traction_motors_unit_impl;
                drive_unit = &drive_unit_impl;
                diesel_engine_unit = &diesel_engine_unit_impl;
            }


        protected:
            /// Mover.cpp:8551-8552 (readWWList) - the shunting power bounds of a WWList row
            static constexpr double WWLIST_SHUNT_POWER_DIVISOR = 47.6;

            void _apply_configuration() override;

        private:
            /* Engine: (Kont.), przekladnia elektryczna */
            bool generator_voltage_flat = false;
            double hyperbolic_speed = 1.0;
            double additional_speed = 1.0;
            double power_correction_ratio = 1.0;
            int shunt_relay_type = 0;
            TypedArray<MaszynaMoverWWListItem> wwlist;

        public:
            MAKE_GS(bool, generator_voltage_flat);
            MAKE_GS(double, hyperbolic_speed);
            MAKE_GS(double, additional_speed);
            MAKE_GS(double, power_correction_ratio);
            MAKE_GS(int, shunt_relay_type);
            TypedArray<MaszynaMoverWWListItem> get_wwlist() {
                return wwlist;
            }

            void set_wwlist(const TypedArray<MaszynaMoverWWListItem> &p_wwlist) {
                wwlist.clear();
                wwlist.append_array(p_wwlist);
            }
            Ref<MaszynaMoverEngineConfiguration> get_engine_configuration() const;
            void set_engine_configuration(const Ref<MaszynaMoverEngineConfiguration> &p_value);
            /* The Mover's model of the engine, read by the sounds and the AI (#316) */
            double get_circuit_nmax_rpm() const;
            double get_eimic_real() const;
            double get_clutch_desired() const;
            Ref<MaszynaMoverDieselConfiguration> get_diesel_configuration() const;
            void set_diesel_configuration(const Ref<MaszynaMoverDieselConfiguration> &p_value);
    };
} // namespace godot
