#pragma once
#include "signalling/SignalHeadKind.hpp"
#include <godot_cpp/classes/object.hpp>

namespace godot {
    /// Kinds of the original's signal heads. The original has none: what a signal head shows is
    /// whatever `lights` events the scenery aims at its model (Event.cpp:1741-1803), each one a
    /// list of TAnimModel::LightSet() values. Every such event is made an aspect of the kind here,
    /// named after the event without the signal head's own name - `(p1)_sem_ligh1` aimed at `(p1)`
    /// is `sem_ligh1` - so every copy of one include has the same kind.
    class MaszynaLegacySignalHeadKindFactory : public Object {
            GDCLASS(MaszynaLegacySignalHeadKindFactory, Object)

        public:
            /// A `lights` event value that leaves the light as it is (Event.cpp:1797)
            static constexpr float LIGHT_UNCHANGED = -1.0;
            /// Values a `lights` event carries at most; a light it names no value for is left as it
            /// is (Event.cpp:1761, :1792)
            static constexpr int MAX_EVENT_LIGHTS = 8;

        protected:
            static void _bind_methods();

        public:
            /// aspect name -> PackedFloat32Array of the event's values
            static Ref<SignalHeadKind> create_kind(const Dictionary &p_aspects);
            /// The aspect a `lights` event is for the signal head it is aimed at
            static StringName get_aspect_name(const String &p_event_name, const String &p_signal_head_name);
    };
} // namespace godot
