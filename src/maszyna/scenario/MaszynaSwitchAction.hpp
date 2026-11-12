#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `switch` event (switch_event, Event.cpp:1822-1893): sets its switches to one
    /// track, with the blade speed and the second blade's delay it may carry (TTrack::Switch(),
    /// Track.cpp:1752-1762) - a negative one leaves the switch's own - and unlocks or locks its
    /// turntables (MaszynaTurntableServer, Track.cpp:1787-1833).
    class MaszynaSwitchAction : public ScenarioEventAction {
            GDCLASS(MaszynaSwitchAction, ScenarioEventAction)

        private:
            TypedArray<RID> tracks;
            TrackServer::SwitchTrack active_track = TrackServer::TRACK_COMMON;
            /// The blades' speed [m/s] and the second blade's delay [m]; negative for the switch's
            /// own (m_switchmoverate, m_switchmovedelay, Event.h:495-496)
            double blade_speed = -1.0;
            double blade_delay = -1.0;
            /// The turntables of its tracks, unlocked by any state but 0 (Track.cpp:1789)
            TypedArray<RID> turntables;
            bool unlock_turntables = false;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_tracks(const TypedArray<RID> &p_tracks);
            TypedArray<RID> get_tracks() const;
            void set_active_track(TrackServer::SwitchTrack p_active_track);
            TrackServer::SwitchTrack get_active_track() const;
            void set_blade_speed(double p_blade_speed);
            double get_blade_speed() const;
            void set_blade_delay(double p_blade_delay);
            double get_blade_delay() const;
            void set_turntables(const TypedArray<RID> &p_turntables);
            TypedArray<RID> get_turntables() const;
            void set_unlock_turntables(bool p_unlock_turntables);
            bool get_unlock_turntables() const;
            /// The switches go to the track with the blade speed and delay, the turntables are
            /// unlocked or locked - what running the event does, and what the scenery's load does to
            /// an event setting state 0 with a delay (switch_event::init(), Event.cpp:1834-1842)
            void set_switches();
    };
} // namespace godot
