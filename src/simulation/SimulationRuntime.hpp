#pragma once

#include <godot_cpp/classes/node.hpp>

namespace godot {

    /// The running simulation: SimulationServer's clock ticks only while this node is in the tree.
    /// The game places it in its scene explicitly; a scenery loaded without it - in the editor, in
    /// a viewer - is built but nothing in it runs (no drivers, events or vehicle physics). In the
    /// editor it does nothing, so a game scene opened there does not start the clock.
    class SimulationRuntime : public Node {
            GDCLASS(SimulationRuntime, Node)

        protected:
            static void _bind_methods();
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);
    };

} // namespace godot
