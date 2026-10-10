#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `trackvel` event (track_event, Event.cpp:1913-1956): sets the speed limit of
    /// its tracks (TTrack::VelocitySet()), negative for none
    class MaszynaLegacyTrackVelocityAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyTrackVelocityAction, ScenarioEventAction)

        private:
            TypedArray<RID> tracks;
            double velocity = -1.0;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_tracks(const TypedArray<RID> &p_tracks);
            TypedArray<RID> get_tracks() const;
            void set_velocity(double p_velocity);
            double get_velocity() const;
    };
} // namespace godot
