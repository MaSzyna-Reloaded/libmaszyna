#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"

namespace godot {
    /* RailVehicleLoad on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleLoad : public RailVehicleLoad, public MoverComponent {
            GDCLASS(MoverRailVehicleLoad, RailVehicleLoad);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

            TypedArray<RailVehicleLoadListItem> load_list;

            /// Below this much the exchange counts as done (DynObj.cpp:2877)
            static constexpr double EXCHANGE_DONE = 0.01;
            /// Faster than this [km/h] the vehicle exchanges nothing and gives the exchange up
            /// (DynObj.cpp:2879, 2932)
            static constexpr double EXCHANGE_MAX_SPEED = 2.0;
            /// The exchange goes on in whole seconds (DynObj.cpp:2903)
            static constexpr double EXCHANGE_STEP = 1.0;
            /// How likely the passengers close doors they control, and mixed ones, once done
            /// (DynObj.cpp:2945-2948)
            static constexpr double PASSENGER_DOORS_CLOSE_CHANCE = 0.75;
            static constexpr double MIXED_DOORS_CLOSE_CHANCE = 0.50;
            /// TMoverParameters::LoadStatus: unloading, loading, done (MOVER.h:1762)
            static constexpr int LOAD_STATUS_UNLOADING = 1;
            static constexpr int LOAD_STATUS_LOADING = 2;
            static constexpr int LOAD_STATUS_DONE = 4;

            /// TDynamicObject::m_exchange (DynObj.h:345-350): still to get off, still to get on, at
            /// which side, and the time of the second being exchanged
            double exchange_unload = 0.0;
            double exchange_load = 0.0;
            PlatformSide exchange_side = PLATFORM_SIDE_BOTH;
            double exchange_time = 0.0;

            bool _exchange_serves(PlatformSide p_side) const;

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            void load_add(double p_amount, PlatformSide p_side, const String &p_load_name) override;
            void load_remove(double p_amount, PlatformSide p_side) override;
            double get_load_exchange_time() const override;
            int get_load_exchange_speed() const override;
            String get_load_name() const override;
            double get_load_amount() const override;

            void set_load_list(const TypedArray<RailVehicleLoadListItem> &p_load_list) override {
                load_list.clear();
                load_list.append_array(p_load_list);
            }
            TypedArray<RailVehicleLoadListItem> get_load_list() override {
                return load_list;
            }
    };
} // namespace godot
