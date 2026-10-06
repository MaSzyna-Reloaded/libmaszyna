#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleLoadListItem.hpp"

namespace godot {
    class RailVehicleLoad : public RailVehicleComponent {
            GDCLASS(RailVehicleLoad, RailVehicleComponent)

        public:
            /// The exchange at a platform is over - done, or given up once moving
            static const char *load_exchange_finished_signal;

            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_LOAD;
            }

        private:
            static void _bind_methods();

        public:
            enum LoadUnit { LOAD_UNIT_TONS, LOAD_UNIT_PIECES };
            MAKE_MEMBER_GS_NR(LoadUnit, load_unit, LoadUnit::LOAD_UNIT_TONS);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<String>, accepted_loads);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<float>, minimum_load_offsets)
            MAKE_MEMBER_GS(float, max_load, 0.0f);
            MAKE_MEMBER_GS(double, overload_factor, 0.0f);
            MAKE_MEMBER_GS(float, load_speed, 0.0f);
            MAKE_MEMBER_GS(float, unload_speed, 0.0f);
            virtual void set_load_list(const TypedArray<RailVehicleLoadListItem> &p_load_list) = 0;
            virtual TypedArray<RailVehicleLoadListItem> get_load_list() = 0;

            /// The side of the platform the load is exchanged at, in the vehicle's own frame
            enum PlatformSide { PLATFORM_SIDE_LEFT, PLATFORM_SIDE_RIGHT, PLATFORM_SIDE_BOTH };

            /// So much more to get on at the platform's side; an empty vehicle takes the load's name
            /// first, or the first load it accepts for an empty name (station.cpp:49-54,
            /// TDynamicObject::LoadExchange(), DynObj.cpp:2813)
            virtual void load_add(double p_amount, PlatformSide p_side, const String &p_load_name) = 0;
            /// So much more to get off at the platform's side (TDynamicObject::LoadExchange())
            virtual void load_remove(double p_amount, PlatformSide p_side) = 0;
            /// The seconds the exchange still takes (TDynamicObject::LoadExchangeTime(), DynObj.cpp:2843)
            virtual double get_load_exchange_time() const = 0;
            /// The doors open on the platform's side, 0 while none is (LoadExchangeSpeed(),
            /// DynObj.cpp:2855)
            virtual int get_load_exchange_speed() const = 0;
            /// What the vehicle carries, empty for nothing, and how much of it
            virtual String get_load_name() const = 0;
            virtual double get_load_amount() const = 0;
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleLoad::LoadUnit)
VARIANT_ENUM_CAST(RailVehicleLoad::PlatformSide)
