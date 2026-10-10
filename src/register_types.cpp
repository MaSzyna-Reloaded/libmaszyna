#include "cabin/Cabin3D.hpp"
#include "cabin/CabinHUDMouseSystem.hpp"
#include "cache/ResourceCache.hpp"
#include "driver/DriverImplementation.hpp"
#include "driver/DriverServer.hpp"
#include "game_data/GameDataServer.hpp"
#include "hud/HUDServer.hpp"
#include "legacy/MaszynaDataPath.hpp"
#include "legacy/cabin/LegacyCabinLampIslands.hpp"
#include "legacy/cabin/PythonScreenServer.hpp"
#include "legacy/e3d/E3DModel.hpp"
#include "legacy/e3d/E3DModelLightDefinition.hpp"
#include "legacy/e3d/E3DModelSmokeSourceDefinition.hpp"
#include "legacy/e3d/E3DRenderingServer.hpp"
#include "legacy/e3d/E3DResourceFormatLoader.hpp"
#include "legacy/e3d/E3DSubModel.hpp"
#include "legacy/e3d/e3d_parser.hpp"
#include "legacy/e3d/t3d_parser.hpp"
#include "legacy/parsers/maszyna_parser.hpp"
#include "legacy/scenario/MaszynaLegacyAnimationAction.hpp"
#include "legacy/scenario/MaszynaLegacyEventCondition.hpp"
#include "legacy/scenario/MaszynaLegacyLightsAction.hpp"
#include "legacy/scenario/MaszynaLegacyMemoryAction.hpp"
#include "legacy/scenario/MaszynaLegacyMultipleAction.hpp"
#include "legacy/scenario/MaszynaLegacySwitchAction.hpp"
#include "legacy/scenario/MaszynaLegacyTrackVelocityAction.hpp"
#include "legacy/scenario/MaszynaLegacyVehicleCommandAction.hpp"
#include "legacy/scenario/MaszynaLegacyVoltageAction.hpp"
#include "legacy/scenery/MaszynaLegacySBTTerrainProvider.hpp"
#include "legacy/scenery/MaszynaTrianglesChunkGeometry.hpp"
#include "legacy/scenery/MaszynaTrianglesImporter.hpp"
#include "legacy/signalling/MaszynaLegacySignalHeadKindFactory.hpp"
#include "legacy/signalling/MaszynaLegacySignallingImplementation.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverRailVehicleAIHints.hpp"
#include "legacy/vehicles/MoverRailVehicleBrake.hpp"
#include "legacy/vehicles/MoverRailVehicleBuffCoupl.hpp"
#include "legacy/vehicles/MoverRailVehicleController.hpp"
#include "legacy/vehicles/MoverRailVehicleDieselElectricEngine.hpp"
#include "legacy/vehicles/MoverRailVehicleDieselEngine.hpp"
#include "legacy/vehicles/MoverRailVehicleDoors.hpp"
#include "legacy/vehicles/MoverRailVehicleElectricInductionEngine.hpp"
#include "legacy/vehicles/MoverRailVehicleElectricSeriesEngine.hpp"
#include "legacy/vehicles/MoverRailVehicleElectroPneumaticDynamicBrake.hpp"
#include "legacy/vehicles/MoverRailVehicleEnginePowerSource.hpp"
#include "legacy/vehicles/MoverRailVehicleHeating.hpp"
#include "legacy/vehicles/MoverRailVehicleHorns.hpp"
#include "legacy/vehicles/MoverRailVehicleLighting.hpp"
#include "legacy/vehicles/MoverRailVehicleLoad.hpp"
#include "legacy/vehicles/MoverRailVehicleMasterController.hpp"
#include "legacy/vehicles/MoverRailVehiclePowerSupply.hpp"
#include "legacy/vehicles/MoverRailVehicleRadio.hpp"
#include "legacy/vehicles/MoverRailVehicleSecuritySystem.hpp"
#include "legacy/vehicles/MoverRailVehicleSpeedControl.hpp"
#include "legacy/vehicles/MoverRailVehicleSpringBrake.hpp"
#include "legacy/vehicles/MoverRailVehicleSwitches.hpp"
#include "legacy/vehicles/MoverRailVehicleUniversalController.hpp"
#include "legacy/vehicles/MoverRailVehicleWheels.hpp"
#include "legacy/vehicles/MoverRailVehicleWipers.hpp"
#include "loaders/OggVorbisFormatLoader.hpp"
#include "logging/GameLog.hpp"
#include "logging/GameLogFileHandler.hpp"
#include "logging/GameLogger.hpp"
#include "person/PersonServer.hpp"
#include "player/PlayerCameraServer.hpp"
#include "player/PlayerServer.hpp"
#include "register_types.h"
#include "rendering/PlanarMirror3D.hpp"
#include "resources/ResourceLazyLoader.hpp"
#include "scenario/ScenarioEventAction.hpp"
#include "scenario/ScenarioEventCondition.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "scenario/Timetable.hpp"
#include "scenario/TimetableEntry.hpp"
#include "scenery/SceneryHUDMouseServer.hpp"
#include "scenery/SceneryModelPlacement.hpp"
#include "scenery/ScenerySoundPlacement.hpp"
#include "scenery/SceneryStreamingProvider.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "scenery/SceneryTrianglesSink.hpp"
#include "scripting/ScenarioScriptAction.hpp"
#include "scripting/ScenarioScriptCabinImplementation.hpp"
#include "scripting/ScenarioScriptServer.hpp"
#include "signalling/SignalAspect.hpp"
#include "signalling/SignalHeadKind.hpp"
#include "signalling/SignalHeadNode.hpp"
#include "signalling/SignallingImplementation.hpp"
#include "signalling/SignallingServer.hpp"
#include "signalling/SignallingSystemNode.hpp"
#include "simulation/SimulationRuntime.hpp"
#include "simulation/SimulationServer.hpp"
#include "station/StationServer.hpp"
#include "tracks/SpatialIndex.hpp"
#include "tracks/TrackEndpointRef.hpp"
#include "tracks/TrackServer.hpp"
#include "traction/TractionServer.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "utils/MaszynaTranslationServer.hpp"
#include "utils/ProcessMemory.hpp"
#include "utils/UserSettings.hpp"
#include "utils/WorkerTaskQueue.hpp"
#include "vehicles/base/GenericVehicleComponent.hpp"
#include "vehicles/base/GenericVehicleComponentNode.hpp"
#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/base/VehicleComponentType.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleCurvePointItem.hpp"
#include "vehicles/base/VehicleImplementationServer.hpp"
#include "vehicles/base/VehiclePerson.hpp"
#include "vehicles/base/VehiclePersonRole.hpp"
#include "vehicles/base/VehiclePhysicsNode.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicle3D.hpp"
#include "vehicles/rail/RailVehicleAIHints.hpp"
#include "vehicles/rail/RailVehicleAppearance.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include "vehicles/rail/RailVehicleBrakePressureTableItem.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"
#include "vehicles/rail/RailVehicleCabinKind.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"
#include "vehicles/rail/RailVehicleCompressorListItem.hpp"
#include "vehicles/rail/RailVehicleDieselElectricEngine.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleDimmerListItem.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"
#include "vehicles/rail/RailVehicleElectricInductionEngine.hpp"
#include "vehicles/rail/RailVehicleElectricSeriesEngine.hpp"
#include "vehicles/rail/RailVehicleElectroPneumaticDynamicBrake.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehicleHeating.hpp"
#include "vehicles/rail/RailVehicleHorns.hpp"
#include "vehicles/rail/RailVehicleInverter.hpp"
#include "vehicles/rail/RailVehicleLightListItem.hpp"
#include "vehicles/rail/RailVehicleLighting.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include "vehicles/rail/RailVehicleLoadListItem.hpp"
#include "vehicles/rail/RailVehicleMasterController.hpp"
#include "vehicles/rail/RailVehicleMotorParameter.hpp"
#include "vehicles/rail/RailVehicleNeighbour.hpp"
#include "vehicles/rail/RailVehiclePhysicsNode.hpp"
#include "vehicles/rail/RailVehiclePowerSupply.hpp"
#include "vehicles/rail/RailVehicleRadio.hpp"
#include "vehicles/rail/RailVehicleRelayListItem.hpp"
#include "vehicles/rail/RailVehicleRenderingServer.hpp"
#include "vehicles/rail/RailVehicleSecuritySystem.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include "vehicles/rail/RailVehicleSpeedControl.hpp"
#include "vehicles/rail/RailVehicleSpringBrake.hpp"
#include "vehicles/rail/RailVehicleSwitches.hpp"
#include "vehicles/rail/RailVehicleThrottlePositionItem.hpp"
#include "vehicles/rail/RailVehicleUniversalController.hpp"
#include "vehicles/rail/RailVehicleUniversalControllerListItem.hpp"
#include "vehicles/rail/RailVehicleWWListItem.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"
#include "vehicles/rail/RailVehicleWiperListItem.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

GameLog *game_log_singleton = nullptr;
E3DParser *e3d_parser_singleton = nullptr;
UserSettings *user_settings_singleton = nullptr;
SimulationServer *simulation_server_singleton = nullptr;
GameDataServer *game_data_server_singleton = nullptr;
ResourceLazyLoader *resource_lazy_loader_singleton = nullptr;
E3DRenderingServer *e3d_rendering_server_singleton = nullptr;
TrackServer *track_server_singleton = nullptr;
PersonServer *person_server_singleton = nullptr;
VehicleServer *vehicle_server_singleton = nullptr;
MaszynaMoverVehicleServer *maszyna_mover_vehicle_server_singleton = nullptr;
RailVehicleServer *rail_vehicle_server_singleton = nullptr;
RailVehicleRenderingServer *rail_vehicle_rendering_server_singleton = nullptr;
TractionServer *traction_server_singleton = nullptr;
SceneryStreamingServer *scenery_streaming_server_singleton = nullptr;
PythonScreenServer *python_screen_server_singleton = nullptr;
MaszynaTranslationServer *maszyna_translation_server_singleton = nullptr;
CabinHUDMouseSystem *cabin_hud_mouse_system_singleton = nullptr;
SignallingServer *signalling_server_singleton = nullptr;
ScenarioEventServer *scenario_event_server_singleton = nullptr;
SceneryHUDMouseServer *scenery_hud_mouse_server_singleton = nullptr;
ScenarioScriptServer *scenario_script_server_singleton = nullptr;
DriverServer *driver_server_singleton = nullptr;
PlayerServer *player_server_singleton = nullptr;
StationServer *station_server_singleton = nullptr;
PlayerCameraServer *player_camera_server_singleton = nullptr;
HUDServer *hud_server_singleton = nullptr;
Ref<E3DResourceFormatLoader> e3d_resource_format_loader;
Ref<OggVorbisFormatLoader> ogg_vorbis_format_loader;

void initialize_libmaszyna_module(const ModuleInitializationLevel p_level) {
    UtilityFunctions::print("Initializing libmaszyna module on level " + String::num(p_level) + "...");

    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        //         GDREGISTER_CLASS(DieselEngineMasterControllerPowerItemEditor);
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_CLASS(UserSettings);
        GDREGISTER_CLASS(ProcessMemory);
        GDREGISTER_CLASS(SimulationServer);
        GDREGISTER_CLASS(SimulationRuntime);
        GDREGISTER_CLASS(GameDataServer);
        GDREGISTER_CLASS(MaszynaTranslationServer);
        GDREGISTER_CLASS(ResourceCache);
        GDREGISTER_CLASS(ResourceLazyLoader);
        GDREGISTER_CLASS(E3DSubModel);
        GDREGISTER_CLASS(E3DModel);
        GDREGISTER_CLASS(E3DParser);
        GDREGISTER_CLASS(T3DParser);
        GDREGISTER_CLASS(E3DModelLightDefinition);
        GDREGISTER_CLASS(E3DModelSmokeSourceDefinition);
        GDREGISTER_CLASS(E3DRenderingServer);
        GDREGISTER_CLASS(PlanarMirror3D);
        GDREGISTER_CLASS(E3DResourceFormatLoader);
        GDREGISTER_CLASS(PersonServer);
        GDREGISTER_CLASS(VehicleServer);
        GDREGISTER_CLASS(VehiclePerson);
        GDREGISTER_ABSTRACT_CLASS(VehicleImplementationServer);
        GDREGISTER_CLASS(MaszynaMoverVehicleServer);
        GDREGISTER_CLASS(RailVehicleServer);
        GDREGISTER_CLASS(RailVehicleNeighbour);
        GDREGISTER_CLASS(RailVehicleAppearance);
        GDREGISTER_CLASS(RailVehicleRenderingServer);
        GDREGISTER_CLASS(TractionServer);
        GDREGISTER_CLASS(SpatialIndex);
        GDREGISTER_CLASS(TrackEndpointRef);
        GDREGISTER_CLASS(TrackRouteSegment);
        GDREGISTER_CLASS(TrackBranchNeighbors);
        GDREGISTER_CLASS(TrackServer);
        GDREGISTER_CLASS(SignallingServer);
        GDREGISTER_CLASS(SignalAspect);
        GDREGISTER_CLASS(SignalHeadKind);
        GDREGISTER_ABSTRACT_CLASS(MaszynaLegacySignalHeadKindFactory);
        GDREGISTER_VIRTUAL_CLASS(SignallingImplementation);
        GDREGISTER_CLASS(MaszynaLegacySignallingImplementation);
        GDREGISTER_CLASS(SignalHeadNode);
        GDREGISTER_CLASS(SignallingSystemNode);
        GDREGISTER_CLASS(ScenarioEventServer);
        GDREGISTER_CLASS(SceneryHUDMouseServer);
        GDREGISTER_CLASS(DriverServer);
        GDREGISTER_VIRTUAL_CLASS(DriverImplementation);
        GDREGISTER_VIRTUAL_CLASS(ScenarioEventAction);
        GDREGISTER_VIRTUAL_CLASS(ScenarioEventCondition);
        GDREGISTER_CLASS(ScenarioScriptServer);
        GDREGISTER_VIRTUAL_CLASS(ScenarioScriptCabinImplementation);
        GDREGISTER_CLASS(PlayerServer);
        GDREGISTER_CLASS(StationServer);
        GDREGISTER_CLASS(PlayerCameraServer);
        GDREGISTER_CLASS(HUDServer);
        GDREGISTER_INTERNAL_CLASS(ScenarioScriptAction);
        GDREGISTER_CLASS(MaszynaLegacyMemoryAction);
        GDREGISTER_CLASS(MaszynaLegacyMultipleAction);
        GDREGISTER_CLASS(MaszynaLegacyLightsAction);
        GDREGISTER_CLASS(MaszynaLegacySwitchAction);
        GDREGISTER_CLASS(MaszynaLegacyVoltageAction);
        GDREGISTER_CLASS(MaszynaLegacyTrackVelocityAction);
        GDREGISTER_CLASS(MaszynaLegacyAnimationAction);
        GDREGISTER_CLASS(MaszynaLegacyVehicleCommandAction);
        GDREGISTER_CLASS(TimetableEntry);
        GDREGISTER_CLASS(Timetable);
        GDREGISTER_CLASS(MaszynaLegacyEventCondition);
        GDREGISTER_ABSTRACT_CLASS(MaszynaDataPath);
        GDREGISTER_ABSTRACT_CLASS(LegacyCabinLampIslands);
        GDREGISTER_CLASS(MaszynaParser);
        GDREGISTER_CLASS(MaszynaTrianglesImporter);
        GDREGISTER_CLASS(MaszynaTrianglesChunkGeometry);
        GDREGISTER_CLASS(WorkerTaskQueue);
        GDREGISTER_VIRTUAL_CLASS(SceneryStreamingProvider);
        // after the provider it implements
        GDREGISTER_CLASS(MaszynaLegacySBTTerrainProvider);
        GDREGISTER_CLASS(SceneryModelPlacement);
        GDREGISTER_CLASS(ScenerySoundPlacement);
        GDREGISTER_CLASS(SceneryStreamingServer);
        GDREGISTER_CLASS(PythonScreenServer);
        GDREGISTER_CLASS(SceneryTrianglesSink);
        GDREGISTER_CLASS(OggVorbisFormatLoader);
        GDREGISTER_ABSTRACT_CLASS(VehicleComponentType);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleComponentType);
        GDREGISTER_ABSTRACT_CLASS(VehiclePersonRole);
        GDREGISTER_ABSTRACT_CLASS(LibMaszynaUnits);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleCabinKind);
        GDREGISTER_CLASS(VehiclePhysicsNode);
        GDREGISTER_CLASS(RailVehiclePhysicsNode);
        GDREGISTER_ABSTRACT_CLASS(VehicleComponent);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleComponent);
        GDREGISTER_CLASS(GenericVehicleComponent);
        GDREGISTER_CLASS(GenericVehicleComponentNode);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleBrake);
        GDREGISTER_CLASS(MoverRailVehicleBrake);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSpringBrake);
        GDREGISTER_CLASS(MoverRailVehicleSpringBrake);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleDoors);
        GDREGISTER_CLASS(MoverRailVehicleDoors);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleDieselEngine);
        GDREGISTER_CLASS(MoverRailVehicleDieselEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleDieselElectricEngine);
        GDREGISTER_CLASS(MoverRailVehicleDieselElectricEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectricEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectricSeriesEngine);
        GDREGISTER_CLASS(MoverRailVehicleElectricSeriesEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectricInductionEngine);
        GDREGISTER_CLASS(MoverRailVehicleElectricInductionEngine);
        GDREGISTER_ABSTRACT_CLASS(VehicleController);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleController);
        GDREGISTER_CLASS(MoverRailVehicleController);
        // the vehicles are simulated on the vendored Mover
        VehiclePhysicsNode::set_controller_implementation(MoverRailVehicleController::get_class_static());
        GDREGISTER_CLASS(Cabin3D);
        GDREGISTER_CLASS(CabinHUDMouseSystem);
        GDREGISTER_CLASS(RailVehicle3D);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleHeating);
        GDREGISTER_CLASS(MoverRailVehicleHeating);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleRadio);
        GDREGISTER_CLASS(MoverRailVehicleRadio);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleWheels);
        GDREGISTER_CLASS(MoverRailVehicleWheels);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSecuritySystem);
        GDREGISTER_CLASS(MoverRailVehicleSecuritySystem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleHorns);
        GDREGISTER_CLASS(MoverRailVehicleHorns);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleAIHints);
        GDREGISTER_CLASS(MoverRailVehicleAIHints);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleLighting);
        GDREGISTER_CLASS(MoverRailVehicleLighting);
        GDREGISTER_CLASS(GameLog);
        GDREGISTER_CLASS(GameLogger);
        GDREGISTER_CLASS(GameLogHandler);
        GDREGISTER_CLASS(GameLogFileHandler);
        GDREGISTER_CLASS(RailVehicleWWListItem);
        GDREGISTER_CLASS(RailVehicleInverter);
        GDREGISTER_CLASS(RailVehicleMotorParameter);
        GDREGISTER_CLASS(RailVehicleLightListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectroPneumaticDynamicBrake);
        GDREGISTER_CLASS(MoverRailVehicleElectroPneumaticDynamicBrake);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleLoad);
        GDREGISTER_CLASS(MoverRailVehicleLoad);
        GDREGISTER_CLASS(RailVehicleLoadListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleBuffCoupl);
        GDREGISTER_CLASS(MoverRailVehicleBuffCoupl);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSpeedControl);
        GDREGISTER_CLASS(MoverRailVehicleSpeedControl);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleUniversalController);
        GDREGISTER_CLASS(MoverRailVehicleUniversalController);
        GDREGISTER_CLASS(RailVehicleUniversalControllerListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleMasterController);
        GDREGISTER_CLASS(MoverRailVehicleMasterController);
        GDREGISTER_ABSTRACT_CLASS(RailVehiclePowerSupply);
        GDREGISTER_CLASS(MoverRailVehiclePowerSupply);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleEnginePowerSource);
        GDREGISTER_CLASS(MoverRailVehicleEnginePowerSource);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleWipers);
        GDREGISTER_CLASS(MoverRailVehicleWipers);
        GDREGISTER_CLASS(RailVehicleWiperListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSwitches);
        GDREGISTER_CLASS(MoverRailVehicleSwitches);
        GDREGISTER_CLASS(RailVehicleDimmerListItem);
        GDREGISTER_CLASS(RailVehicleBrakePressureTableItem);
        GDREGISTER_CLASS(RailVehicleCompressorListItem);
        GDREGISTER_CLASS(RailVehicleRelayListItem);
        GDREGISTER_CLASS(VehicleCurvePointItem);
        GDREGISTER_CLASS(RailVehicleThrottlePositionItem);

        user_settings_singleton = memnew(UserSettings);
        Engine::get_singleton()->register_singleton("UserSettings", user_settings_singleton); // 1
        // after UserSettings is registered: the constructor follows the game directory; before
        // every server whose constructor creates a ResourceCache, which follows its cache clearing
        game_data_server_singleton = memnew(GameDataServer);
        Engine::get_singleton()->register_singleton("GameDataServer", game_data_server_singleton); // 1a
        // after GameDataServer is registered: the constructor follows its unloading; before the
        // streamed servers, which load through it
        resource_lazy_loader_singleton = memnew(ResourceLazyLoader);
        Engine::get_singleton()->register_singleton("ResourceLazyLoader", resource_lazy_loader_singleton); // 1b
        simulation_server_singleton = memnew(SimulationServer);
        game_log_singleton = memnew(GameLog);
        e3d_parser_singleton = memnew(E3DParser);
        scenery_streaming_server_singleton = memnew(SceneryStreamingServer);
        track_server_singleton = memnew(TrackServer);
        python_screen_server_singleton = memnew(PythonScreenServer);
        cabin_hud_mouse_system_singleton = memnew(CabinHUDMouseSystem);

        Engine::get_singleton()->register_singleton("E3DParser", e3d_parser_singleton);                            // 2
        Engine::get_singleton()->register_singleton("GameLog", game_log_singleton);                                // 3
        Engine::get_singleton()->register_singleton("SceneryStreamingServer", scenery_streaming_server_singleton); // 5
        // after SceneryStreamingServer is registered: the constructor takes the models providers supply
        e3d_rendering_server_singleton = memnew(E3DRenderingServer);
        Engine::get_singleton()->register_singleton("E3DRenderingServer", e3d_rendering_server_singleton); // 6
        Engine::get_singleton()->register_singleton("SimulationServer", simulation_server_singleton);      // 7
        Engine::get_singleton()->register_singleton("TrackServer", track_server_singleton);                // 8
        /* after SimulationServer is registered and before VehicleServer: the constructor follows
         * its clock first, so a slice ticks the power sources before the vehicles draw from them -
         * the original's order (simulation.cpp:115-116) */
        traction_server_singleton = memnew(TractionServer);
        // after SimulationServer is registered: the constructor follows its clock
        person_server_singleton = memnew(PersonServer);
        Engine::get_singleton()->register_singleton("PersonServer", person_server_singleton); // 8b
        // after PersonServer is registered: the constructor follows its freed persons
        vehicle_server_singleton = memnew(VehicleServer);
        Engine::get_singleton()->register_singleton("VehicleServer", vehicle_server_singleton); // 9
        // after VehicleServer is registered: the constructor follows the vehicles' lifetime
        rail_vehicle_server_singleton = memnew(RailVehicleServer);
        Engine::get_singleton()->register_singleton("RailVehicleServer", rail_vehicle_server_singleton); // 10
        maszyna_mover_vehicle_server_singleton = memnew(MaszynaMoverVehicleServer);
        Engine::get_singleton()->register_singleton(
                "MaszynaMoverVehicleServer", maszyna_mover_vehicle_server_singleton); // 10a
        vehicle_server_singleton->implementation_register(
                MaszynaMoverVehicleServer::IMPLEMENTATION_NAME,
                maszyna_mover_vehicle_server_singleton->get_instance_id());
        Engine::get_singleton()->register_singleton("TractionServer", traction_server_singleton);          // 11
        Engine::get_singleton()->register_singleton("PythonScreenServer", python_screen_server_singleton); // 12
        // after UserSettings is registered: the constructor reads the game directory from it
        maszyna_translation_server_singleton = memnew(MaszynaTranslationServer);
        Engine::get_singleton()->register_singleton(
                "MaszynaTranslationServer", maszyna_translation_server_singleton);                            // 13
        Engine::get_singleton()->register_singleton("CabinHUDMouseSystem", cabin_hud_mouse_system_singleton); // 14
        // after E3DRenderingServer is registered: the constructor follows its freed instances
        signalling_server_singleton = memnew(SignallingServer);
        Engine::get_singleton()->register_singleton("SignallingServer", signalling_server_singleton); // 15
        // after SimulationServer is registered: the constructor follows its pause and speed
        scenario_event_server_singleton = memnew(ScenarioEventServer);
        Engine::get_singleton()->register_singleton("ScenarioEventServer", scenario_event_server_singleton); // 16
        // after RailVehicleServer is registered: the constructor follows its freed vehicles
        driver_server_singleton = memnew(DriverServer);
        Engine::get_singleton()->register_singleton("DriverServer", driver_server_singleton); // 17
        // after E3DRenderingServer is registered: it outlines and picks its instances
        scenery_hud_mouse_server_singleton = memnew(SceneryHUDMouseServer);
        Engine::get_singleton()->register_singleton("SceneryHUDMouseServer", scenery_hud_mouse_server_singleton); // 18
        // after RailVehicleServer, E3DRenderingServer and SceneryHUDMouseServer: the constructor
        // follows their vehicles and instances, and it registers what the player picks
        rail_vehicle_rendering_server_singleton = memnew(RailVehicleRenderingServer);
        Engine::get_singleton()->register_singleton(
                "RailVehicleRenderingServer", rail_vehicle_rendering_server_singleton); // 18a
        // after VehicleServer: the constructor follows its freed vehicles
        station_server_singleton = memnew(StationServer);
        Engine::get_singleton()->register_singleton("StationServer", station_server_singleton); // 18b
        // after RailVehicleServer and DriverServer: it follows freed vehicles and hands trainsets
        // over to their drivers
        player_server_singleton = memnew(PlayerServer);
        Engine::get_singleton()->register_singleton("PlayerServer", player_server_singleton); // 19
        // after PlayerServer: the view follows what the player drives
        player_camera_server_singleton = memnew(PlayerCameraServer);
        Engine::get_singleton()->register_singleton("PlayerCameraServer", player_camera_server_singleton); // 20
        hud_server_singleton = memnew(HUDServer);
        Engine::get_singleton()->register_singleton("HUDServer", hud_server_singleton); // 21
        // last: the constructor follows what the servers above report to the scripts
        scenario_script_server_singleton = memnew(ScenarioScriptServer);
        Engine::get_singleton()->register_singleton("ScenarioScriptServer", scenario_script_server_singleton); // 22

        e3d_resource_format_loader.instantiate();
        ogg_vorbis_format_loader.instantiate();
        ResourceLoader::get_singleton()->add_resource_format_loader(e3d_resource_format_loader);
        ResourceLoader::get_singleton()->add_resource_format_loader(ogg_vorbis_format_loader);
    }
}

void uninitialize_libmaszyna_module(const ModuleInitializationLevel p_level) {
    UtilityFunctions::print("De-initializing libmaszyna module on level " + String::num(p_level) + "...");

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    if (ogg_vorbis_format_loader.is_valid()) {
        ResourceLoader::get_singleton()->remove_resource_format_loader(ogg_vorbis_format_loader);
        ogg_vorbis_format_loader.unref();
    }

    if (e3d_resource_format_loader.is_valid()) {
        ResourceLoader::get_singleton()->remove_resource_format_loader(e3d_resource_format_loader);
        e3d_resource_format_loader.unref();
    }

    // in reverse order of creation, which is not the order of registration: a singleton created
    // early and registered late (SimulationServer) still outlives the ones created after it
    if (Engine::get_singleton()->has_singleton("ScenarioScriptServer")) {
        Engine::get_singleton()->unregister_singleton("ScenarioScriptServer"); // 22
    }
    if (scenario_script_server_singleton != nullptr) {
        memdelete(scenario_script_server_singleton);
        scenario_script_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("HUDServer")) {
        Engine::get_singleton()->unregister_singleton("HUDServer"); // 21
    }
    if (hud_server_singleton != nullptr) {
        memdelete(hud_server_singleton);
        hud_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PlayerCameraServer")) {
        Engine::get_singleton()->unregister_singleton("PlayerCameraServer"); // 20
    }
    if (player_camera_server_singleton != nullptr) {
        memdelete(player_camera_server_singleton);
        player_camera_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PlayerServer")) {
        Engine::get_singleton()->unregister_singleton("PlayerServer"); // 19
    }
    if (player_server_singleton != nullptr) {
        memdelete(player_server_singleton);
        player_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("StationServer")) {
        Engine::get_singleton()->unregister_singleton("StationServer"); // 18b
    }
    if (station_server_singleton != nullptr) {
        memdelete(station_server_singleton);
        station_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("RailVehicleRenderingServer")) {
        Engine::get_singleton()->unregister_singleton("RailVehicleRenderingServer"); // 18a
    }
    if (rail_vehicle_rendering_server_singleton != nullptr) {
        memdelete(rail_vehicle_rendering_server_singleton);
        rail_vehicle_rendering_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SceneryHUDMouseServer")) {
        Engine::get_singleton()->unregister_singleton("SceneryHUDMouseServer"); // 18
    }
    if (scenery_hud_mouse_server_singleton != nullptr) {
        memdelete(scenery_hud_mouse_server_singleton);
        scenery_hud_mouse_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("DriverServer")) {
        Engine::get_singleton()->unregister_singleton("DriverServer"); // 17
    }
    if (driver_server_singleton != nullptr) {
        memdelete(driver_server_singleton);
        driver_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("ScenarioEventServer")) {
        Engine::get_singleton()->unregister_singleton("ScenarioEventServer"); // 16
    }
    if (scenario_event_server_singleton != nullptr) {
        memdelete(scenario_event_server_singleton);
        scenario_event_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SignallingServer")) {
        Engine::get_singleton()->unregister_singleton("SignallingServer"); // 15
    }
    if (signalling_server_singleton != nullptr) {
        memdelete(signalling_server_singleton);
        signalling_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("MaszynaTranslationServer")) {
        Engine::get_singleton()->unregister_singleton("MaszynaTranslationServer"); // 13
    }
    if (maszyna_translation_server_singleton != nullptr) {
        memdelete(maszyna_translation_server_singleton);
        maszyna_translation_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("MaszynaMoverVehicleServer")) {
        Engine::get_singleton()->unregister_singleton("MaszynaMoverVehicleServer"); // 10a
    }
    if (maszyna_mover_vehicle_server_singleton != nullptr) {
        if (vehicle_server_singleton != nullptr) {
            vehicle_server_singleton->implementation_unregister(MaszynaMoverVehicleServer::IMPLEMENTATION_NAME);
        }
        memdelete(maszyna_mover_vehicle_server_singleton);
        maszyna_mover_vehicle_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("RailVehicleServer")) {
        Engine::get_singleton()->unregister_singleton("RailVehicleServer"); // 10
    }
    if (rail_vehicle_server_singleton != nullptr) {
        memdelete(rail_vehicle_server_singleton);
        rail_vehicle_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("VehicleServer")) {
        Engine::get_singleton()->unregister_singleton("VehicleServer"); // 9
    }
    if (vehicle_server_singleton != nullptr) {
        memdelete(vehicle_server_singleton);
        vehicle_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PersonServer")) {
        Engine::get_singleton()->unregister_singleton("PersonServer"); // 8b
    }
    if (person_server_singleton != nullptr) {
        memdelete(person_server_singleton);
        person_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("TractionServer")) {
        Engine::get_singleton()->unregister_singleton("TractionServer"); // 11
    }
    if (traction_server_singleton != nullptr) {
        memdelete(traction_server_singleton);
        traction_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("E3DRenderingServer")) {
        Engine::get_singleton()->unregister_singleton("E3DRenderingServer"); // 6
    }
    if (e3d_rendering_server_singleton != nullptr) {
        memdelete(e3d_rendering_server_singleton);
        e3d_rendering_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("CabinHUDMouseSystem")) {
        Engine::get_singleton()->unregister_singleton("CabinHUDMouseSystem"); // 14
    }
    if (cabin_hud_mouse_system_singleton != nullptr) {
        memdelete(cabin_hud_mouse_system_singleton);
        cabin_hud_mouse_system_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PythonScreenServer")) {
        Engine::get_singleton()->unregister_singleton("PythonScreenServer"); // 12
    }
    if (python_screen_server_singleton != nullptr) {
        memdelete(python_screen_server_singleton);
        python_screen_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("TrackServer")) {
        Engine::get_singleton()->unregister_singleton("TrackServer"); // 8
    }
    if (track_server_singleton != nullptr) {
        memdelete(track_server_singleton);
        track_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SceneryStreamingServer")) {
        Engine::get_singleton()->unregister_singleton("SceneryStreamingServer"); // 5
    }
    if (scenery_streaming_server_singleton != nullptr) {
        memdelete(scenery_streaming_server_singleton);
        scenery_streaming_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("E3DParser")) {
        Engine::get_singleton()->unregister_singleton("E3DParser"); // 2
    }
    if (e3d_parser_singleton != nullptr) {
        memdelete(e3d_parser_singleton);
        e3d_parser_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("GameLog")) {
        Engine::get_singleton()->unregister_singleton("GameLog"); // 3
    }
    if (game_log_singleton != nullptr) {
        memdelete(game_log_singleton);
        game_log_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SimulationServer")) {
        Engine::get_singleton()->unregister_singleton("SimulationServer"); // 7
    }
    if (simulation_server_singleton != nullptr) {
        memdelete(simulation_server_singleton);
        simulation_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("ResourceLazyLoader")) {
        Engine::get_singleton()->unregister_singleton("ResourceLazyLoader"); // 1b
    }
    if (resource_lazy_loader_singleton != nullptr) {
        memdelete(resource_lazy_loader_singleton);
        resource_lazy_loader_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("GameDataServer")) {
        Engine::get_singleton()->unregister_singleton("GameDataServer"); // 1a
    }
    if (game_data_server_singleton != nullptr) {
        memdelete(game_data_server_singleton);
        game_data_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("UserSettings")) {
        Engine::get_singleton()->unregister_singleton("UserSettings"); // 1
    }
    if (user_settings_singleton != nullptr) {
        memdelete(user_settings_singleton);
        user_settings_singleton = nullptr;
    }
}
extern "C" {
    // Initialization.
    GDExtensionBool GDE_EXPORT libmaszyna_library_init(
            const GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library,
            GDExtensionInitialization *p_r_initialization) {
        const GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_r_initialization);

        init_obj.register_initializer(initialize_libmaszyna_module);
        init_obj.register_terminator(uninitialize_libmaszyna_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
