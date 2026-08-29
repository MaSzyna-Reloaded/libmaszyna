#include "brakes/TrainBrake.hpp"
#include "brakes/TrainElectroPneumaticDynamicBrake.hpp"
#include "brakes/TrainSpringBrake.hpp"
#include "buffers/TrainBuffCoupl.hpp"
#include "controllers/TrainUniversalController.hpp"
#include "core/GameLog.hpp"
#include "core/GenericTrainPart.hpp"
#include "core/ResourceCache.hpp"
#include "core/TrainController.hpp"
#include "core/TrainPart.hpp"
#include "core/TrainSystem.hpp"
#include "core/UserSettings.hpp"
#include "doors/TrainDoors.hpp"
#include "e3d/E3DModel.hpp"
#include "e3d/E3DModelLightDefinition.hpp"
#include "e3d/E3DSubModel.hpp"
#include "engines/TrainDieselElectricEngine.hpp"
#include "engines/TrainDieselEngine.hpp"
#include "engines/TrainElectricEngine.hpp"
#include "engines/TrainElectricInductionEngine.hpp"
#include "engines/TrainElectricSeriesEngine.hpp"
#include "engines/TrainEngine.hpp"
#include "heating/TrainHeating.hpp"
#include "lighting/TrainLighting.hpp"
#include "load/TrainLoad.hpp"
#include "loaders/E3DResourceFormatLoader.hpp"
#include "loaders/OggVorbisFormatLoader.hpp"
#include "parsers/e3d_parser.hpp"
#include "parsers/maszyna_parser.hpp"
#include "register_types.h"
#include "resources/brakes/BrakePressureTableItem.hpp"
#include "resources/brakes/CompressorListItem.hpp"
#include "resources/controllers/UniversalControllerListItem.hpp"
#include "resources/engines/CurvePointItem.hpp"
#include "resources/engines/MotorParameter.hpp"
#include "resources/engines/RelayListItem.hpp"
#include "resources/engines/ThrottlePositionItem.hpp"
#include "resources/engines/WWListItem.hpp"
#include "resources/lighting/LightListItem.hpp"
#include "resources/load/LoadListItem.hpp"
#include "resources/switches/DimmerListItem.hpp"
#include "resources/wipers/WiperListItem.hpp"
#include "speed_control/TrainSpeedControl.hpp"
#include "switches/TrainSwitches.hpp"
#include "systems/TrainAIHints.hpp"
#include "systems/TrainSecuritySystem.hpp"
#include "wheels/TrainWheels.hpp"
#include "wipers/TrainWipers.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

TrainSystem *train_system_singleton = nullptr;
GameLog *game_log_singleton = nullptr;
E3DParser *e3d_parser_singleton = nullptr;
UserSettings *user_settings_singleton = nullptr;
Ref<E3DResourceFormatLoader> e3d_resource_format_loader;
Ref<OggVorbisFormatLoader> ogg_vorbis_format_loader;

void initialize_libmaszyna_module(const ModuleInitializationLevel p_level) {
    UtilityFunctions::print("Initializing libmaszyna module on level " + String::num(p_level) + "...");

    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        //         GDREGISTER_CLASS(DieselEngineMasterControllerPowerItemEditor);
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_CLASS(UserSettings);
        GDREGISTER_CLASS(ResourceCache);
        GDREGISTER_CLASS(E3DSubModel);
        GDREGISTER_CLASS(E3DModel);
        GDREGISTER_CLASS(E3DParser);
        GDREGISTER_CLASS(E3DModelLightDefinition);
        GDREGISTER_CLASS(E3DResourceFormatLoader);
        GDREGISTER_CLASS(MaszynaParser);
        GDREGISTER_CLASS(OggVorbisFormatLoader);
        GDREGISTER_ABSTRACT_CLASS(TrainPart);
        GDREGISTER_CLASS(GenericTrainPart);
        GDREGISTER_CLASS(TrainBrake);
        GDREGISTER_CLASS(TrainSpringBrake);
        GDREGISTER_CLASS(TrainDoors);
        GDREGISTER_ABSTRACT_CLASS(TrainEngine);
        GDREGISTER_CLASS(TrainDieselEngine);
        GDREGISTER_CLASS(TrainDieselElectricEngine);
        GDREGISTER_ABSTRACT_CLASS(TrainElectricEngine);
        GDREGISTER_CLASS(TrainElectricSeriesEngine);
        GDREGISTER_CLASS(TrainElectricInductionEngine);
        GDREGISTER_CLASS(TrainController);
        GDREGISTER_CLASS(TrainHeating);
        GDREGISTER_CLASS(TrainWheels);
        GDREGISTER_CLASS(TrainSecuritySystem);
        GDREGISTER_CLASS(TrainAIHints);
        GDREGISTER_CLASS(TrainSystem);
        GDREGISTER_CLASS(TrainLighting)
        GDREGISTER_CLASS(GameLog);
        GDREGISTER_CLASS(WWListItem);
        GDREGISTER_CLASS(MotorParameter);
        GDREGISTER_CLASS(LightListItem)
        GDREGISTER_CLASS(TrainElectroPneumaticDynamicBrake)
        GDREGISTER_CLASS(TrainLoad)
        GDREGISTER_CLASS(LoadListItem)
        GDREGISTER_CLASS(TrainBuffCoupl)
        GDREGISTER_CLASS(TrainSpeedControl)
        GDREGISTER_CLASS(TrainUniversalController)
        GDREGISTER_CLASS(UniversalControllerListItem)
        GDREGISTER_CLASS(TrainWipers)
        GDREGISTER_CLASS(WiperListItem)
        GDREGISTER_CLASS(TrainSwitches)
        GDREGISTER_CLASS(DimmerListItem)
        GDREGISTER_CLASS(BrakePressureTableItem)
        GDREGISTER_CLASS(CompressorListItem)
        GDREGISTER_CLASS(RelayListItem)
        GDREGISTER_CLASS(CurvePointItem)
        GDREGISTER_CLASS(ThrottlePositionItem)

        user_settings_singleton = memnew(UserSettings);
        train_system_singleton = memnew(TrainSystem);
        game_log_singleton = memnew(GameLog);
        e3d_parser_singleton = memnew(E3DParser);

        Engine::get_singleton()->register_singleton("UserSettings", user_settings_singleton); // 1
        Engine::get_singleton()->register_singleton("E3DParser", e3d_parser_singleton);       // 2
        Engine::get_singleton()->register_singleton("GameLog", game_log_singleton);           // 3
        Engine::get_singleton()->register_singleton("TrainSystem", train_system_singleton);   // 4

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

    if (Engine::get_singleton()->has_singleton("TrainSystem")) {
        Engine::get_singleton()->unregister_singleton("TrainSystem"); // 4
    }

    if (Engine::get_singleton()->has_singleton("GameLog")) {
        Engine::get_singleton()->unregister_singleton("GameLog"); // 3
    }

    if (Engine::get_singleton()->has_singleton("E3DParser")) {
        Engine::get_singleton()->unregister_singleton("E3DParser"); // 2
    }

    if (Engine::get_singleton()->has_singleton("UserSettings")) {
        Engine::get_singleton()->unregister_singleton("UserSettings"); // 1
    }

    if (train_system_singleton != nullptr) { // 4
        memdelete(train_system_singleton);
        train_system_singleton = nullptr;
    }

    if (game_log_singleton != nullptr) { // 3
        memdelete(game_log_singleton);
        game_log_singleton = nullptr;
    }

    if (e3d_parser_singleton != nullptr) { // 2
        memdelete(e3d_parser_singleton);
        e3d_parser_singleton = nullptr;
    }

    if (user_settings_singleton != nullptr) { // 1
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
