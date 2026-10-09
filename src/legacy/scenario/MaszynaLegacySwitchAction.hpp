#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `switch` event (switch_event, Event.cpp:1824-1893): sets its switches to one
    /// track. The blade speed and the second blade's delay it may carry are not ported (TODO.md).
    class MaszynaLegacySwitchAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacySwitchAction, ScenarioEventAction)

        private:
            TypedArray<RID> tracks;
            TrackServer::SwitchTrack active_track = TrackServer::TRACK_COMMON;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_tracks(const TypedArray<RID> &p_tracks);
            TypedArray<RID> get_tracks() const;
            void set_active_track(TrackServer::SwitchTrack p_active_track);
            TrackServer::SwitchTrack get_active_track() const;
    };
} // namespace godot
