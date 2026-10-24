#include "maszyna/MaszynaDataPath.hpp"
#include "maszyna/MaszynaGameDirectory.hpp"
#include "maszyna/MaszynaPlugin.hpp"
#include "maszyna/cabin/MaszynaCabinLampIslands.hpp"
#include "maszyna/cabin/PythonScreenServer.hpp"
#include "maszyna/e3d/MaszynaE3DMesh.hpp"
#include "maszyna/e3d/MaszynaE3DModel.hpp"
#include "maszyna/e3d/MaszynaE3DModelLightDefinition.hpp"
#include "maszyna/e3d/MaszynaE3DModelSmokeSourceDefinition.hpp"
#include "maszyna/e3d/MaszynaE3DRenderingServer.hpp"
#include "maszyna/e3d/MaszynaE3DSubModel.hpp"
#include "maszyna/e3d/e3d_parser.hpp"
#include "maszyna/e3d/t3d_parser.hpp"
#include "maszyna/parsers/maszyna_parser.hpp"
#include "maszyna/scenario/MaszynaAnimationAction.hpp"
#include "maszyna/scenario/MaszynaEventCondition.hpp"
#include "maszyna/scenario/MaszynaFrictionAction.hpp"
#include "maszyna/scenario/MaszynaLightsAction.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "rendering/TrackRenderingServer.hpp"
#ifdef LIBRELOADED_LUA
#include "maszyna/scenario/MaszynaLuaEventsModule.hpp"
#include "maszyna/scenario/MaszynaLuaMemoryModule.hpp"
#include "scripting/lua/LuaModules.hpp"
#endif
#include "maszyna/scenario/MaszynaMemoryAction.hpp"
#include "maszyna/scenario/MaszynaMultipleAction.hpp"
#include "maszyna/scenario/MaszynaScenarioEventImplementation.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "maszyna/scenario/MaszynaSwitchAction.hpp"
#include "maszyna/scenario/MaszynaTrackVelocityAction.hpp"
#include "maszyna/scenario/MaszynaVehicleCommandAction.hpp"
#include "maszyna/scenario/MaszynaVoltageAction.hpp"
#include "maszyna/scenery/MaszynaSBTTerrainProvider.hpp"
#include "maszyna/scenery/MaszynaTrackData.hpp"
#include "maszyna/scenery/MaszynaTrianglesChunkGeometry.hpp"
#include "maszyna/scenery/MaszynaTrianglesImporter.hpp"
#include "maszyna/scenery/MaszynaTrianglesSector.hpp"
#include "maszyna/scenery/MaszynaTrianglesSink.hpp"
#include "maszyna/signalling/MaszynaSignalHeadKindFactory.hpp"
#include "maszyna/signalling/MaszynaSignallingImplementation.hpp"
#include "maszyna/tracks/MaszynaTrackLook.hpp"
#include "maszyna/tracks/MaszynaTrackLooks.hpp"
#include "maszyna/tracks/MaszynaTrackRenderingBackend.hpp"
#include "maszyna/traction/MaszynaTractionServer.hpp"
#include "maszyna/vehicles/MaszynaE3DRailVehicleRenderingServer.hpp"
#include "maszyna/vehicles/MaszynaMoverBrakePressureTableItem.hpp"
#include "maszyna/vehicles/MaszynaMoverCompressorListItem.hpp"
#include "maszyna/vehicles/MaszynaMoverDieselConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverElectricEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverMotorParameter.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleAIHints.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleBrake.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleBuffCoupl.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleController.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleDieselElectricEngine.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleDieselEngine.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleDoors.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleElectricInductionEngine.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleElectricSeriesEngine.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleElectroPneumaticDynamicBrake.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleEnginePowerSource.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleHeating.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleHorns.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleLighting.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleLoad.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleMasterController.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehiclePowerSupply.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleRadio.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleSecuritySystem.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleSpeedControl.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleSpringBrake.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleSwitches.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleUniversalController.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleWheels.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleWipers.hpp"
#include "maszyna/vehicles/MaszynaMoverRelayListItem.hpp"
#include "maszyna/vehicles/MaszynaMoverThrottlePositionItem.hpp"
#include "maszyna/vehicles/MaszynaMoverUniversalControllerListItem.hpp"
#include "maszyna/vehicles/MaszynaMoverVehicleServer.hpp"
#include "maszyna/vehicles/MaszynaMoverWWListItem.hpp"
#include "maszyna/vehicles/MaszynaRailVehicleAppearance.hpp"
#include "plugins/ReloadedPluginServer.hpp"
#include "vehicles/base/VehiclePhysicsNode.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>

namespace godot {
    MaszynaPlugin::MaszynaPlugin() {
        name = "MaSzyna";
        description = "Vehicles and sceneries of MaSzyna EU07: the vehicles simulated on its Mover, its E3D models, "
                      "sceneries, scenarios, signalling and cabs";
    }

    void MaszynaPlugin::register_classes() {
        GDREGISTER_CLASS(MaszynaGameDirectory);
        GDREGISTER_CLASS(MaszynaE3DMesh);
        GDREGISTER_CLASS(MaszynaE3DSubModel);
        GDREGISTER_CLASS(MaszynaE3DModel);
        GDREGISTER_CLASS(MaszynaE3DParser);
        GDREGISTER_CLASS(MaszynaT3DParser);
        GDREGISTER_CLASS(MaszynaE3DModelLightDefinition);
        GDREGISTER_CLASS(MaszynaE3DModelSmokeSourceDefinition);
        GDREGISTER_CLASS(MaszynaE3DRenderingServer);
        GDREGISTER_CLASS(MaszynaRailVehicleAppearance);
        GDREGISTER_CLASS(MaszynaE3DRailVehicleRenderingServer);
        GDREGISTER_CLASS(MaszynaTractionServer);
        GDREGISTER_CLASS(MaszynaE3DResourceFormatLoader);
        GDREGISTER_CLASS(MaszynaMoverVehicleServer);
        GDREGISTER_ABSTRACT_CLASS(MaszynaSignalHeadKindFactory);
        GDREGISTER_CLASS(MaszynaSignallingImplementation);
        GDREGISTER_CLASS(MaszynaScenarioEventServer);
        GDREGISTER_CLASS(MaszynaScenarioEventImplementation);
        GDREGISTER_CLASS(MaszynaMemoryAction);
        GDREGISTER_CLASS(MaszynaMultipleAction);
        GDREGISTER_CLASS(MaszynaLightsAction);
        GDREGISTER_CLASS(MaszynaSwitchAction);
        GDREGISTER_CLASS(MaszynaVoltageAction);
        GDREGISTER_CLASS(MaszynaTrackVelocityAction);
        GDREGISTER_CLASS(MaszynaFrictionAction);
        GDREGISTER_CLASS(MaszynaAnimationAction);
        GDREGISTER_CLASS(MaszynaVehicleCommandAction);
        GDREGISTER_CLASS(MaszynaEventCondition);
        GDREGISTER_ABSTRACT_CLASS(MaszynaDataPath);
        GDREGISTER_ABSTRACT_CLASS(MaszynaCabinLampIslands);
        GDREGISTER_CLASS(MaszynaParser);
        GDREGISTER_CLASS(MaszynaTrianglesImporter);
        GDREGISTER_CLASS(MaszynaTrianglesChunkGeometry);
        GDREGISTER_CLASS(MaszynaTrianglesSector);
        GDREGISTER_CLASS(MaszynaTrianglesSink);
        GDREGISTER_CLASS(MaszynaSBTTerrainProvider);
        GDREGISTER_CLASS(MaszynaTrackData);
        GDREGISTER_CLASS(MaszynaTrackLook);
        GDREGISTER_CLASS(MaszynaTrackLooks);
        GDREGISTER_ABSTRACT_CLASS(MaszynaTrackRenderingBackend);
        GDREGISTER_CLASS(PythonScreenServer);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleBrake);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleSpringBrake);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleDoors);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleDieselEngine);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleDieselElectricEngine);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleElectricSeriesEngine);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleElectricInductionEngine);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleController);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleHeating);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleRadio);
        GDREGISTER_CLASS(MaszynaMoverBrakePressureTableItem);
        GDREGISTER_CLASS(MaszynaMoverCompressorListItem);
        GDREGISTER_CLASS(MaszynaMoverRelayListItem);
        GDREGISTER_CLASS(MaszynaMoverUniversalControllerListItem);
        GDREGISTER_CLASS(MaszynaMoverWWListItem);
        GDREGISTER_CLASS(MaszynaMoverEngineConfiguration);
        GDREGISTER_CLASS(MaszynaMoverElectricEngineConfiguration);
        GDREGISTER_CLASS(MaszynaMoverDieselConfiguration);
        GDREGISTER_CLASS(MaszynaMoverMotorParameter);
        GDREGISTER_CLASS(MaszynaMoverThrottlePositionItem);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleWheels);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleSecuritySystem);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleHorns);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleAIHints);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleLighting);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleElectroPneumaticDynamicBrake);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleLoad);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleBuffCoupl);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleSpeedControl);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleUniversalController);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleMasterController);
        GDREGISTER_CLASS(MaszynaMoverRailVehiclePowerSupply);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleEnginePowerSource);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleWipers);
        GDREGISTER_CLASS(MaszynaMoverRailVehicleSwitches);
        // the vehicles are simulated on the vendored Mover
        VehiclePhysicsNode::set_controller_implementation(MaszynaMoverRailVehicleController::get_class_static());
        // the data is written for Windows' case-insensitive filesystem
        ReloadedPluginServer::get_instance()->data_path_set_resolver(callable_mp_static(&MaszynaDataPath::resolve));
#ifdef LIBRELOADED_LUA
        // the memory cells in every scenario script, then the original's scripting API written on them
        MaszynaLuaMemoryModule::register_module();
        LuaModules::opener_register(&MaszynaLuaEventsModule::open);
#endif
    }

    void MaszynaPlugin::create_servers() {
        // after UserSettings, GameDataServer and ReloadedTranslationServer: the constructor follows
        // the settings, hands the translations the data's catalogues and reloads the data on a
        // change; before every server that reads the game directory
        game_directory = memnew(MaszynaGameDirectory);
        Engine::get_singleton()->register_singleton("MaszynaGameDirectory", game_directory);
        e3d_parser = memnew(MaszynaE3DParser);
        Engine::get_singleton()->register_singleton("MaszynaE3DParser", e3d_parser);
        // after SceneryStreamingServer and ModelRenderingServer are registered: the constructor takes
        // the models providers supply, and it draws the models of MaSzyna
        e3d_rendering_server = memnew(MaszynaE3DRenderingServer);
        Engine::get_singleton()->register_singleton("MaszynaE3DRenderingServer", e3d_rendering_server);
        if (ModelRenderingServer *models = ModelRenderingServer::get_instance(); models != nullptr) {
            models->backend_register(MaszynaE3DRenderingServer::BACKEND_NAME, e3d_rendering_server->get_instance_id());
        }
        // after TrackRenderingServer, SceneryStreamingServer and GameDataServer: the constructor streams
        // the roads and follows the data's unloads and reloads; it draws the tracks of MaSzyna
        track_rendering_backend = memnew(MaszynaTrackRenderingBackend);
        if (TrackRenderingServer *track_rendering = TrackRenderingServer::get_instance(); track_rendering != nullptr) {
            track_rendering->backend_register(
                    MaszynaTrackRenderingBackend::NAME, track_rendering_backend->get_instance_id());
        }
        // after VehicleServer is registered: the constructor follows the vehicles' lifetime
        mover_vehicle_server = memnew(MaszynaMoverVehicleServer);
        Engine::get_singleton()->register_singleton("MaszynaMoverVehicleServer", mover_vehicle_server);
        VehicleServer::get_instance()->implementation_register(
                MaszynaMoverVehicleServer::IMPLEMENTATION_NAME, mover_vehicle_server->get_instance_id());
        python_screen_server = memnew(PythonScreenServer);
        Engine::get_singleton()->register_singleton("PythonScreenServer", python_screen_server);
        // after the vehicle servers and MaszynaE3DRenderingServer: the constructor follows their
        // vehicles and instances. The core's scripts reach it under the core's name
        rail_vehicle_rendering_server = memnew(MaszynaE3DRailVehicleRenderingServer);
        Engine::get_singleton()->register_singleton("RailVehicleRenderingServer", rail_vehicle_rendering_server);
        Engine::get_singleton()->register_singleton(
                "MaszynaE3DRailVehicleRenderingServer", rail_vehicle_rendering_server);
        // the traction network as the original computes it, under the core's name
        traction_server = memnew(MaszynaTractionServer);
        Engine::get_singleton()->register_singleton("TractionServer", traction_server);
        // after ScenarioEventServer, SimulationServer, RailVehicleServer, RadioServer and TrackServer:
        // the constructor follows their events, clock, tracks, radio and isolated sections
        scenario_event_server = memnew(MaszynaScenarioEventServer);
        Engine::get_singleton()->register_singleton("MaszynaScenarioEventServer", scenario_event_server);
        e3d_resource_format_loader.instantiate();
        ResourceLoader::get_singleton()->add_resource_format_loader(e3d_resource_format_loader);
        logo = ResourceLoader::get_singleton()->load(LOGO_PATH);
#ifdef LIBRELOADED_LUA
        // a memory cell's change runs the functions the scripts subscribed to it
        if (MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance(); events != nullptr) {
            events->connect(
                    MaszynaScenarioEventServer::memory_values_changed_signal,
                    callable_mp_static(&MaszynaLuaMemoryModule::deliver_values_changed));
        }
#endif
    }

    void MaszynaPlugin::free_servers() {
#ifdef LIBRELOADED_LUA
        if (MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance(); events != nullptr) {
            events->disconnect(
                    MaszynaScenarioEventServer::memory_values_changed_signal,
                    callable_mp_static(&MaszynaLuaMemoryModule::deliver_values_changed));
        }
#endif
        ResourceLoader::get_singleton()->remove_resource_format_loader(e3d_resource_format_loader);
        e3d_resource_format_loader.unref();

        Engine::get_singleton()->unregister_singleton("MaszynaScenarioEventServer");
        memdelete(scenario_event_server);
        scenario_event_server = nullptr;

        Engine::get_singleton()->unregister_singleton("TractionServer");
        memdelete(traction_server);
        traction_server = nullptr;

        Engine::get_singleton()->unregister_singleton("MaszynaE3DRailVehicleRenderingServer");
        Engine::get_singleton()->unregister_singleton("RailVehicleRenderingServer");
        memdelete(rail_vehicle_rendering_server);
        rail_vehicle_rendering_server = nullptr;

        Engine::get_singleton()->unregister_singleton("PythonScreenServer");
        memdelete(python_screen_server);
        python_screen_server = nullptr;

        Engine::get_singleton()->unregister_singleton("MaszynaMoverVehicleServer");
        VehicleServer::get_instance()->implementation_unregister(MaszynaMoverVehicleServer::IMPLEMENTATION_NAME);
        memdelete(mover_vehicle_server);
        mover_vehicle_server = nullptr;

        if (TrackRenderingServer *track_rendering = TrackRenderingServer::get_instance(); track_rendering != nullptr) {
            track_rendering->backend_unregister(MaszynaTrackRenderingBackend::NAME);
        }
        memdelete(track_rendering_backend);
        track_rendering_backend = nullptr;

        if (ModelRenderingServer *models = ModelRenderingServer::get_instance(); models != nullptr) {
            models->backend_unregister(MaszynaE3DRenderingServer::BACKEND_NAME);
        }
        Engine::get_singleton()->unregister_singleton("MaszynaE3DRenderingServer");
        memdelete(e3d_rendering_server);
        e3d_rendering_server = nullptr;

        Engine::get_singleton()->unregister_singleton("MaszynaE3DParser");
        memdelete(e3d_parser);
        e3d_parser = nullptr;

        Engine::get_singleton()->unregister_singleton("MaszynaGameDirectory");
        memdelete(game_directory);
        game_directory = nullptr;
    }
} // namespace godot
