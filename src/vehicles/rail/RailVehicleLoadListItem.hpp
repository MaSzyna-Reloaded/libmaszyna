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

        private:
            float max_load = 0.0;
            LoadType load_type = LoadType::LOAD_TYPE_NONE;
            float load_offset = 0.0;

        public:
            MAKE_GS(float, max_load);
            MAKE_GS_NR(LoadType, load_type);
            MAKE_GS(float, load_offset);
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleLoadListItem::LoadType)
