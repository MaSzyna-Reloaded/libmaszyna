#pragma once

#include <godot_cpp/core/error_macros.hpp>

namespace godot {
    /* A server's own instance, as the engine's servers keep theirs (RenderingServer::singleton): made
     * by memnew() in register_types.cpp and freed there, in the order the servers depend on. A
     * second instance is refused and never takes the pointer over, and freeing it does not clear
     * the first one's (Engine::~Engine(), core/config/engine.cpp:449). The pointer is cleared after
     * the server's own destructor has run. */
    template<typename T>
    class ServerSingleton {
        public:
            static T *get_instance() {
                return singleton;
            }

        private:
            friend T;

            static inline T *singleton = nullptr;

            ServerSingleton() {
                ERR_FAIL_COND_MSG(singleton != nullptr, "A server is made only once, by the module");
                singleton = static_cast<T *>(this);
            }

            ~ServerSingleton() {
                if (singleton == static_cast<T *>(this)) {
                    singleton = nullptr;
                }
            }
    };
} // namespace godot
