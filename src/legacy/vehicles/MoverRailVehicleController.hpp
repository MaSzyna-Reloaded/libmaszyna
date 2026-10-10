#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "vehicles/rail/RailVehicleController.hpp"

namespace godot {
    /* VehicleController on the vendored Mover: the one class that owns a vehicle's
     * TMoverParameters. It builds it, steps it, answers from it and carries the original engine's
     * vehicle mechanics that act on it (the TDynamicObject parts of DynObj.cpp: cab, couplers,
     * neighbours). Movement along the track, transforms, tracks and wires are not here - they are
     * RailVehicleServer's. */
    class MoverRailVehicleController : public RailVehicleController {
            GDCLASS(MoverRailVehicleController, RailVehicleController)

        private:
            /* An EZT's automatic start thresholds (LoadFIZ_Param, Mover.cpp:10303-10304) */
            static constexpr int EZT_IMIN_LOW = 1;
            static constexpr int EZT_IMIN_HIGH = 2;
            /* attach_coupler_adapter(): the neighbour's reach and the room asked for fitting the
             * adapter [m] (DynObj.cpp:1760, 1774) */
            static constexpr double COUPLER_ADAPTER_REACH = 25.0;
            static constexpr double COUPLER_ADAPTER_ROOM = 0.5;
            /* This vehicle's Mover, which MaszynaMoverVehicleServer owns: taken in
             * _initialize_simulation(), handed back in release(). mover_vehicle is the handle it
             * was created for - the controller's own may change meanwhile. */
            TMoverParameters *mover = nullptr;
            RID mover_vehicle;
            /* The model of the adapter fitted to each end, "" without one */
            String fitted_adapter_models[2];
            /* An end fitted with its neighbour's adapter; p_with_room asks for room left for it */
            bool _fit_coupler_adapter(CouplerEnd p_end, bool p_with_room);
            /* The MaszynaMoverVehicleServer the Mover came from, by id: at shutdown it is freed -
             * and with it every Mover - before the controllers it served */
            ObjectID mover_implementation;
            /* That server, or null once it is gone - and the Mover with it */
            class MaszynaMoverVehicleServer *_mover_implementation() const;
            /* The controller a coupled Mover belongs to - coupled Movers only know each other
             * (TCoupling::Connected) */
            Ref<RailVehicleController> _controller_of(const TMoverParameters *p_mover) const;

            /* Which movement integration one sub-iteration performs. The original runs the cheap
             * one for every sub-iteration but the last (DynObj.cpp:4086). */
            enum class Integration {
                FAST,
                FULL,
            };

            void initialize_mover_state();
            static int cab_occupied(RailVehicleCabinKind::Kind p_kind);
            void _integrate(double p_delta, Integration p_integration);
            CouplerEnd _resolve_coupler_end(const Variant &p_where) const;
            /* The coupled vehicle as this end's neighbour; false when the end is not coupled */
            bool _neighbour_from_coupler(CouplerEnd p_end);
            void _consume_coupler_events();

        protected:
            static void _bind_methods();
            void _initialize_simulation() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

            Direction get_direction_absolute() const override;
            int get_train_damage() const override;
            double get_mass_reduced() const override;
            bool get_coupler_stretched() const override;

        public:
            MoverRailVehicleController();
            ~MoverRailVehicleController() override;

            /* C++ only and unbound: the Mover is this implementation's own business. */
            TMoverParameters *get_mover() const;

            void cab_activation(bool p_enabled) const override;
            void compartment_lights(bool p_enabled) const override;
            void compartment_lights_switch_off(bool p_enabled) const override;
            bool get_compartment_lights_enabled() const override;
            bool get_compartment_lights_active() const override;
            void cab_activation_auto() const override;
            void cab_deactivation_auto() const override;
            void cab_controls_reset() const override;
            void set_driver_cabin_kind(RailVehicleCabinKind::Kind p_kind) override;
            RailVehicleCabinKind::Kind get_active_cabin_kind() const override;
            void cabin_leave() const override;
            void cabin_enter() const override;
            void ground_relay_reset() const override;
            void antislip() const override;
            void main_controller_increase(int p_step = 1) const override;
            void main_controller_decrease(int p_step = 1) const override;
            void main_controller_set_position(int p_position) const override;
            void second_controller_increase(int p_step = 1) const override;
            void second_controller_decrease(int p_step = 1) const override;
            void direction_increase() const override;
            void direction_decrease() const override;

            bool is_simulation_ready() const override;
            void release() override;
            void update_state() override;
            double get_velocity() const override;
            double get_speed() const override;
            double get_acceleration() const override;
            double get_mass_total() const override;
            double get_total_distance() const override;
            Direction get_direction() const override;
            void apply_config() override;
            void apply_vehicle_config() override;
            double process_movement(double p_delta) override;
            void update_location() override;
            void update_neighbour(
                    CouplerEnd p_end, const Ref<RailVehicleController> &p_other, CouplerEnd p_other_end,
                    double p_track_distance) override;
            void clear_neighbour(CouplerEnd p_end) override;
            void compute_forces(double p_delta) override;
            void compute_movement(double p_delta) override;
            void compute_fast_movement(double p_delta) override;
            bool is_physics_active() const override;
            void wake() override;
            void
            couple(const Ref<RailVehicleController> &p_other, CouplerEnd p_end, CouplerEnd p_other_end,
                   BitField<CouplingFlags> p_coupling) override;
            void uncouple(CouplerEnd p_end) override;
            bool is_coupled(CouplerEnd p_end) const override;
            bool is_coupled_by(CouplerEnd p_end, BitField<CouplingFlags> p_flags) const override;
            void coupler_connect(const Variant &p_where) override;
            void coupler_disconnect(const Variant &p_where) override;
            bool coupler_adapter_attach(const Variant &p_where) override;
            bool coupler_adapter_fit(CouplerEnd p_end) override;
            bool is_coupler_automatic(CouplerEnd p_end) const override;
            BitField<CouplingFlags> get_coupler_joinable_flags(CouplerEnd p_end) const override;
            bool coupler_adapter_remove(const Variant &p_where) override;
            String get_coupler_adapter_fitted_model(CouplerEnd p_end) const override;
            double get_coupler_adapter_fitted_length(CouplerEnd p_end) const override;
            double get_coupler_adapter_fitted_height(CouplerEnd p_end) const override;
            Ref<RailVehicleController> get_coupled_controller(CouplerEnd p_end) const override;
            CouplerEnd get_coupled_end(CouplerEnd p_end) const override;
    };
} // namespace godot
