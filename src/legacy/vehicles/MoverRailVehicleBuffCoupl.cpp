#include "MoverRailVehicleBuffCoupl.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"

namespace godot {
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


    void MoverRailVehicleBuffCoupl::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        // LoadFIZ_BuffCoupl (Mover.cpp:10297): BuffCoupl2. -> rear coupler, BuffCoupl./BuffCoupl1. -> front
        TCoupling *coupler;
        if (get_buffer_location() == BufferLocation::BUFFER_LOCATION_BACK) {
            coupler = &p_mover->Couplers[end::rear];
        } else {
            coupler = &p_mover->Couplers[end::front];
        }
        const double mass = train_controller_node->get_mass();
        const double max_velocity = train_controller_node->get_max_velocity();
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

            coupler->SpringKC *= 1000;
            coupler->FmaxC *= 1000;
            coupler->SpringKB *= 1000;
            coupler->FmaxB *= 1000;
        } else if (coupler->CouplerType == TCouplerType::Bare) {
            coupler->SpringKC = (50.0 * mass) + (max_velocity / 0.05);
            coupler->DmaxC = 0.05;
            coupler->FmaxC = (100.0 * mass) + (2 * max_velocity);
            coupler->SpringKB = (60.0 * mass) + (max_velocity / 0.05);
            coupler->DmaxB = 0.05;
            coupler->FmaxB = (50.0 * mass) + (2.0 * max_velocity);
            coupler->beta = 0.3;
        } else if (coupler->CouplerType == TCouplerType::Articulated) {
            coupler->SpringKC = 4500 * 1000;
            coupler->DmaxC = 0.05;
            coupler->FmaxC = 850 * 1000;
            coupler->SpringKB = 9200 * 1000;
            coupler->DmaxB = 0.05;
            coupler->FmaxB = 320 * 1000;
            coupler->beta = 0.55;
        }

        if (get_buffer_location() == BufferLocation::BUFFER_LOCATION_BOTH) {
            // single entry for both couplers (Mover.cpp:10370); copy only the configuration, never the
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
