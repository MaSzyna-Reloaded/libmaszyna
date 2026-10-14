#include "SceneryVirtualRadio.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void SceneryVirtualRadio::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("transmit", "position", "channel", "reach", "kind", "payload"), &SceneryVirtualRadio::transmit,
                DEFVAL(Variant()));
    }

    SceneryVirtualRadio::SceneryVirtualRadio() {
        RadioServer *server = RadioServer::get_instance();
        ERR_FAIL_NULL(server);
        radio = server->radio_create();
    }

    SceneryVirtualRadio::~SceneryVirtualRadio() {
        if (RadioServer *server = RadioServer::get_instance(); server != nullptr && radio.is_valid()) {
            server->radio_free(radio);
        }
    }

    void SceneryVirtualRadio::transmit(
            const Vector3 &p_position, const int p_channel, const double p_reach, const RadioServer::Kind p_kind,
            const Variant &p_payload) {
        RadioServer *server = RadioServer::get_instance();
        ERR_FAIL_NULL(server);
        server->radio_set_position(radio, p_position);
        server->radio_transmit(radio, p_channel, p_reach, p_kind, p_payload);
    }
} // namespace godot
