#include "RadioServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *RadioServer::transmitted_signal = "transmitted";

    void RadioServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("radio_create"), &RadioServer::radio_create);
        ClassDB::bind_method(D_METHOD("radio_free", "radio"), &RadioServer::radio_free);
        ClassDB::bind_method(
                D_METHOD("radio_set_receive_callback", "radio", "callback"), &RadioServer::radio_set_receive_callback);
        ClassDB::bind_method(D_METHOD("radio_set_position", "radio", "position"), &RadioServer::radio_set_position);
        ClassDB::bind_method(D_METHOD("radio_set_vehicle", "radio", "vehicle"), &RadioServer::radio_set_vehicle);
        ClassDB::bind_method(
                D_METHOD("radio_transmit", "radio", "channel", "reach", "kind", "payload"),
                &RadioServer::radio_transmit, DEFVAL(Variant()));

        BIND_ENUM_CONSTANT(KIND_MESSAGE);
        BIND_ENUM_CONSTANT(KIND_CALL);
        BIND_ENUM_CONSTANT(KIND_RADIO_STOP);
        BIND_CONSTANT(CHANNEL_ALL);
        BIND_CONSTANT(RADIO_STOP_REACH);

        ADD_SIGNAL(MethodInfo(
                transmitted_signal, PropertyInfo(Variant::INT, "kind", PROPERTY_HINT_ENUM, "Message,Call,Radio-Stop"),
                PropertyInfo(Variant::INT, "channel"), PropertyInfo(Variant::NIL, "payload"),
                PropertyInfo(Variant::VECTOR3, "position"), PropertyInfo(Variant::FLOAT, "reach")));
    }

    RID RadioServer::radio_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        radios.insert(rid, Radio());
        return rid;
    }

    void RadioServer::radio_free(const RID &p_radio) {
        radios.erase(p_radio);
    }

    void RadioServer::radio_set_receive_callback(const RID &p_radio, const Callable &p_callback) {
        Radio *radio = radios.getptr(p_radio);
        ERR_FAIL_NULL(radio);
        radio->receive_callback = p_callback;
    }

    void RadioServer::radio_set_position(const RID &p_radio, const Vector3 &p_position) {
        Radio *radio = radios.getptr(p_radio);
        ERR_FAIL_NULL(radio);
        radio->vehicle = RID();
        radio->position = p_position;
    }

    void RadioServer::radio_set_vehicle(const RID &p_radio, const RID &p_vehicle) {
        Radio *radio = radios.getptr(p_radio);
        ERR_FAIL_NULL(radio);
        radio->vehicle = p_vehicle;
    }

    Vector3 RadioServer::_get_position(const Radio &p_radio) const {
        if (!p_radio.vehicle.is_valid()) {
            return p_radio.position;
        }
        const VehicleServer *vehicles = VehicleServer::get_instance();
        return vehicles != nullptr ? vehicles->vehicle_get_position(p_radio.vehicle) : Vector3();
    }

    void RadioServer::radio_transmit(
            const RID &p_radio, const int p_channel, const double p_reach, const Kind p_kind,
            const Variant &p_payload) {
        const Radio *sender = radios.getptr(p_radio);
        ERR_FAIL_NULL_MSG(sender, "Only a registered radio transmits.");
        const Vector3 position = _get_position(*sender);
        // the receivers taken first: one may switch a radio off as it hears this
        Vector<Callable> receivers;
        for (const KeyValue<RID, Radio> &entry: radios) {
            if (entry.key == p_radio || !entry.value.receive_callback.is_valid()) {
                continue;
            }
            if (p_reach > 0.0 && _get_position(entry.value).distance_to(position) > p_reach) {
                continue;
            }
            receivers.push_back(entry.value.receive_callback);
        }
        for (const Callable &receiver: receivers) {
            receiver.call(p_kind, p_channel, p_payload, position);
        }
        emit_signal(transmitted_signal, p_kind, p_channel, p_payload, position, p_reach);
    }
} // namespace godot
