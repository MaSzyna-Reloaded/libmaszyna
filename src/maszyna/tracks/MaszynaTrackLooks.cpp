#include "maszyna/tracks/MaszynaTrackLooks.hpp"

namespace godot {
    void MaszynaTrackLooks::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                MaszynaTrackLooks, Variant::DICTIONARY, rail_looks, PROPERTY_HINT_DICTIONARY_TYPE,
                "int;MaszynaTrackLook");
        ClassDB::bind_method(D_METHOD("set_default_bed", "value"), &MaszynaTrackLooks::set_default_bed);
        ClassDB::bind_method(D_METHOD("get_default_bed"), &MaszynaTrackLooks::get_default_bed);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "default_bed", PROPERTY_HINT_ENUM, "Crushed Stone,Gravel,Dirt,Slab,Embedded",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackData.Bed"),
                "set_default_bed", "get_default_bed");
        BIND_PROPERTY_W_HINT(
                MaszynaTrackLooks, Variant::DICTIONARY, road_looks, PROPERTY_HINT_DICTIONARY_TYPE,
                "int;MaszynaTrackLook");
        ClassDB::bind_method(D_METHOD("set_default_surface", "value"), &MaszynaTrackLooks::set_default_surface);
        ClassDB::bind_method(D_METHOD("get_default_surface"), &MaszynaTrackLooks::get_default_surface);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "default_surface", PROPERTY_HINT_ENUM, "Asphalt,Concrete,Paving,Gravel,Dirt",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackData.Surface"),
                "set_default_surface", "get_default_surface");
    }
} // namespace godot
