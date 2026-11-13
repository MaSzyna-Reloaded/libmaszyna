@tool
extends EditorPlugin

## The MaSzyna plugin's addon: its autoloads, its node types, the FIZ importer and the project
## settings only its own code reads

const PLUGIN_NAME = "libmaszyna"

var e3d_model_instance_script = preload("e3d/maszyna_e3d_model_instance.gd")
var e3d_model_instance_icon = preload("e3d/e3d_model_instance.png")
var maszyna_rail_vehicle_physics_node_script = preload("vehicle/maszyna_rail_vehicle_physics_node.gd")
var fiz_import_plugin = preload("fiz/fiz_import_plugin.gd").new()
var python_screens_export_plugin = preload("editor/python_screens_export_plugin.gd").new()

func _enable_plugin():
    add_autoload_singleton("MaszynaMaterialManager", "res://addons/libmaszyna/materials/maszyna_material_manager.gd")
    add_autoload_singleton("AudioStreamManager", "res://addons/libmaszyna/sound/audio_stream_manager.gd")
    add_autoload_singleton("MaszynaMaterialParser", "res://addons/libmaszyna/materials/maszyna_material_parser.gd")
    add_autoload_singleton("MaszynaMaterialFactory", "res://addons/libmaszyna/materials/maszyna_material_factory.gd")
    add_autoload_singleton("MaszynaE3DModelManager", "res://addons/libmaszyna/e3d/maszyna_e3d_model_manager.gd")
    add_autoload_singleton("MaszynaE3DModelTool", "res://addons/libmaszyna/e3d/maszyna_e3d_model_tool.gd")
    add_autoload_singleton("MaszynaVehicleSystem", "res://addons/libmaszyna/vehicle/maszyna_vehicle_system.gd")
    add_autoload_singleton("MaszynaVehicleProfileManager", "res://addons/libmaszyna/vehicle/maszyna_vehicle_profile_manager.gd")
    add_autoload_singleton("FIZResourceLoaderRegistrar", "res://addons/libmaszyna/fiz/fiz_resource_loader_registrar.gd")
    add_autoload_singleton("MaszynaSceneryChunkRenderingServer", "res://addons/libmaszyna/scenery/maszyna_scenery_chunk_rendering_server.gd")
    add_autoload_singleton("MaszynaSmokeSourceLibrary", "res://addons/libmaszyna/smoke/maszyna_smoke_source_library.gd")
    add_autoload_singleton("MaszynaConsoleCommands", "res://addons/libmaszyna/console/maszyna_console_commands.gd")

    add_custom_type(
        "MaszynaE3DModelInstance",
        "ReloadedModel3D",
        e3d_model_instance_script,
        e3d_model_instance_icon,
    )

    add_custom_type(
        "MaszynaRailVehiclePhysicsNode",
        "Node",
        maszyna_rail_vehicle_physics_node_script,
        null
    )

    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/e3d_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/fiz_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/nodebank", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scenery_info", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/maszyna_vehicles", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scn_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/user_settings_dock", true)

func _disable_plugin():
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/e3d_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/fiz_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/nodebank", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scenery_info", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/maszyna_vehicles", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scn_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/user_settings_dock", false)

    remove_custom_type("MaszynaE3DModelInstance")
    remove_custom_type("MaszynaRailVehiclePhysicsNode")

    remove_autoload_singleton("MaszynaConsoleCommands")
    remove_autoload_singleton("FIZResourceLoaderRegistrar")
    remove_autoload_singleton("MaszynaVehicleProfileManager")
    remove_autoload_singleton("MaszynaVehicleSystem")
    remove_autoload_singleton("MaszynaE3DModelTool")
    remove_autoload_singleton("MaszynaE3DModelManager")
    remove_autoload_singleton("MaszynaMaterialFactory")
    remove_autoload_singleton("MaszynaMaterialManager")
    remove_autoload_singleton("AudioStreamManager")
    remove_autoload_singleton("MaszynaMaterialParser")
    remove_autoload_singleton("MaszynaSceneryChunkRenderingServer")
    remove_autoload_singleton("MaszynaSmokeSourceLibrary")

func _enter_tree():
    MaszynaProjectSettings.register()
    add_import_plugin(fiz_import_plugin)
    add_export_plugin(python_screens_export_plugin)

func _exit_tree():
    remove_import_plugin(fiz_import_plugin)
    remove_export_plugin(python_screens_export_plugin)
