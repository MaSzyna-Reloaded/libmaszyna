#pragma once
#include "maszyna/e3d/MaszynaE3DResourceFormatLoader.hpp"
#include "plugins/ReloadedPlugin.hpp"

namespace godot {
    class MaszynaE3DParser;
    class MaszynaGameDirectory;
    class MaszynaE3DRailVehicleRenderingServer;
    class MaszynaE3DRenderingServer;
    class MaszynaMoverVehicleServer;
    class MaszynaTrackRenderingBackend;
    class MaszynaTractionServer;
    class MaszynaScenarioEventServer;
    class PythonScreenServer;

    /* MaSzyna's vehicles and sceneries for Rails Reloaded: the vehicles simulated on the vendored
     * Mover, the E3D models, the original's scenery, scenario, signalling and cab formats */
    class MaszynaPlugin : public ReloadedPlugin {
            GDCLASS(MaszynaPlugin, ReloadedPlugin)

        private:
            static constexpr const char *LOGO_PATH = "res://addons/libmaszyna/logo.png";

            /* Owned: created by create_servers(), freed by free_servers() */
            MaszynaGameDirectory *game_directory = nullptr;
            MaszynaE3DParser *e3d_parser = nullptr;
            MaszynaE3DRenderingServer *e3d_rendering_server = nullptr;
            MaszynaE3DRailVehicleRenderingServer *rail_vehicle_rendering_server = nullptr;
            MaszynaTractionServer *traction_server = nullptr;
            MaszynaScenarioEventServer *scenario_event_server = nullptr;
            MaszynaMoverVehicleServer *mover_vehicle_server = nullptr;
            MaszynaTrackRenderingBackend *track_rendering_backend = nullptr;
            PythonScreenServer *python_screen_server = nullptr;
            Ref<MaszynaE3DResourceFormatLoader> e3d_resource_format_loader;

        protected:
            static void _bind_methods() {}

        public:
            MaszynaPlugin();

            void register_classes() override;
            void create_servers() override;
            void free_servers() override;
    };
} // namespace godot
