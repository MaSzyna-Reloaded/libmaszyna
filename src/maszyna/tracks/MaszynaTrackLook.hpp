#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* The textures and the section a TrackData's feature is drawn with (MaszynaTrackLooks), in the .scn's
     * own terms (MaszynaTrackData) */
    class MaszynaTrackLook : public Resource {
            GDCLASS(MaszynaTrackLook, Resource)

        public:
            static constexpr const char *DEFAULT_RAILPROFILE = "default";

        private:
            String material1;
            /* A road's first texture when it has a centre line; empty for material1 */
            String material1_centre_line;
            String material2;
            /* The length the textures cover [m]; 0 for the track's width (a road's) */
            double tex_length = 0.0;
            double tex_height = 0.0;
            double tex_width = 0.0;
            double tex_slope = 0.0;
            String railprofile = DEFAULT_RAILPROFILE;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, material1);
            MAKE_GS(String, material1_centre_line);
            MAKE_GS(String, material2);
            MAKE_GS_NR(double, tex_length);
            MAKE_GS_NR(double, tex_height);
            MAKE_GS_NR(double, tex_width);
            MAKE_GS_NR(double, tex_slope);
            MAKE_GS(String, railprofile);
    };
} // namespace godot
