#include "MoverRailVehicleLoad.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleLoad::_bind_methods() {}


    void MoverRailVehicleLoad::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->MaxLoad = get_max_load();
        // Build LoadAttributes from get_accepted_loads() with optional per-load minimum offset
        const int loads_count = static_cast<int>(get_accepted_loads().size());
        const int offsets_count = static_cast<int>(get_minimum_load_offsets().size());
        p_mover->LoadAttributes.clear();
        for (int i = 0; i < loads_count; ++i) {
            String load_str = get_accepted_loads()[i]; // TypedArray<String> element
            float min_offset = 0.f;
            if (i < offsets_count) {
                // TypedArray<float> element may be represented as real Variant (double)
                min_offset = static_cast<float>(static_cast<double>(get_minimum_load_offsets()[i]));
            }
            p_mover->LoadAttributes.emplace_back(std::string(load_str.utf8().get_data()), min_offset);
        }
        p_mover->LoadQuantity = get_load_unit() == LOAD_UNIT_TONS ? "tons" : "pieces";
        p_mover->LoadSpeed = get_load_speed();
        p_mover->UnLoadSpeed = get_unload_speed();
        p_mover->OverLoadFactor = static_cast<float>(get_overload_factor());
        VehicleComponent::_apply_configuration();
    }


    void MoverRailVehicleLoad::_fill_state_dictionary(Dictionary &p_state) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_state["load_name"] = get_load_name();
        p_state["load_amount"] = get_load_amount();
        p_state["load_max"] = mover->MaxLoad;
        p_state["load_exchange_unload"] = exchange_unload;
        p_state["load_exchange_load"] = exchange_load;
        p_state["load_exchange_time"] = get_load_exchange_time();
    }

    void MoverRailVehicleLoad::load_add(const double p_amount, const PlatformSide p_side, const String &p_load_name) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        // an empty vehicle takes the load first, or the first it accepts (basic_station::update_load(),
        // station.cpp:49-54)
        if (mover->LoadType.name.empty()) {
            std::string name;
            if (!p_load_name.is_empty()) {
                name = p_load_name.utf8().get_data();
            } else if (!mover->LoadAttributes.empty()) {
                name = mover->LoadAttributes.front().name;
            }
            mover->LoadAmount = 0.f;
            mover->AssignLoad(name);
            mover->ComputeMass();
        }
        exchange_load += p_amount;
        exchange_side = p_side;
        exchange_time = 0.0;
    }

    void MoverRailVehicleLoad::load_remove(const double p_amount, const PlatformSide p_side) {
        exchange_unload += p_amount;
        exchange_side = p_side;
        exchange_time = 0.0;
    }

    bool MoverRailVehicleLoad::_exchange_serves(const PlatformSide p_side) const {
        return exchange_side == PLATFORM_SIDE_BOTH || exchange_side == p_side;
    }

    int MoverRailVehicleLoad::get_load_exchange_speed() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0;
        }
        int speed = 0;
        if (_exchange_serves(PLATFORM_SIDE_LEFT) && mover->Doors.instances[side::left].is_open) {
            ++speed;
        }
        if (_exchange_serves(PLATFORM_SIDE_RIGHT) && mover->Doors.instances[side::right].is_open) {
            ++speed;
        }
        return speed;
    }

    double MoverRailVehicleLoad::get_load_exchange_time() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr || (exchange_unload < EXCHANGE_DONE && exchange_load < EXCHANGE_DONE)) {
            return 0.0;
        }
        const double base = (exchange_unload / mover->UnLoadSpeed) + (exchange_load / mover->LoadSpeed);
        // both sides exchange twice as fast (DynObj.cpp:2848)
        const int nominal = exchange_side == PLATFORM_SIDE_BOTH ? 2 : 1;
        const int speed = get_load_exchange_speed();
        return base / (speed > 0 ? speed : nominal);
    }

    String MoverRailVehicleLoad::get_load_name() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? String(mover->LoadType.name.c_str()) : String();
    }

    double MoverRailVehicleLoad::get_load_amount() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LoadAmount : 0.0;
    }

    /// TDynamicObject::update_exchange() (DynObj.cpp:2872-2960): standing, the vehicle opens its
    /// doors at the platform and exchanges a second's worth per open side; moving, it gives the
    /// exchange up; once done, the passengers may close the doors they control
    void MoverRailVehicleLoad::_do_process_component(const double p_delta) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (exchange_unload < EXCHANGE_DONE && exchange_load < EXCHANGE_DONE) {
            return;
        }
        if (mover->Vel < EXCHANGE_MAX_SPEED) {
            const int speed = get_load_exchange_speed();
            const int nominal = exchange_side == PLATFORM_SIDE_BOTH ? 2 : 1;
            if (speed < nominal) {
                const auto &left_door = mover->Doors.instances[side::left];
                const auto &right_door = mover->Doors.instances[side::right];
                if (_exchange_serves(PLATFORM_SIDE_LEFT) && !(left_door.is_open || left_door.is_opening)) {
                    mover->OperateDoors(side::left, true, range_t::local);
                }
                if (_exchange_serves(PLATFORM_SIDE_RIGHT) && !(right_door.is_open || right_door.is_opening)) {
                    mover->OperateDoors(side::right, true, range_t::local);
                }
            }
            if (speed > 0) {
                exchange_time += p_delta;
                while (exchange_unload > EXCHANGE_DONE && exchange_time >= EXCHANGE_STEP) {
                    exchange_time -= EXCHANGE_STEP;
                    const double size = std::min(exchange_unload, static_cast<double>(mover->UnLoadSpeed) * speed);
                    exchange_unload -= size;
                    mover->LoadStatus = LOAD_STATUS_UNLOADING;
                    mover->LoadAmount = std::max(0.f, mover->LoadAmount - static_cast<float>(size));
                    mover->ComputeMass();
                }
                if (exchange_unload < EXCHANGE_DONE) {
                    // what gets off first, and no more gets on than fits
                    exchange_load = std::min(exchange_load, static_cast<double>(mover->MaxLoad - mover->LoadAmount));
                    while (exchange_load > EXCHANGE_DONE && exchange_time >= EXCHANGE_STEP) {
                        exchange_time -= EXCHANGE_STEP;
                        const double size = std::min(exchange_load, static_cast<double>(mover->LoadSpeed) * speed);
                        exchange_load -= size;
                        mover->LoadStatus = LOAD_STATUS_LOADING;
                        mover->LoadAmount = std::min(mover->MaxLoad, mover->LoadAmount + static_cast<float>(size));
                        mover->ComputeMass();
                    }
                }
            }
        }
        if (mover->Vel > EXCHANGE_MAX_SPEED) {
            exchange_unload = 0.0;
            exchange_load = 0.0;
        }
        if (exchange_unload >= EXCHANGE_DONE || exchange_load >= EXCHANGE_DONE) {
            return;
        }
        mover->LoadStatus = LOAD_STATUS_DONE;
        const Maszyna::control_t close_control = mover->Doors.close_control;
        if (close_control == Maszyna::control_t::passenger || close_control == Maszyna::control_t::mixed) {
            const double chance = close_control == Maszyna::control_t::passenger ? PASSENGER_DOORS_CLOSE_CHANCE
                                                                                 : MIXED_DOORS_CLOSE_CHANCE;
            if (mover->Vel > EXCHANGE_MAX_SPEED || UtilityFunctions::randf() < chance) {
                mover->OperateDoors(side::left, false, range_t::local);
                mover->OperateDoors(side::right, false, range_t::local);
            }
        }
        if (mover->LoadAmount == 0.f) {
            mover->AssignLoad("");
        }
        emit_signal(load_exchange_finished_signal);
    }

    void MoverRailVehicleLoad::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        VehicleComponent::_fill_config_dictionary(p_config);
    }


} // namespace godot
