#include "MaszynaSignallingImplementation.hpp"
#include "signalling/SignallingServer.hpp"

namespace godot {
    void MaszynaSignallingImplementation::_bind_methods() {}

    void MaszynaSignallingImplementation::handle_event(
            const RID &p_system, const StringName &p_event, const Dictionary &p_arguments) {
        if (!(p_event == StringName(LIGHTS_EVENT))) {
            return;
        }
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->signal_head_set_aspect(p_arguments.get("signal_head", RID()), p_arguments.get("aspect", StringName()));
    }
} // namespace godot
