#pragma once
#include "macros.hpp"
#include "tracks/TrackData.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    /* What MaszynaTrackRenderingBackend draws a TrackData with that is no .scn's track: a rail track by
     * its bed, a road by its surface; a feature with no look of its own takes the default's. */
    class MaszynaTrackLooks : public Resource {
            GDCLASS(MaszynaTrackLooks, Resource)

        private:
            /* {TrackData.Bed: MaszynaTrackLook} */
            Dictionary rail_looks;
            TrackData::Bed default_bed = TrackData::BED_CRUSHED_STONE;
            /* {TrackData.Surface: MaszynaTrackLook} */
            Dictionary road_looks;
            TrackData::Surface default_surface = TrackData::SURFACE_ASPHALT;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(Dictionary, rail_looks);
            MAKE_GS_NR(TrackData::Bed, default_bed);
            MAKE_GS(Dictionary, road_looks);
            MAKE_GS_NR(TrackData::Surface, default_surface);
    };
} // namespace godot
