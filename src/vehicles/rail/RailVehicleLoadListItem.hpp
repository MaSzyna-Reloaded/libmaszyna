#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleLoadListItem : public Resource {
            GDCLASS(RailVehicleLoadListItem, Resource);

        public:
            static void _bind_methods();
            enum LoadType {
                LOAD_TYPE_NONE = 0,
                LOAD_TYPE_PASSENGER = 1,
                LOAD_TYPE_CARGO = 2,
                //@TODO: Figure out what load types are really accepted
            };

            MAKE_MEMBER_GS(float, max_load, 0.0);
            MAKE_MEMBER_GS_NR(LoadType, load_type, LoadType::LOAD_TYPE_NONE);
            MAKE_MEMBER_GS(float, load_offset, 0.0)
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleLoadListItem::LoadType)
