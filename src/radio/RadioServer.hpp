#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// The radio ether: what a radio transmits - on a channel, from where it is, as far as its reach -
    /// every other radio within that reach hears, whatever it is tuned to; the receiver compares the
    /// channel itself, so a message tuned to midway is still heard (TTrain::radio_message(),
    /// Train.cpp:11010-11029). A radio is registered here, a transmitter and a receiver at once: a
    /// vehicle's (its position is the vehicle's at the moment of a transmission, whatever kind of
    /// vehicle it is), or one standing at a place. Whether it is on is its owner's to answer: it
    /// sends only when it can, and plays only what it can hear. Whoever transmits has a radio.
    class RadioServer : public Object {
            GDCLASS(RadioServer, Object)

        public:
            enum Kind {
                /// A spoken message: payload {sound: SfxEvent, transcript: Transcript}
                KIND_MESSAGE,
                /// A radio call (RailVehicleRadio.RadioCall), to the dispatcher
                KIND_CALL,
                /// Radio-Stop: every train in reach brakes in emergency
                KIND_RADIO_STOP,
            };

            /// Heard on every channel - Radio-Stop (0 is a scenery sound's "not a radio message")
            static constexpr int CHANNEL_ALL = -1;
            /// How far Radio-Stop reaches [m] (basic_region::RadioStop, scene.cpp:1271)
            static constexpr double RADIO_STOP_REACH = 2000.0;

            /// Something was transmitted, for whoever follows the ether without being a radio - the
            /// scenario (kind: Kind, channel: int, payload: Variant, position: Vector3, reach: float)
            static const char *transmitted_signal;

            static RadioServer *get_instance() {
                return Object::cast_to<RadioServer>(Engine::get_singleton()->get_singleton("RadioServer"));
            }

        private:
            struct Radio {
                    /// The vehicle it is in, or none for one standing at `position`
                    RID vehicle;
                    Vector3 position;
                    /// (kind, channel, payload, position) of what it hears; none for a transmitter only
                    Callable receive_callback;
            };
            HashMap<RID, Radio> radios;

            Vector3 _get_position(const Radio &p_radio) const;

        protected:
            static void _bind_methods();

        public:
            RID radio_create();
            void radio_free(const RID &p_radio);
            void radio_set_receive_callback(const RID &p_radio, const Callable &p_callback);
            /// A radio standing at a place
            void radio_set_position(const RID &p_radio, const Vector3 &p_position);
            /// A radio in a vehicle, wherever the vehicle is
            void radio_set_vehicle(const RID &p_radio, const RID &p_vehicle);
            /// From where the radio is, to every other radio within `reach` [m] - everywhere when it
            /// is not positive
            void radio_transmit(
                    const RID &p_radio, int p_channel, double p_reach, Kind p_kind,
                    const Variant &p_payload = Variant());
    };
} // namespace godot

VARIANT_ENUM_CAST(RadioServer::Kind)
