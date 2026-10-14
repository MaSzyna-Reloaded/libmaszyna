#include "MoverRailVehicleBuffCoupl.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"

namespace godot {
    namespace {
        /* The data's own values of a coupler without a FIZ entry of its own (LoadFIZ_BuffCoupl,
         * Mover.cpp:10663-10692) */
        constexpr double COUPLER_DMAX = 0.05;
        constexpr double BARE_COUPLER_SPRING_KC_PER_MASS = 50.0;
        constexpr double BARE_COUPLER_FMAXC_PER_MASS = 100.0;
        constexpr double BARE_COUPLER_SPRING_KB_PER_MASS = 60.0;
        constexpr double BARE_COUPLER_FMAXB_PER_MASS = 50.0;
        constexpr double BARE_COUPLER_FMAX_PER_FORCE = 2.0;
        constexpr double BARE_COUPLER_BETA = 0.3;
        constexpr double ARTICULATED_COUPLER_SPRING_KC = 4500.0;
        constexpr double ARTICULATED_COUPLER_FMAXC = 850.0;
        constexpr double ARTICULATED_COUPLER_SPRING_KB = 9200.0;
        constexpr double ARTICULATED_COUPLER_FMAXB = 320.0;
        constexpr double ARTICULATED_COUPLER_BETA = 0.55;
    } // namespace

    void MoverRailVehicleBuffCoupl::_bind_methods() {}

    /* The coupling flags of one end, read straight from the backend - this class is the only one
     * that may (Mover.h: TCoupling, coupling::). */
    bool MoverRailVehicleBuffCoupl::is_coupled(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && (mover->Couplers[p_end].CouplingFlag & coupling::coupler) != 0;
    }

    bool MoverRailVehicleBuffCoupl::is_brake_hose_connected(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && (mover->Couplers[p_end].CouplingFlag & coupling::brakehose) != 0;
    }

    bool MoverRailVehicleBuffCoupl::is_main_hose_connected(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && (mover->Couplers[p_end].CouplingFlag & coupling::mainhose) != 0;
    }

    bool MoverRailVehicleBuffCoupl::is_coupling_owner(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Couplers[p_end].Render;
    }

    RailVehicleController::CouplerEnd
    MoverRailVehicleBuffCoupl::get_connected_end(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? static_cast<RailVehicleController::CouplerEnd>(mover->Couplers[p_end].ConnectedNr)
                                : RailVehicleController::COUPLER_END_FRONT;
    }

    double MoverRailVehicleBuffCoupl::get_coupler_max_force(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Couplers[p_end].FmaxC : 0.0;
    }


    TCoupling *MoverRailVehicleBuffCoupl::_get_coupling(TMoverParameters *p_mover) const {
        // LoadFIZ_BuffCoupl (Mover.cpp:10619): BuffCoupl2. -> rear coupler, BuffCoupl./BuffCoupl1. -> front
        return &p_mover->Couplers
                        [get_buffer_location() == BufferLocation::BUFFER_LOCATION_BACK ? end::rear : end::front];
    }

    void MoverRailVehicleBuffCoupl::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        TCoupling *coupler = _get_coupling(p_mover);
        std::map<CouplerType, TCouplerType> coupler_types{
                {COUPLER_TYPE_AUTOMATIC, TCouplerType::Automatic},
                {COUPLER_TYPE_SCREW, TCouplerType::Screw},
                {COUPLER_TYPE_CHAIN, TCouplerType::Chain},
                {COUPLER_TYPE_BARE, TCouplerType::Bare},
                {COUPLER_TYPE_ARTICULATED, TCouplerType::Articulated},
        };

        const std::map<CouplerType, TCouplerType>::iterator lookup = coupler_types.find(get_coupler_type());
        const TCouplerType resolved_type = lookup != coupler_types.end() ? lookup->second : TCouplerType::NoCoupler;

        coupler->CouplerType = resolved_type;
        coupler->SpringKC = get_coupler_stiffness_k();
        coupler->DmaxC = get_coupler_max_compression_tolerance();
        coupler->FmaxC = get_coupler_max_tension_tolerance();
        coupler->SpringKB = get_buffer_stiffness_k();
        coupler->DmaxB = get_buffer_max_compression_tolerance();
        coupler->FmaxB = get_buffer_max_tension_tolerance();
        coupler->beta = get_damping_beta();
        coupler->AutomaticCouplingFlag = get_automatic_flag();
        // the FIZ's negative AllowedFlag is made a permanent one by its parser (fiz_train_buff_coupl_parser.gd)
        coupler->AllowedFlag = get_allowed_flag();

        coupler->PowerCoupling = get_power_coupling();
        coupler->PowerFlag = get_power_flag();
        coupler->control_type = get_control_type().ascii().get_data();

        if (coupler->CouplerType != TCouplerType::NoCoupler && coupler->CouplerType != TCouplerType::Bare &&
            coupler->CouplerType != TCouplerType::Articulated) {

            coupler->SpringKC *= LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->FmaxC *= LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->SpringKB *= LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->FmaxB *= LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
        } else if (coupler->CouplerType == TCouplerType::Articulated) {
            coupler->SpringKC = ARTICULATED_COUPLER_SPRING_KC * LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->DmaxC = COUPLER_DMAX;
            coupler->FmaxC = ARTICULATED_COUPLER_FMAXC * LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->SpringKB = ARTICULATED_COUPLER_SPRING_KB * LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->DmaxB = COUPLER_DMAX;
            coupler->FmaxB = ARTICULATED_COUPLER_FMAXB * LibMaszynaUnits::NEWTONS_PER_KILONEWTON;
            coupler->beta = ARTICULATED_COUPLER_BETA;
        }

        if (get_buffer_location() == BufferLocation::BUFFER_LOCATION_BOTH) {
            // single entry for both couplers (Mover.cpp:10694-10698); copy only the configuration, never the
            // runtime connection state (Connected, CouplingFlag, ...) of an already coupled vehicle
            TCoupling &rear = p_mover->Couplers[end::rear];
            rear.CouplerType = coupler->CouplerType;
            rear.SpringKC = coupler->SpringKC;
            rear.DmaxC = coupler->DmaxC;
            rear.FmaxC = coupler->FmaxC;
            rear.SpringKB = coupler->SpringKB;
            rear.DmaxB = coupler->DmaxB;
            rear.FmaxB = coupler->FmaxB;
            rear.beta = coupler->beta;
            rear.AutomaticCouplingFlag = coupler->AutomaticCouplingFlag;
            rear.AllowedFlag = coupler->AllowedFlag;
            rear.PowerCoupling = coupler->PowerCoupling;
            rear.PowerFlag = coupler->PowerFlag;
            rear.control_type = coupler->control_type;
        }
    }

    /* A bare coupler is sized by the vehicle's mass and its maximum tractive force
     * (LoadFIZ_BuffCoupl, Mover.cpp:10663-10672). The original reads Ftmax as far as its FIZ
     * has been parsed, which is 0 before an Engine: section that comes after BuffCoupl. (see
     * FINDINGS.md); the engine's property is the force the formula means. */
    void MoverRailVehicleBuffCoupl::apply_vehicle_config() {
        if (get_coupler_type() != COUPLER_TYPE_BARE) {
            return;
        }
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        const double mass = train_controller_node->get_mass();
        const Ref<RailVehicleEngine> engine =
                train_controller_node->get_component(VehicleComponentType::COMPONENT_ENGINE);
        const double traction_force = engine.is_valid() ? engine->get_maximum_traction_force() : 0.0;
        const auto size_bare_coupler = [mass, traction_force](TCoupling &p_coupler) {
            p_coupler.SpringKC = (BARE_COUPLER_SPRING_KC_PER_MASS * mass) + (traction_force / COUPLER_DMAX);
            p_coupler.DmaxC = COUPLER_DMAX;
            p_coupler.FmaxC = (BARE_COUPLER_FMAXC_PER_MASS * mass) + (BARE_COUPLER_FMAX_PER_FORCE * traction_force);
            p_coupler.SpringKB = (BARE_COUPLER_SPRING_KB_PER_MASS * mass) + (traction_force / COUPLER_DMAX);
            p_coupler.DmaxB = COUPLER_DMAX;
            p_coupler.FmaxB = (BARE_COUPLER_FMAXB_PER_MASS * mass) + (BARE_COUPLER_FMAX_PER_FORCE * traction_force);
            p_coupler.beta = BARE_COUPLER_BETA;
        };
        size_bare_coupler(*_get_coupling(p_mover));
        if (get_buffer_location() == BufferLocation::BUFFER_LOCATION_BOTH) {
            size_bare_coupler(p_mover->Couplers[end::rear]);
        }
    }


    /* How far each coupler is stretched (+) or its buffers pressed (-) [m], and the force it passes
     * [N] (TCoupling::Dist, CForce) - what decides whether it breaks (Mover.cpp:4843-4857) */
    void MoverRailVehicleBuffCoupl::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleBuffCoupl::_fill_state_dictionary(p_state);
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        // each coupler part publishes its own end; a single BuffCoupl. entry is both
        if (get_buffer_location() != BufferLocation::BUFFER_LOCATION_BACK) {
            p_state["coupler_front_distance"] = mover->Couplers[end::front].Dist;
            p_state["coupler_front_force"] = mover->Couplers[end::front].CForce;
        }
        if (get_buffer_location() != BufferLocation::BUFFER_LOCATION_FRONT) {
            p_state["coupler_rear_distance"] = mover->Couplers[end::rear].Dist;
            p_state["coupler_rear_force"] = mover->Couplers[end::rear].CForce;
        }
    }

    void MoverRailVehicleBuffCoupl::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        // what the Mover made of it: each coupler's strength [N], front and rear (FmaxC)
        p_config["coupler_max_force"] = PackedFloat64Array(
                {get_coupler_max_force(RailVehicleController::COUPLER_END_FRONT),
                 get_coupler_max_force(RailVehicleController::COUPLER_END_REAR)});
    }
} // namespace godot
