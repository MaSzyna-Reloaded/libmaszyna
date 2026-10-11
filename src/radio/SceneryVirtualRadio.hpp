#pragma once
#include "radio/RadioServer.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// The scenery's own radio, one a scenery: whatever the scenario says on the radio - a signal box,
    /// the dispatcher, the scenery's Radio-Stop - is transmitted from where it is said. A transmitter
    /// only: it hears nothing. Its radio goes with it.
    class SceneryVirtualRadio : public RefCounted {
            GDCLASS(SceneryVirtualRadio, RefCounted)

        private:
            RID radio;

        protected:
            static void _bind_methods();

        public:
            SceneryVirtualRadio();
            ~SceneryVirtualRadio() override;

            /// RadioServer.radio_transmit(), from `position`
            void transmit(
                    const Vector3 &p_position, int p_channel, double p_reach, RadioServer::Kind p_kind,
                    const Variant &p_payload = Variant());
    };
} // namespace godot
