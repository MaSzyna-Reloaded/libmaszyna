#pragma once
#include "scenario/ScenarioEventAction.hpp"

namespace godot {
    /// The original's `friction` event (friction_event, Event.cpp:2077-2110): sets the scenery's rail
    /// friction multiplier (Global.fFriction) - SimulationServer's friction
    class MaszynaLegacyFrictionAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyFrictionAction, ScenarioEventAction)

        private:
            double friction = 1.0;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_friction(double p_friction);
            double get_friction() const;
    };
} // namespace godot
