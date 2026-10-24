#pragma once
#include "MaszynaMoverDieselEngineUnit.hpp"
#include "MaszynaMoverDriveUnit.hpp"
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverDieselConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverEngineConfiguration.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"

namespace godot {
    /* RailVehicleDieselEngine on the vendored Mover. It owns no logic of its own - only the units
     * the engine is composed of, whose implementations other engine kinds share. */
    class MaszynaMoverRailVehicleDieselEngine : public RailVehicleDieselEngine, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleDieselEngine, RailVehicleDieselEngine);

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

        public:
            MaszynaMoverRailVehicleDieselEngine() {
                engine_configuration.instantiate();
                diesel_configuration.instantiate();
                drive_unit = &drive_unit_impl;
                diesel_engine_unit = &diesel_engine_unit_impl;
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
