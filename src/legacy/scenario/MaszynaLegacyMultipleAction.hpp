#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `multiple` event (multi_event, Event.cpp:1213-1321): queues its events,
    /// handing them its activator. The events listed after `else` are queued instead when the
    /// condition fails.
    class MaszynaLegacyMultipleAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyMultipleAction, ScenarioEventAction)

        private:
            TypedArray<RID> events;
            TypedArray<RID> else_events;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;
            void run_else(const RID &p_event, const RID &p_activator) override;

        public:
            void set_events(const TypedArray<RID> &p_events);
            TypedArray<RID> get_events() const;
            void set_else_events(const TypedArray<RID> &p_events);
            TypedArray<RID> get_else_events() const;
    };
} // namespace godot
