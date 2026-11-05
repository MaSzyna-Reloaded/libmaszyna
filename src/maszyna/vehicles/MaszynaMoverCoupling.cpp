#include "MaszynaMoverCoupling.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"

namespace godot {
    namespace {
        /* A core's coupling flag and the Mover's coupling:: bit it stands for */
        struct CouplingFlagBit {
                RailVehicleController::CouplingFlags flag;
                int mover_flag;
        };

        /* The core's coupling flags and the Mover's coupling:: bits they stand for (MOVER.h:162-176) */
        constexpr CouplingFlagBit COUPLING_FLAGS[] = {
                {RailVehicleController::COUPLING_FLAG_COUPLER, coupling::coupler},
                {RailVehicleController::COUPLING_FLAG_BRAKEHOSE, coupling::brakehose},
                {RailVehicleController::COUPLING_FLAG_CONTROL, coupling::control},
                {RailVehicleController::COUPLING_FLAG_HIGHVOLTAGE, coupling::highvoltage},
                {RailVehicleController::COUPLING_FLAG_GANGWAY, coupling::gangway},
                {RailVehicleController::COUPLING_FLAG_MAINHOSE, coupling::mainhose},
                {RailVehicleController::COUPLING_FLAG_HEATING, coupling::heating},
                {RailVehicleController::COUPLING_FLAG_PERMANENT, coupling::permanent},
                {RailVehicleController::COUPLING_FLAG_POWER_24V, coupling::power24v},
                {RailVehicleController::COUPLING_FLAG_POWER_110V, coupling::power110v},
                {RailVehicleController::COUPLING_FLAG_POWER_3X400V, coupling::power3x400v},
        };
    } // namespace

    void MaszynaMoverCoupling::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaMoverCoupling", D_METHOD("flags_from_mover", "mover_flags"),
                &MaszynaMoverCoupling::flags_from_mover);
        ClassDB::bind_static_method(
                "MaszynaMoverCoupling", D_METHOD("flags_to_mover", "flags"), &MaszynaMoverCoupling::flags_to_mover);
        ClassDB::bind_static_method(
                "MaszynaMoverCoupling", D_METHOD("has_unknown_bits", "mover_flags"),
                &MaszynaMoverCoupling::has_unknown_bits);
    }

    int MaszynaMoverCoupling::flags_to_mover(const BitField<RailVehicleController::CouplingFlags> p_flags) {
        int mover_flags = coupling::faux;
        for (const auto &[flag, mover_flag]: COUPLING_FLAGS) {
            if (p_flags.has_flag(flag)) {
                mover_flags |= mover_flag;
            }
        }
        return mover_flags;
    }

    BitField<RailVehicleController::CouplingFlags> MaszynaMoverCoupling::flags_from_mover(const int p_mover_flags) {
        BitField<RailVehicleController::CouplingFlags> flags = RailVehicleController::COUPLING_FLAG_NONE;
        for (const auto &[flag, mover_flag]: COUPLING_FLAGS) {
            if ((p_mover_flags & mover_flag) != 0) {
                flags.set_flag(flag);
            }
        }
        return flags;
    }

    bool MaszynaMoverCoupling::has_unknown_bits(const int p_mover_flags) {
        return flags_to_mover(flags_from_mover(p_mover_flags)) != p_mover_flags;
    }

    int MaszynaMoverCoupling::end_to_mover(const RailVehicleController::CouplerEnd p_end) {
        return p_end == RailVehicleController::COUPLER_END_REAR ? end::rear : end::front;
    }

    RailVehicleController::CouplerEnd MaszynaMoverCoupling::end_from_mover(const int p_mover_end) {
        return p_mover_end == end::rear ? RailVehicleController::COUPLER_END_REAR
                                        : RailVehicleController::COUPLER_END_FRONT;
    }
} // namespace godot
