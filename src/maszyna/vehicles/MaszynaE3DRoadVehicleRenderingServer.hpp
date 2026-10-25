#pragma once
#include "utils/RIDHash.hpp"
#include "vehicles/road/RoadVehicleRenderingServer.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    /* MaSzyna's road vehicles drawn (RoadVehicleRenderingServer): their lamps lit as the car's lights
     * are on (RoadVehicleController.get_lights()), by the names of the original's lamp submodels.
     * Registered by MaszynaPlugin under the core's name. */
    class MaszynaE3DRoadVehicleRenderingServer : public RoadVehicleRenderingServer {
            GDCLASS(MaszynaE3DRoadVehicleRenderingServer, RoadVehicleRenderingServer)

        private:
            /* The lamps of the car's model, by name, as they are lit now, and the lights they show */
            struct Lamps {
                    Dictionary state;
                    int64_t shown = -1;
            };

            RIDMap<Lamps> vehicles_lamps;

            void _update_lamps(const RID &p_vehicle, Lamps &p_lamps);

        protected:
            static void _bind_methods() {}

            void _vehicle_detaching(const RID &p_vehicle) override;
            void _vehicle_parts_bound(const RID &p_vehicle) override;
            void _vehicle_update(const RID &p_vehicle) override;

        public:
            ~MaszynaE3DRoadVehicleRenderingServer() override;
    };
} // namespace godot
