#pragma once
#include "scenario/ScenarioEventAction.hpp"

namespace godot {
    /// The action of a ScenarioEventServer event a scenario script owns: running the event runs
    /// the script's function behind it. Created by ScenarioScriptServer only.
    class ScenarioScriptAction : public ScenarioEventAction {
            GDCLASS(ScenarioScriptAction, ScenarioEventAction)

        private:
            /// ScenarioScriptServer's record of what the event does
            RID hook;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;
            void run_else(const RID &p_event, const RID &p_activator) override;

        public:
            void set_hook(const RID &p_hook);
    };
} // namespace godot
