@tool
extends EditorPlugin

## The MaSzyna plugin's addon: its autoloads, its node types, the FIZ importer and the project
## settings only its own code reads

const PLUGIN_NAME = "libmaszyna"

var e3d_model_instance_script = preload("e3d/maszyna_e3d_model_instance.gd")
var e3d_model_instance_icon = preload("e3d/e3d_model_instance.png")
var maszyna_rail_vehicle_physics_node_script = preload("vehicle/maszyna_rail_vehicle_physics_node.gd")
var fiz_import_plugin = preload("fiz/fiz_import_plugin.gd").new()

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
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/maszyna_vehicles", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scn_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/user_settings_dock", true)

func _disable_plugin():
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/e3d_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/fiz_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/nodebank", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/maszyna_vehicles", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scn_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/user_settings_dock", false)

    remove_custom_type("MaszynaE3DModelInstance")
    remove_custom_type("MaszynaRailVehiclePhysicsNode")

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
    add_custom_project_setting("maszyna/import/model_scale_factor", 1.0, TYPE_FLOAT)
    add_custom_project_setting(
        "maszyna/sound/culling_distance", 1000.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,5000.0,10.0,or_greater"
    )
    # The glow an indicator lamp throws into the cab while it is lit (an OmniLight3D at the lamp)
    add_custom_project_setting("maszyna/cabin/indicator_glow_enabled", true, TYPE_BOOL)
    add_custom_project_setting(
        "maszyna/cabin/indicator_glow_energy", 0.05, TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,2.0,0.005")
    add_custom_project_setting(
        "maszyna/cabin/indicator_glow_range", 0.1, TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.01,2.0,0.01,suffix:m")
    # A lamp of several pieces - three alerter lamps in one mesh, a backlight overlay per gauge -
    # gets a light at each piece, the backlight in each overlay's own colour
    add_custom_project_setting("maszyna/cabin/real_instruments_lights", true, TYPE_BOOL)
    # A Python cab screen throws a glow in the colour of what it shows - weak, but far enough to
    # light the desk around it
    add_custom_project_setting("maszyna/cabin/screen_glow_enabled", true, TYPE_BOOL)
    add_custom_project_setting(
        "maszyna/cabin/screen_glow_energy", 0.05, TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,2.0,0.005")
    add_custom_project_setting(
        "maszyna/cabin/screen_glow_range", 1.0, TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.05,4.0,0.05,suffix:m")
    add_custom_project_setting(
        "maszyna/cabin/instrument_glow_energy", 0.002, TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001")
    add_custom_project_setting(
        "maszyna/cabin/instrument_glow_range", 0.564628, TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.01,2.0,0.01,suffix:m")
    # CPython 2.7 prefix PythonScreenServer runs the cab screens with (lib/libpython2.7.so.1.0 on
    # Linux); empty is python2.7 in the game directory on Linux and the original's python64 on Windows
    add_custom_project_setting("maszyna/python/home", "", TYPE_STRING, PROPERTY_HINT_GLOBAL_DIR)
    # A scenery light is streamed with a range of its own, shorter than the model's - a street lamp
    # is visible from half a kilometre and lights forty metres. This is how far the camera may be
    # from the light itself, not how far the light reaches. The densest 300 m of stary_jawor holds
    # 152 of them, so their shadow maps are dropped 80 m out while the light itself keeps reaching.
    add_custom_project_setting(
        "reloaded/scenery/lights/distance", 400.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "10.0,1000.0,5.0,suffix:m"
    )
    add_custom_project_setting("maszyna/scenery/lights/cast_shadows", true, TYPE_BOOL)
    # A signal's or a lamp's free spotlight is drawn as a point of a constant size on the screen, the
    # way the original draws it, so it stays visible long after the lens is smaller than a pixel
    add_custom_project_setting("maszyna/scenery/railway_lights_visibility_improved", true, TYPE_BOOL)
    # A vehicle's mirror glass reflects the scene: a camera mirrored across it renders the glass at
    # its size on the screen while the mirror is in view within 30 m (PlanarMirror3D) - a scene
    # render per mirror
    add_custom_project_setting("maszyna/rendering/real_mirrors", true, TYPE_BOOL)
    # How many times the original's pointsize such a point is drawn; the original's 4 makes blobs
    add_custom_project_setting(
        "maszyna/scenery/railway_lights_point_size_multiplier", 2.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.5,8.0,0.1"
    )
    # How much of the original's glare alpha the light's glare is drawn with; the full value makes the
    # billboard read as an overlay
    add_custom_project_setting(
        "maszyna/scenery/railway_lights_glare_intensity", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    # The share of a light's cone (falloff angle) past which its glare fades out towards the cone's
    # edge; 1.0 keeps only the model's own hotspot-to-falloff band, a degree wide on vehicle lamps
    add_custom_project_setting(
        "maszyna/scenery/railway_lights_glare_fade_start", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    # How many degrees a glare's rays turn per degree the camera goes off the light's axis; 0 keeps
    # the same star from every side
    add_custom_project_setting(
        "maszyna/scenery/railway_lights_glare_rotation_ratio", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,4.0,0.1"
    )
    # A glare's size at the edge of the light's cone, as a share of its size on the axis; 1.0 keeps
    # the original's constant 2.5 m
    add_custom_project_setting(
        "maszyna/scenery/railway_lights_glare_edge_size", 0.4, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    # The least share of the screen's height a glare spans, however far the light; 0 lets it shrink
    # with the distance like the original's 2.5 m
    add_custom_project_setting(
        "maszyna/scenery/railway_lights_glare_min_screen_size", 0.03, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,0.2,0.005"
    )
    # How much a scenery model's real lights are worth. "Lights off" renders only the model's own
    # lit submodels; "Economy" collapses the lights of one model light into a single one between
    # them, raised by the offset and widened to cover every cone it replaces (a five-armed lamp is
    # otherwise five lights with five shadow maps); "High quality" keeps every declared light.
    add_custom_project_setting(
        "maszyna/scenery/lights/mode", 1,
        TYPE_INT, PROPERTY_HINT_ENUM, "Lights off,Economy,High quality"
    )
    add_custom_project_setting(
        "maszyna/scenery/lights/economy_height_offset", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "-2.0,5.0,0.1,suffix:m"
    )
    add_custom_project_setting(
        "maszyna/scenery/lights/economy_cone_scale", 1.2, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.5,3.0,0.05"
    )
    # Radius of the light source (Light3D.light_size). A lamp head is not a point: a size softens
    # the apex of the shaft in the fog and gives the shadows a penumbra that grows with distance.
    # In economy mode a merged light uses the radius of the ring of heads it replaces instead.
    add_custom_project_setting(
        "maszyna/scenery/lights/size", 0.25, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,3.0,0.05,suffix:m"
    )
    add_custom_project_setting(
        "maszyna/scenery/lights/energy", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.05"
    )
    # How much of the lamp's own colour is mixed into a white light. Used raw, a sodium lamp's
    # (1.0, 0.66, 0.18) throws away most of the luminance and the pool comes out nearly black.
    add_custom_project_setting(
        "maszyna/scenery/lights/tint", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.05"
    )
    add_custom_project_setting(
        "maszyna/scenery/lights/volumetric_fog_energy", 4.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,16.0,0.1"
    )
    add_custom_project_setting(
        "maszyna/vehicles/lights/volumetric_fog_energy", 4.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,16.0,0.1"
    )
    # Lifts the synthesized street lamp light above the halo billboard that marks the lamp head.
    # Nothing in the data asks for it - a plain tuning offset.
    add_custom_project_setting(
        "maszyna/scenery/lights/lamp_height_offset", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "-2.0,5.0,0.1,suffix:m"
    )
    # Widens the lamp cone past the lit patch the model draws (1.0 covers exactly the patch), and
    # the falloff exponent below 1.0 keeps the pool bright out to its edge. Both are tuning only -
    # a light the model declares keeps the falloff its own iFarAttenDecay asks for.
    add_custom_project_setting(
        "maszyna/scenery/lights/lamp_cone_scale", 1.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.5,4.0,0.05"
    )
    # Same parameter Godot calls SpotLight3D.spot_attenuation / OmniLight3D.omni_attenuation: the
    # exponent of the falloff with distance, drawn by the editor as the curve of brightness over
    # the light's range. Below 1.0 the pool holds its brightness and drops only near the range,
    # above 1.0 it dies right at the lamp. Not the cone edge - that is spot_angle_attenuation.
    add_custom_project_setting(
        "maszyna/scenery/lights/lamp_attenuation", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_EXP_EASING, "attenuation"
    )
    # The sprite of the models' emitters: the original's single one, or the core's own smoke; the
    # labels come from the enum itself, so they cannot drift from what the code branches on
    add_custom_project_setting(
        MaszynaSmokeSourceLibrary.GENERATOR_MODE_SETTING, MaszynaSmokeSourceLibrary.GeneratorMode.ORIGINAL,
        TYPE_INT, PROPERTY_HINT_ENUM, ",".join(MaszynaSmokeSourceLibrary.GeneratorMode.keys())
    )
    add_custom_project_setting(
        "maszyna/import/dds_max_texture_size", 1024, TYPE_INT,
        PROPERTY_HINT_ENUM, "512:512,1024:1024,2048:2048,4096:4096,8192:8192"
    )
    # the original's own limit for the cab, iMaxCabTextureSize (Globals.h:165)
    add_custom_project_setting(
        "maszyna/import/dds_max_cab_texture_size", 4096, TYPE_INT,
        PROPERTY_HINT_ENUM, "512:512,1024:1024,2048:2048,4096:4096,8192:8192"
    )
    add_import_plugin(fiz_import_plugin)

func _exit_tree():
    remove_import_plugin(fiz_import_plugin)

func add_custom_project_setting(name: String, default_value, type: int, hint: int = PROPERTY_HINT_NONE, hint_string: String = "") -> void:
    var setting_info: Dictionary = {
        "name": name,
        "type": type,
        "hint": hint,
        "hint_string": hint_string
    }

    # project.godot keeps only the value - the hint and the initial value are gone with every
    # editor restart, so a setting saved there still needs them registered again
    if not ProjectSettings.has_setting(name):
        ProjectSettings.set_setting(name, default_value)
    ProjectSettings.add_property_info(setting_info)
    ProjectSettings.set_initial_value(name, default_value)
