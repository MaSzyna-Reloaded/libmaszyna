@tool
extends EditorPlugin

# Custom nodes
const PLUGIN_NAME = "libmaszyna"

var maszyna_environment_node_script = preload("res://addons/libmaszyna/environment/maszyna_environment_node.gd")
var maszyna_environment_node_icon = preload("res://addons/libmaszyna/environment/maszyna_environment_node_icon.png")
var e3d_model_instance_script = preload("res://addons/libmaszyna/legacy/e3d/e3d_model_instance.gd")
var e3d_model_instance_icon = preload("res://addons/libmaszyna/legacy/e3d/e3d_model_instance.png")
var track_3d_script = preload("res://addons/libmaszyna/tracks/track_3d.gd")
var track_normal_3d_script = preload("res://addons/libmaszyna/tracks/track_normal_3d.gd")
var track_switch_3d_script = preload("res://addons/libmaszyna/tracks/track_switch_3d.gd")
var maszyna_rail_vehicle_physics_node_script = preload("res://addons/libmaszyna/legacy/vehicle/maszyna_rail_vehicle_physics_node.gd")
var fiz_import_plugin = preload("res://addons/libmaszyna/legacy/fiz/fiz_import_plugin.gd").new()

func _enable_plugin():
    add_autoload_singleton("Console", "res://addons/libmaszyna/console/console.gd")
    add_autoload_singleton("MaterialManager", "res://addons/libmaszyna/legacy/materials/material_manager.gd")
    add_autoload_singleton("MaszynaMaterialParser", "res://addons/libmaszyna/legacy/materials/maszyna_material_parser.gd")
    add_autoload_singleton("MaszynaMaterialFactory", "res://addons/libmaszyna/legacy/materials/maszyna_material_factory.gd")
    add_autoload_singleton("E3DModelManager", "res://addons/libmaszyna/legacy/e3d/e3d_model_manager.gd")
    add_autoload_singleton("E3DModelTool", "res://addons/libmaszyna/legacy/e3d/e3d_model_tool.gd")
    add_autoload_singleton("MaszynaLegacyVehicleSystem", "res://addons/libmaszyna/legacy/vehicle/maszyna_legacy_vehicle_system.gd")
    add_autoload_singleton("MaszynaVehicleProfileManager", "res://addons/libmaszyna/legacy/vehicle/maszyna_vehicle_profile_manager.gd")
    add_autoload_singleton("AudioStreamManager", "res://addons/libmaszyna/sound/audio_stream_manager.gd")
    add_autoload_singleton("TranscriptSystem", "res://addons/libmaszyna/sound/transcript_system.gd")
    add_autoload_singleton("TrainSoundSystem", "res://addons/libmaszyna/sound/train_sound_system.gd")
    add_autoload_singleton("ScenerySoundServer", "res://addons/libmaszyna/sound/scenery_sound_server.gd")
    add_autoload_singleton("CabinSystem", "res://addons/libmaszyna/cabin/cabin_system.gd")
    add_autoload_singleton("FIZResourceLoaderRegistrar", "res://addons/libmaszyna/legacy/fiz/fiz_resource_loader_registrar.gd")
    add_autoload_singleton("MaszynaSceneryChunkRenderingServer", "res://addons/libmaszyna/legacy/scenery/maszyna_scenery_chunk_rendering_server.gd")
    add_autoload_singleton("SmokeSourceLibrary", "res://addons/libmaszyna/smoke/smoke_source_library.gd")

    add_custom_type(
        "MaszynaEnvironmentNode",
        "Node",
        maszyna_environment_node_script,
        maszyna_environment_node_icon,
    )

    add_custom_type(
        "E3DModelInstance",
        "VisualInstance3D",
        e3d_model_instance_script,
        e3d_model_instance_icon,
    )

    add_custom_type(
        "Track3D",
        "VisualInstance3D",
        track_3d_script,
        null
    )

    add_custom_type(
        "TrackNormal3D",
        "VisualInstance3D",
        track_normal_3d_script,
        null
    )

    add_custom_type(
        "TrackSwitch3D",
        "VisualInstance3D",
        track_switch_3d_script,
        null
    )

    add_custom_type(
        "MaszynaRailVehiclePhysicsNode",
        "Node",
        maszyna_rail_vehicle_physics_node_script,
        null
    )

    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/e3d_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/fiz_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scenery_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/tracks", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/nodebank", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/user_settings_dock", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/maszyna_vehicles", true)

func _disable_plugin():
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/e3d_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/fiz_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scenery_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/tracks", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/nodebank", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/user_settings_dock", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/maszyna_vehicles", false)

    remove_custom_type("E3DModelInstance")
    remove_custom_type("MaszynaEnvironmentNode")
    remove_custom_type("Track3D")
    remove_custom_type("TrackNormal3D")
    remove_custom_type("TrackSwitch3D")
    remove_custom_type("MaszynaRailVehiclePhysicsNode")

    remove_autoload_singleton("TrainSoundSystem")
    remove_autoload_singleton("ScenerySoundServer")
    remove_autoload_singleton("CabinSystem")
    remove_autoload_singleton("TranscriptSystem")
    remove_autoload_singleton("AudioStreamManager")
    remove_autoload_singleton("FIZResourceLoaderRegistrar")
    remove_autoload_singleton("MaszynaVehicleProfileManager")
    remove_autoload_singleton("MaszynaLegacyVehicleSystem")
    remove_autoload_singleton("E3DModelTool")
    remove_autoload_singleton("E3DModelManager")
    remove_autoload_singleton("MaszynaMaterialFactory")
    remove_autoload_singleton("MaterialManager")
    remove_autoload_singleton("MaszynaMaterialParser")
    remove_autoload_singleton("SmokeSourceLibrary")
    remove_autoload_singleton("MaszynaSceneryChunkRenderingServer")
    remove_autoload_singleton("Console")

func _enter_tree():
    add_custom_project_setting("maszyna/import/model_scale_factor", 1.0, TYPE_FLOAT)
    add_custom_project_setting("maszyna/scenery/track_curve_bake_interval", 10.0, TYPE_FLOAT)
    # The original renders shadow maps with front faces culled (opengl33renderer.cpp:1758) against
    # self-shadowing acne; off by default like in Godot, because an open single-sided model then casts
    # no shadow and the acne only moves to the unlit side
    add_custom_project_setting("maszyna/lights/reverse_cull_face", false, TYPE_BOOL)
    # The cab gets a sun of its own with sharp shadows over its few metres, and the world's sun keeps
    # its far cascades in the cab view too; costs a second shadow map and a twice larger atlas
    add_custom_project_setting("maszyna/cabin/improve_shadows_quality", true, TYPE_BOOL)
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
    # ResourceLazyLoader: on, the models of a scenery, its terrain and a vehicle's models are loaded
    # when the camera comes near and let go when it leaves; off, they are all loaded with the
    # scenery and kept, and the streaming only builds and clears their instances. The
    # --enable-lazy-loading command-line switch turns it on (read at startup)
    add_custom_project_setting("maszyna/resources/lazy_loading", false, TYPE_BOOL)
    # E3DRenderingServer streams registered scenery models in and out around the camera; this caps
    # every node's own range and stands in for the nodes that declare none (read at startup)
    add_custom_project_setting(
        "maszyna/scenery/draw_distance", 3000.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,20000.0,10.0,suffix:m"
    )
    # A scenery light is streamed with a range of its own, shorter than the model's - a street lamp
    # is visible from half a kilometre and lights forty metres. This is how far the camera may be
    # from the light itself, not how far the light reaches. The densest 300 m of stary_jawor holds
    # 152 of them, so their shadow maps are dropped 80 m out while the light itself keeps reaching.
    add_custom_project_setting(
        "maszyna/scenery/lights/distance", 400.0, TYPE_FLOAT,
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
    # How long the simulation's running speed takes to reach one set, like a tape's motor
    # (SimulationServer.simulation_get_current_speed())
    add_custom_project_setting(
        "maszyna/simulation/speed_change_time", 0.4, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,5.0,0.05,suffix:s"
    )
    # How close and how far the zoom takes the external view (Shift+F4) from what it looks at
    add_custom_project_setting(
        "maszyna/camera/external_view_min_distance", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,20.0,0.1,suffix:m"
    )
    add_custom_project_setting(
        "maszyna/camera/external_view_max_distance", 200.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "10.0,2000.0,10.0,suffix:m"
    )
    # A vehicle to follow farther than this from the view is not flown to: the view jumps beside it
    # (the vehicle card's crosshair) and follows from there
    add_custom_project_setting(
        "maszyna/camera/follow_jump_distance", 10000.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,100000.0,100.0,suffix:m"
    )
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
    # Skydome's volumetric fog volume is 8 m deep by day and 3 m at night, and it is measured from
    # the camera - a light shaft is only visible while its lamp is inside it. This stretches the
    # volume while the density is divided by the same factor, which leaves the optical depth, and
    # so the look of the fog, alone. At 24 that is 72 m by night and 192 m by day.
    add_custom_project_setting(
        "maszyna/weather/fog/volumetric_length_scale", 24.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "1.0,64.0,0.5"
    )
    # Floor under fog_volumetric_far_falloff. A scenery asking for a fog of kilometres drives that
    # falloff to 0.01-0.04 and leaves no haze by the camera at all, so a street lamp has nothing to
    # scatter in and casts no visible shaft. Night air is never that clean.
    add_custom_project_setting(
        "maszyna/weather/fog/volumetric_minimum", 0.25, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    # Sun altitude between which the light level ramps from night to full day; a scenery light set
    # to come on automatically lights below a level of 0.325 (AnimModel.cpp:598), which lands about
    # 1.4 degrees below the horizon on this ramp. A winter noon sun peaks at 16-19 degrees at 50 N,
    # so the day end must stay well below that (FINDINGS.md).
    add_custom_project_setting(
        "maszyna/lights/night_altitude", -6.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "-18.0,0.0,0.5,suffix:°"
    )
    add_custom_project_setting(
        "maszyna/lights/day_altitude", 6.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,15.0,0.5,suffix:°"
    )
    # The distance a vehicle stops rendering from a node hierarchy at and switches to
    # RenderingServer instances; it switches back 25% closer. 350 m is where E3DNodesBackend has
    # already faded its spotlights out completely (distance_fade_begin 150 + length 200), and the
    # OPTIMIZED backend's own lights are streamed by scenery_light_distance instead.
    add_custom_project_setting(
        "maszyna/vehicles/detail_distance", 350.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "50.0,10000.0,10.0,suffix:m"
    )
    # The original's gfx.smoke (Globals.cpp:1314): with it off no model builds a particle emitter
    # at all, whether it is a locomotive's exhaust or a factory chimney.
    add_custom_project_setting("maszyna/smoke/enabled", true, TYPE_BOOL)
    # the labels come from the enum itself, so they cannot drift from what the code branches on
    add_custom_project_setting(
        SmokeSourceLibrary.GENERATOR_MODE_SETTING, SmokeSourceLibrary.GeneratorMode.ORIGINAL,
        TYPE_INT, PROPERTY_HINT_ENUM, ",".join(SmokeSourceLibrary.GeneratorMode.keys())
    )
    # MODERN needs a flipbook, and the addon ships none - the project using it fills this in
    add_custom_project_setting(
        SmokeSourceLibrary.ATLAS_SETTING, "", TYPE_STRING, PROPERTY_HINT_FILE, "*.png,*.webp,*.dds"
    )
    add_custom_project_setting(
        SmokeSourceLibrary.ATLAS_FRAMES_SETTING, Vector2i(4, 4), TYPE_VECTOR2I
    )
    # How far from the camera a scenery emitter is kept alive. The original stops spawning beyond
    # twice the draw range (particles.cpp:452); a chimney has to be seen from further away than a
    # street lamp, so this is not scenery_light_distance.
    add_custom_project_setting(
        "maszyna/smoke/dynamic/distance", 1500.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,10000.0,10.0,suffix:m"
    )
    add_custom_project_setting(
        "maszyna/smoke/static/distance", 1500.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,10000.0,10.0,suffix:m"
    )
    # Every knob below comes in a pair, one for a vehicle and one for a static piece of the
    # scenery - the .scn already draws that line: a "dynamic" is a vehicle, a "node model" is
    # static. A road vehicle placed as a "node model" - which is how all 349 of them stand in the
    # data - is a prop and takes the static numbers.
    #
    # How many particles an emitter spawns per second, over what its template asks for. Each one is
    # made correspondingly fainter, so a denser plume comes out smoother rather than darker - the
    # original's gfx.smoke.fidelity works the same way (particles.cpp:73, :128, :165).
    add_custom_project_setting(
        "maszyna/smoke/dynamic/density", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.25,8.0,0.25"
    )
    add_custom_project_setting(
        "maszyna/smoke/static/density", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.25,8.0,0.25"
    )
    # How long a particle lives, over what its template asks for. A chimney template fades at 0.01
    # per second, which is a minute of particle in the air - far more than a scenery prop needs,
    # and it is also what decides how many of them are in flight at once.
    add_custom_project_setting(
        "maszyna/smoke/dynamic/lifetime", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.05"
    )
    add_custom_project_setting(
        "maszyna/smoke/static/lifetime", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.05"
    )
    # Particle budget of a single emitter, which the original caps at 500 per source at its lowest
    # smoke fidelity (particles.cpp:128). It has to leave room for the density above.
    add_custom_project_setting(
        "maszyna/smoke/dynamic/max_particles", 2000, TYPE_INT,
        PROPERTY_HINT_RANGE, "16,8000,1"
    )
    add_custom_project_setting(
        "maszyna/smoke/static/max_particles", 500, TYPE_INT,
        PROPERTY_HINT_RANGE, "16,8000,1"
    )
    add_custom_project_setting("maszyna/debug/physics_diagnostics", false, TYPE_BOOL)
    add_custom_project_setting(
        "maszyna/import/dds_max_texture_size", 1024, TYPE_INT,
        PROPERTY_HINT_ENUM, "512:512,1024:1024,2048:2048,4096:4096,8192:8192"
    )
    # the original's own limit for the cab, iMaxCabTextureSize (Globals.h:165)
    add_custom_project_setting(
        "maszyna/import/dds_max_cab_texture_size", 4096, TYPE_INT,
        PROPERTY_HINT_ENUM, "512:512,1024:1024,2048:2048,4096:4096,8192:8192"
    )
    add_custom_project_setting(
        "maszyna/sound/culling_distance", 1000.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,5000.0,10.0,or_greater"
    )
    add_custom_project_setting(
        "maszyna/weather/wind_turbulence", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,4.0,0.01,or_greater"
    )
    add_custom_project_setting(MaszynaSkyEnvironment.SHADOW_SCENERY_ENABLED_SETTING, true, TYPE_BOOL)
    add_custom_project_setting(MaszynaSkyEnvironment.SHADOW_CABIN_ENABLED_SETTING, true, TYPE_BOOL)
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_SCENERY_MODE_SETTING, DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS,
        TYPE_INT, PROPERTY_HINT_ENUM, "Orthogonal,PSSM 2 Splits,PSSM 4 Splits"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_CABIN_MODE_SETTING, DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS,
        TYPE_INT, PROPERTY_HINT_ENUM, "Orthogonal,PSSM 2 Splits,PSSM 4 Splits"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_SCENERY_BLUR_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.01,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_CABIN_BLUR_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.01,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_SCENERY_OPACITY_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_CABIN_OPACITY_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_SCENERY_BIAS_SETTING, 0.1, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.001,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_CABIN_BIAS_SETTING, 0.1, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.001,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_SCENERY_NORMAL_BIAS_SETTING, 10.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,32.0,0.001,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_CABIN_NORMAL_BIAS_SETTING, 5.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,32.0,0.001,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_SCENERY_MAX_DISTANCE_SETTING, 100.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10000.0,1.0,suffix:m"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.SHADOW_CABIN_MAX_DISTANCE_SETTING, 150.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10000.0,1.0,suffix:m"
    )
    add_custom_project_setting(MaszynaSkyEnvironment.SHADOW_SCENERY_BLEND_SPLITS_SETTING, true, TYPE_BOOL)
    add_custom_project_setting(MaszynaSkyEnvironment.SHADOW_CABIN_BLEND_SPLITS_SETTING, true, TYPE_BOOL)
    for i: int in 3:
        add_custom_project_setting(
            MaszynaSkyEnvironment.SHADOW_SCENERY_SPLIT_SETTINGS[i], MaszynaSkyEnvironment.SHADOW_SCENERY_SPLITS[i],
            TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
        )
        add_custom_project_setting(
            MaszynaSkyEnvironment.SHADOW_CABIN_SPLIT_SETTINGS[i], MaszynaSkyEnvironment.SHADOW_CABIN_SPLITS[i],
            TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
        )
    add_custom_project_setting(
        MaszynaSkyEnvironment.VOLUMETRIC_FOG_ENERGY_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,16.0,0.001,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_CURVE_SETTING, MaszynaSkyEnvironment.FOG_CURVE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_EXP_EASING, "positive_only"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_SKY_HEIGHT_SETTING, MaszynaSkyEnvironment.FOG_SKY_HEIGHT_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "10.0,5000.0,10.0,or_greater,suffix:m"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_AERIAL_PERSPECTIVE_SETTING,
        MaszynaSkyEnvironment.FOG_AERIAL_PERSPECTIVE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.RAIN_FOG_DISTANCE_SETTING, MaszynaSkyEnvironment.RAIN_FOG_DISTANCE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "10.0,5000.0,10.0,or_greater,suffix:m"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.RAIN_FOG_DENSITY_SETTING, MaszynaSkyEnvironment.RAIN_FOG_DENSITY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_VOLUMETRIC_FAR_FALLOFF_SETTING,
        MaszynaSkyEnvironment.FOG_VOLUMETRIC_FAR_FALLOFF_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "1.0,4.0,0.05"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
        MaszynaSkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.01,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_DAY_DISTANCE_FACTOR_SETTING,
        MaszynaSkyEnvironment.FOG_DAY_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.01,4.0,0.0001,or_greater"
    )
    add_custom_project_setting(
        MaszynaSkyEnvironment.FOG_NIGHT_DISTANCE_FACTOR_SETTING,
        MaszynaSkyEnvironment.FOG_NIGHT_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.01,4.0,0.0001,or_greater"
    )
    # The wrapper's own .po files; MaszynaTranslationServer merges them with the game's catalogue into
    # the one translation TranslationServer uses, so they are not listed in Localization
    add_custom_project_setting(
        "maszyna/locale/translations", PackedStringArray(), TYPE_PACKED_STRING_ARRAY,
        PROPERTY_HINT_TYPE_STRING, "%d/%d:*.po" % [TYPE_STRING, PROPERTY_HINT_FILE]
    )
    add_import_plugin(fiz_import_plugin)

func _exit_tree():
    remove_import_plugin(fiz_import_plugin)
    print_verbose("Libmaszyna.gd _exit_tree finished!")

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
