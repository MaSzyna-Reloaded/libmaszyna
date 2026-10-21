#include "maszyna/tracks/MaszynaTrackLook.hpp"

namespace godot {
    void MaszynaTrackLook::_bind_methods() {
        BIND_PROPERTY(MaszynaTrackLook, Variant::STRING, material1);
        BIND_PROPERTY(MaszynaTrackLook, Variant::STRING, material1_centre_line);
        BIND_PROPERTY(MaszynaTrackLook, Variant::STRING, material2);
        BIND_PROPERTY(MaszynaTrackLook, Variant::FLOAT, tex_length);
        BIND_PROPERTY(MaszynaTrackLook, Variant::FLOAT, tex_height);
        BIND_PROPERTY(MaszynaTrackLook, Variant::FLOAT, tex_width);
        BIND_PROPERTY(MaszynaTrackLook, Variant::FLOAT, tex_slope);
        BIND_PROPERTY(MaszynaTrackLook, Variant::STRING, railprofile);
    }
} // namespace godot
