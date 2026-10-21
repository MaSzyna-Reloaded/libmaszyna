#pragma once
#include "macros.hpp"
#include "tracks/TrackData.hpp"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

namespace godot {
    /* A .scn's track: what the core's TrackData says, and what MaSzyna's format adds - how the track is
     * drawn (MaszynaTrackRenderingBackend), its quality and its surroundings for the running sound, its
     * events. Saved in the compiled scenery's cache by its class name. */
    class MaszynaTrackData : public TrackData {
            GDCLASS(MaszynaTrackData, TrackData)

        public:
            /// What surrounds a track, for its running sounds (TEnvironmentType, Track.h:36-44); UNKNOWN for
            /// a name the original does not know (Track.cpp:474-477)
            enum MaszynaEnvironmentType {
                UNKNOWN = -1,
                FLAT,
                MOUNTAINS,
                CANYON,
                TUNNEL,
                BRIDGE,
                BANK,
            };
            /// Keys of TrackData.meta the running sound reads per track (TrackServer.track_get_meta()) - an
            /// enum, as a native class binds no string constant
            enum MetaKey {
                META_ENVIRONMENT,
                META_QUALITY,
            };
            /// The name the track is drawn by (TrackRenderingServer.backend_register())
            static constexpr const char *RENDERING_BACKEND = "maszyna";
            /// The original's default texture length of a track [m] (Track.h:166 fTexLength)
            static constexpr double DEFAULT_TEX_LENGTH = 4.0;
            static constexpr const char *DEFAULT_RAILPROFILE = "default";

        private:
            double length = 0.0;
            String material1;
            String material2;
            double tex_length = DEFAULT_TEX_LENGTH;
            double tex_height = 0.0;
            double tex_width = 0.0;
            double tex_slope = 0.0;
            String railprofile = DEFAULT_RAILPROFILE;
            /* The .scn's other keywords of the track: `trackbed`, `isolated`, `friction` (a memory cell) */
            Dictionary parameters;
            /* The events the track names for each of its slots, by the slot's key: a slot may name several,
             * every `event2 <name>` line one more (Track.cpp:815-858, m_events2.emplace_back) */
            TypedDictionary<String, PackedStringArray> events;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS_NR(double, length);
            MAKE_GS(String, material1);
            MAKE_GS(String, material2);
            MAKE_GS_NR(double, tex_length);
            MAKE_GS_NR(double, tex_height);
            MAKE_GS_NR(double, tex_width);
            MAKE_GS_NR(double, tex_slope);
            MAKE_GS(String, railprofile);
            MAKE_GS(Dictionary, parameters);
            void set_events(const TypedDictionary<String, PackedStringArray> &p_events);
            TypedDictionary<String, PackedStringArray> get_events() const;

            /// MaszynaTrackRenderingBackend's
            StringName get_rendering_backend() const override;
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaTrackData::MaszynaEnvironmentType)
VARIANT_ENUM_CAST(MaszynaTrackData::MetaKey)
