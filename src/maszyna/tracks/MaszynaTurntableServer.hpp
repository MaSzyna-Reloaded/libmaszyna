#pragma once
#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// The original's turntables (TTrack of tt_Table, `turn` and `table` in a .scn): a track bound to
    /// the model of its name, which an `animation` event turns. A `switch 1` unlocks it - takes it
    /// off its neighbours and stops traffic on it, and its track follows the model's first animated
    /// submodel from then on; a `switch 0` locks it where it stands, joins it to the tracks its ends
    /// meet and lets traffic on again (TTrack::Switch(), Track.cpp:1787-1833; TTrack::RaAnimate(),
    /// Track.cpp:2024-2061). The track's shape, its neighbours and its speed limit are TrackServer's:
    /// this server only says what they become.
    class MaszynaTurntableServer : public Object {
            GDCLASS(MaszynaTurntableServer, Object)

        private:
            static MaszynaTurntableServer *singleton;

            /// The speed a locked turntable and the tracks it joined are let on at [km/h]
            /// (Track.cpp:1816-1825)
            static constexpr double JOINED_VELOCITY = 6.0;
            /// A t3d's translation as the scenery's x, y, z (TAnimContainer::TransGet(), AnimModel.h:96)
            static constexpr int T3D_SIDE_SIGN = -1;
            /// TTrack::location() averages the start, the middle and the end of the track (Track.cpp:940-944)
            static constexpr double LOCATION_POINTS = 3.0;
            /// The parameter of the middle of a track's curve (FastGetPoint(0.5), Track.cpp:942)
            static constexpr double CURVE_MIDDLE = 0.5;

            struct TurntableData {
                    RID track;
                    /// The model of the track's name, none for a turntable of no model - a derailer
                    /// (path_table::InitTracks(), Track.cpp:3418-3423)
                    RID instance;
                    /// The model's first animated submodel, the one the track follows
                    /// (TAnimModel::GetContainer(), AnimModel.cpp:486-489)
                    String submodel;
                    /// The model's yaw [deg] (TAnimModel::Angles().y)
                    double model_yaw = 0.0;
                    /// Run when it is locked and joined (`<track>:joined`, SwitchExtension->evPlus)
                    RID joined_event;
                    /// The track's middle and half its length as loaded (TTrack::location(),
                    /// SwitchExtension->Segments[0])
                    Vector3 location;
                    double half_length = 0.0;
                    /// SwitchExtension->CurrentIndex: 1 unlocked, turning
                    bool unlocked = false;
                    /// The angle and the translation it was last turned to (SwitchExtension->fOffset, vTrans)
                    double offset = 0.0;
                    Vector3 translation;
            };

            RIDMap<TurntableData> turntables;
            /// The unlocked turntables with a model, followed on every simulation step
            LocalVector<RID> following;

            /// TTrack::RaAnimate() of a turntable (Track.cpp:2024-2061): its track laid again
            /// where the submodel has turned it, when it has moved since
            void _follow(TurntableData &p_turntable) const;
            void _on_simulation_advanced(double p_seconds);
            void _set_following(const RID &p_turntable, bool p_following);

        protected:
            static void _bind_methods();

        public:
            static MaszynaTurntableServer *get_instance();

            MaszynaTurntableServer();
            ~MaszynaTurntableServer() override;

            /// A turntable of the track - locked, as it is loaded
            RID turntable_create(const RID &p_track);
            void turntable_free(const RID &p_turntable);
            /// The model it is bound to and the submodel of it the track follows
            void turntable_set_model(const RID &p_turntable, const RID &p_instance, const String &p_submodel);
            void turntable_set_joined_event(const RID &p_turntable, const RID &p_event);
            /// `switch 1` (Track.cpp:1789-1806): off its neighbours, no traffic on it, turning
            void turntable_unlock(const RID &p_turntable);
            /// `switch 0` (Track.cpp:1807-1831): laid where it turned to, joined to the tracks its
            /// ends meet - and, if it met any, traffic let on and its `:joined` event run
            void turntable_lock(const RID &p_turntable);
            bool turntable_is_locked(const RID &p_turntable) const;
    };
} // namespace godot
