#include "maszyna/scenery/MaszynaTrackData.hpp"

namespace godot {
    void MaszynaTrackData::_bind_methods() {
        BIND_ENUM_CONSTANT(UNKNOWN);
        BIND_ENUM_CONSTANT(FLAT);
        BIND_ENUM_CONSTANT(MOUNTAINS);
        BIND_ENUM_CONSTANT(CANYON);
        BIND_ENUM_CONSTANT(TUNNEL);
        BIND_ENUM_CONSTANT(BRIDGE);
        BIND_ENUM_CONSTANT(BANK);
        BIND_ENUM_CONSTANT(META_ENVIRONMENT);
        BIND_ENUM_CONSTANT(META_QUALITY);

        BIND_PROPERTY(MaszynaTrackData, Variant::FLOAT, length);
        BIND_PROPERTY(MaszynaTrackData, Variant::STRING, material1);
        BIND_PROPERTY(MaszynaTrackData, Variant::STRING, material2);
        BIND_PROPERTY(MaszynaTrackData, Variant::FLOAT, tex_length);
        BIND_PROPERTY(MaszynaTrackData, Variant::FLOAT, tex_height);
        BIND_PROPERTY(MaszynaTrackData, Variant::FLOAT, tex_width);
        BIND_PROPERTY(MaszynaTrackData, Variant::FLOAT, tex_slope);
        BIND_PROPERTY(MaszynaTrackData, Variant::STRING, railprofile);
        BIND_PROPERTY(MaszynaTrackData, Variant::DICTIONARY, parameters);
        BIND_PROPERTY_W_HINT(
                MaszynaTrackData, Variant::DICTIONARY, events, PROPERTY_HINT_DICTIONARY_TYPE,
                "String;PackedStringArray");
    }

    void MaszynaTrackData::set_events(const TypedDictionary<String, PackedStringArray> &p_events) {
        events = p_events;
    }

    TypedDictionary<String, PackedStringArray> MaszynaTrackData::get_events() const {
        return events;
    }

    StringName MaszynaTrackData::get_rendering_backend() const {
        return RENDERING_BACKEND;
    }
} // namespace godot
