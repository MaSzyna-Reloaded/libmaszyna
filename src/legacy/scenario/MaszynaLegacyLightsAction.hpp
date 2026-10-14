#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `lights` event (lights_event, Event.cpp:1741-1803). The event is an aspect of
    /// each signal head it is aimed at (MaszynaLegacySignalHeadKindFactory); this hands it to the
    /// signal head's system, whose MaszynaLegacySignallingImplementation shows it.
    class MaszynaLegacyLightsAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyLightsAction, ScenarioEventAction)

        private:
            TypedArray<RID> signal_heads;
            /// The aspect each of the signal heads shows, in their order
            TypedArray<StringName> aspects;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_signal_heads(const TypedArray<RID> &p_signal_heads);
            TypedArray<RID> get_signal_heads() const;
            void set_aspects(const TypedArray<StringName> &p_aspects);
            TypedArray<StringName> get_aspects() const;
    };
} // namespace godot
