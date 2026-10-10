#pragma once
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleCabinKind.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"

namespace godot {
    /// A railway vehicle: what VehicleController says of every vehicle, plus the railway's own -
    /// its couplers and trainset, the occupied cab and the reverser, and the stepping
    /// RailVehicleServer runs it by along its track. Everything a vehicle may or may not have - the
    /// master controller, the engine, the low voltage, the radio - is a railway component
    /// (RailVehicleComponent), and fills its own state.
    class RailVehicleController : public VehicleController {
            GDCLASS(RailVehicleController, VehicleController)

        public:
            /* The original's own coupler adapter, for a vehicle whose MMD has no coupleradapter:
             * (DynObj.cpp:1765-1769) */
            static constexpr const char *DEFAULT_COUPLER_ADAPTER_MODEL = "tabor/polsprzeg";
            static constexpr double DEFAULT_COUPLER_ADAPTER_LENGTH = 0.085;
            static constexpr double DEFAULT_COUPLER_ADAPTER_HEIGHT = 0.95;

        private:
            RailVehicleCabinKind::Kind driver_cabin_kind = RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
            String coupler_adapter_model = DEFAULT_COUPLER_ADAPTER_MODEL;
            double coupler_adapter_length = DEFAULT_COUPLER_ADAPTER_LENGTH;
            double coupler_adapter_height = DEFAULT_COUPLER_ADAPTER_HEIGHT;

        protected:
            static void _bind_methods();
            void _register_commands() override;
            void _unregister_commands() override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        public:
            /* Live state, read straight from the backend - nothing is stored. Public because it
             * is: every one of these is bound for GDScript, and a reader in C++ - the node that
             * draws the vehicle, a component of another kind - has the same right to it as a
             * script has. */
            virtual Direction get_direction_absolute() const = 0;
            virtual int get_train_damage() const = 0;
            /* The rotating masses' share of the vehicle's mass [kg] (Mred) - as the simulation
             * holds it, which the wheels may have derived from their own inertia */
            virtual double get_mass_reduced() const = 0;
            /* A coupler pulled past its strength (stretch_duration > 0, Mover.cpp:5405) */
            virtual bool get_coupler_stretched() const = 0;
            /* The passengers' compartment lights switched on, and lit (CompartmentLights.is_enabled,
             * is_active) */
            virtual bool get_compartment_lights_enabled() const = 0;
            virtual bool get_compartment_lights_active() const = 0;

            /* shared enum for every FIZ "...Start=" device activation mode field (Cntrl. section) */
            enum StartMode {
                START_MODE_DISABLED,
                START_MODE_MANUAL,
                START_MODE_AUTOMATIC,
                START_MODE_MANUAL_WITH_AUTO_FALLBACK,
                START_MODE_CONVERTER,
                START_MODE_BATTERY,
                START_MODE_DIRECTION,
            };

            /* The ends of a vehicle, as the original numbers them (end::front, end::rear, MOVER.h) */
            enum CouplerEnd {
                COUPLER_END_FRONT = 0,
                COUPLER_END_REAR = 1,
            };

            /* What joins two coupled vehicles, as the original names it; the flags combine into a
             * coupling (enum coupling, MOVER.h:162). Permanent marks the couplings inside one unit. */
            enum CouplingFlags {
                COUPLING_FLAG_NONE = 0x0,
                COUPLING_FLAG_COUPLER = 0x1,
                COUPLING_FLAG_BRAKEHOSE = 0x2,
                COUPLING_FLAG_CONTROL = 0x4,
                COUPLING_FLAG_HIGHVOLTAGE = 0x8,
                COUPLING_FLAG_GANGWAY = 0x10,
                COUPLING_FLAG_MAINHOSE = 0x20,
                COUPLING_FLAG_HEATING = 0x40,
                COUPLING_FLAG_PERMANENT = 0x80,
                COUPLING_FLAG_POWER_24V = 0x100,
                COUPLING_FLAG_POWER_110V = 0x200,
                COUPLING_FLAG_POWER_3X400V = 0x400,
            };

            /* The other end of the same vehicle - what a walk along a trainset leaves a vehicle by */
            static CouplerEnd opposite_end(CouplerEnd p_end);

            /* Type= : bitmask identifying a vehicle's special-cased behavior family */
            enum TrainType {
                TRAIN_TYPE_DEFAULT = 0,
                TRAIN_TYPE_EZT = 1,
                TRAIN_TYPE_ET41 = 2,
                TRAIN_TYPE_ET42 = 4,
                TRAIN_TYPE_PSEUDODIESEL = 8,
                TRAIN_TYPE_ET22 = 0x10,
                TRAIN_TYPE_SN61 = 0x20,
                TRAIN_TYPE_EP05 = 0x40,
                TRAIN_TYPE_ET40 = 0x80,
                TRAIN_TYPE_181 = 0x100,
                TRAIN_TYPE_DMU = 0x200,
            };

            enum TrainPowerSource {
                POWER_SOURCE_NOT_DEFINED,
                POWER_SOURCE_INTERNAL,
                POWER_SOURCE_TRANSDUCER,
                POWER_SOURCE_GENERATOR,
                POWER_SOURCE_ACCUMULATOR,
                POWER_SOURCE_CURRENTCOLLECTOR,
                POWER_SOURCE_POWERCABLE,
                POWER_SOURCE_HEATER,
                POWER_SOURCE_MAIN
            };

            enum TrainPowerType {
                POWER_TYPE_NONE,
                POWER_TYPE_BIO,
                POWER_TYPE_MECH,
                POWER_TYPE_ELECTRIC,
                POWER_TYPE_STEAM
            };

            /// The trainset this vehicle belongs to gained or lost a vehicle
            static const char *trainset_changed_signal;
            /// One coupling flag attached / detached, once per event. Two signals rather than
            /// one carrying a direction: every listener would have opened by branching on it.
            static const char *coupler_attached_signal;
            static const char *coupler_detached_signal;
            /// A coupler adapter was fitted to / taken off an end (TDynamicObject::attach_coupler_adapter(),
            /// remove_coupler_adapter(), DynObj.cpp:1754-1810)
            static const char *coupler_adapter_attached_signal;
            static const char *coupler_adapter_removed_signal;

            virtual void cab_activation(bool p_enabled) const = 0;
            /* The compartment lights switched on, or their "off" switch (CompartmentLightsSwitch(),
             * CompartmentLightsSwitchOff()) */
            virtual void compartment_lights(bool p_enabled) const = 0;
            virtual void compartment_lights_switch_off(bool p_enabled) const = 0;
            virtual void cab_activation_auto() const = 0;
            /* The cab switched off as the vehicle switches it off by itself, when the FIZ lets it
             * (CabDeactivisationAuto(), Train.cpp:10336) */
            virtual void cab_deactivation_auto() const = 0;
            /* The controls a cab change leaves at rest - the brake handle at its neutral, the
             * controllers at zero (TMoverParameters::ChangeCab(), Mover.cpp:735-749) */
            virtual void cab_controls_reset() const = 0;
            /* The kind of cabin its driver sits in, handed down by RailVehicleServer whenever it
             * changes - C++ only and unbound: who sits where is VehicleServer's to tell */
            virtual void set_driver_cabin_kind(RailVehicleCabinKind::Kind p_kind);
            RailVehicleCabinKind::Kind get_driver_cabin_kind() const;
            /* The kind of cabin whose cab is switched on (CabActive), NONE with none - C++ only:
             * RailVehicleServer reads it to choose between the drivers of one vehicle */
            virtual RailVehicleCabinKind::Kind get_active_cabin_kind() const = 0;
            /* Its driver went over to another vehicle: the cab switched off, the brake handle at its
             * neutral, the controllers at zero (TTrain::MoveToVehicle(), Train.cpp:10950-10954) -
             * command cabin_leave, sent by RailVehicleServer::person_change_cabin() */
            virtual void cabin_leave() const = 0;
            /* A driver came over from another vehicle: the pipe pressure limit taken from the pipe,
             * the cab switched on (Train.cpp:10976-10977) - command cabin_enter, as cabin_leave() */
            virtual void cabin_enter() const = 0;
            /* The main circuit's ground relay reset (maincircuitgroundreset, RelayReset(), Mover.cpp:6653) */
            virtual void ground_relay_reset() const = 0;
            /* The anti-slip brake pressed (antislip, AntiSlippingButton()) */
            virtual void antislip() const = 0;
            virtual void main_controller_increase(int p_step = 1) const = 0;
            virtual void main_controller_decrease(int p_step = 1) const = 0;
            /* The master controller put at a position at once, as the original's driver sets
             * MainCtrlPos of an EIM controller (Driver.cpp:3771-3816, 4284-4290): no step, so the
             * relay time a step restarts goes on (CheckEIMIC(), Mover.cpp) */
            virtual void main_controller_set_position(int p_position) const = 0;
            virtual void second_controller_increase(int p_step = 1) const = 0;
            virtual void second_controller_decrease(int p_step = 1) const = 0;
            virtual void direction_increase() const = 0;
            virtual void direction_decrease() const = 0;
            virtual double process_movement(double p_delta) = 0;
            virtual void update_location() = 0;
            /* The vehicle nearest beyond p_end, p_track_distance [m] center to center along the
             * track, facing it with p_other_end */
            virtual void update_neighbour(
                    CouplerEnd p_end, const Ref<RailVehicleController> &p_other, CouplerEnd p_other_end,
                    double p_track_distance) = 0;
            /* Nothing beyond p_end within the scan range */
            virtual void clear_neighbour(CouplerEnd p_end) = 0;
            virtual void compute_forces(double p_delta) = 0;
            virtual void compute_movement(double p_delta) = 0;
            virtual void compute_fast_movement(double p_delta) = 0;
            virtual void
            couple(const Ref<RailVehicleController> &p_other, CouplerEnd p_end, CouplerEnd p_other_end,
                   BitField<CouplingFlags> p_coupling) = 0;
            virtual void uncouple(CouplerEnd p_end) = 0;
            virtual bool is_coupled(CouplerEnd p_end) const = 0;
            /* Whether this end is joined by every one of p_flags (TestFlag(Couplers[end].CouplingFlag, ...)) */
            virtual bool is_coupled_by(CouplerEnd p_end, BitField<CouplingFlags> p_flags) const = 0;
            virtual void coupler_connect(const Variant &p_where) = 0;
            virtual void coupler_disconnect(const Variant &p_where) = 0;
            /* The adapter of the neighbour beyond an end (its coupleradapter:, or the original's own
             * when it has none) fitted to that end, with room left for it (attach_coupler_adapter(),
             * DynObj.cpp:1754-1789) - p_where as coupler_connect()'s */
            virtual bool coupler_adapter_attach(const Variant &p_where) = 0;
            /* The adapter of the vehicle coupled at an end fitted to it, whatever the room - a
             * trainset the scenery couples (AttachNext(), DynObj.cpp:2740-2758) */
            virtual bool coupler_adapter_fit(CouplerEnd p_end) = 0;
            /* Whether an end couples as an automatic coupler, its adapter's or its own
             * (TCoupling::type(), MOVER.h:800) */
            virtual bool is_coupler_automatic(CouplerEnd p_end) const = 0;
            /* The couplings this end and the neighbour beyond it can join: what both couplers allow, the
             * control line only between equal control types; none without a neighbour */
            virtual BitField<CouplingFlags> get_coupler_joinable_flags(CouplerEnd p_end) const = 0;
            /* The adapter taken off an end, uncoupling it first (DynObj.cpp:1791-1810) */
            virtual bool coupler_adapter_remove(const Variant &p_where) = 0;
            /* The model of the adapter fitted to an end, "" without one */
            virtual String get_coupler_adapter_fitted_model(CouplerEnd p_end) const = 0;
            /* The fitted adapter's length added to the end and its height over the rail [m] */
            virtual double get_coupler_adapter_fitted_length(CouplerEnd p_end) const = 0;
            virtual double get_coupler_adapter_fitted_height(CouplerEnd p_end) const = 0;

            /* coupleradapter: of the MMD - the adapter this vehicle hands a neighbour of another
             * coupler type: its model, its length and its height over the rail [m]; the original's
             * own without the key (DynObj.cpp:1765-1773, 5284-5292) */
            void set_coupler_adapter_model(const String &p_value);
            String get_coupler_adapter_model() const;
            void set_coupler_adapter_length(double p_value);
            double get_coupler_adapter_length() const;
            void set_coupler_adapter_height(double p_value);
            double get_coupler_adapter_height() const;
            virtual Ref<RailVehicleController> get_coupled_controller(CouplerEnd p_end) const = 0;
            /* Wakes the simulation the vehicle switched off while it stood with nothing to do -
             * somebody took it (RailVehicleServer::vehicle_wake()) */
            virtual void wake() = 0;
            /* The end of the coupled vehicle facing this one (TCoupling::ConnectedNr); only while
             * is_coupled(p_end) */
            virtual CouplerEnd get_coupled_end(CouplerEnd p_end) const = 0;
            /* The railway component of a kind, or null when this vehicle has none; every one of a
             * kind - a vehicle has two couplers, one per end */
            Ref<VehicleComponent> get_rail_component(RailVehicleComponentType::Type p_type) const;
            TypedArray<VehicleComponent> find_rail_components(RailVehicleComponentType::Type p_type) const;
            /* The name of the vehicle's type - the original's CHK/MMD name TMoverParameters keeps
             * as TypeName (DynObj.cpp:2019) */
            MAKE_MEMBER_GS(String, type_name, "");
            /* What the vehicle carries when the scenery places it, as the `.scn` names it - the
             * amount and the cargo's own name (`loadcount` and `loadtype` of a `dynamic`). The
             * simulation takes both at once, and it reads more than cargo out of them: `pantstate`
             * is how a scenery starts a locomotive with its pantographs already up. */
            MAKE_MEMBER_GS(String, load_name, "");
            MAKE_MEMBER_GS(double, load_amount, 0.0);
            MAKE_MEMBER_GS_NR(TrainType, train_type, TRAIN_TYPE_DEFAULT);
            MAKE_MEMBER_GS(double, reduced_mass, 0.0);
            MAKE_MEMBER_GS(double, sand_capacity, 0.0);
            MAKE_MEMBER_GS(double, heating_power, 0.0);
            MAKE_MEMBER_GS(double, light_power, 0.0);

            /* Cntrl. (ogolne, przekaznik ziemnozwarciowy/oswietlenie przedzialow/aktywacja kabiny) */
            MAKE_MEMBER_GS_NR(StartMode, cntrl_ground_relay_start_mode, START_MODE_MANUAL);
            /* CompartmentLightsStart= absent: automatic, "legacy behaviour" (Mover.cpp:10984) */
            MAKE_MEMBER_GS_NR(StartMode, cntrl_compartment_lights_start_mode, START_MODE_AUTOMATIC);
            MAKE_MEMBER_GS(bool, cntrl_automatic_cab_activation, true);
            MAKE_MEMBER_GS(int, cntrl_inactive_cab_flag, 0);
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleController::TrainPowerSource);
VARIANT_ENUM_CAST(RailVehicleController::TrainPowerType);
VARIANT_ENUM_CAST(RailVehicleController::CouplerEnd);
VARIANT_BITFIELD_CAST(RailVehicleController::CouplingFlags);
VARIANT_ENUM_CAST(RailVehicleController::TrainType);
VARIANT_ENUM_CAST(RailVehicleController::StartMode);
