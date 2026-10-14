@tool
@abstract
extends Node
class_name MaszynaSkyEnvironment

## Draws the state of a MaszynaEnvironmentNode - the WorldEnvironment, its sky, the sun and the cab
## light. A sky addon is a backend extending this; libmaszyna carries none of its own, the project
## that has the addon places its backend next to the environment node and connects the node's
## configuration_changed to apply_configuration() in the scene.

const GENERATED_WORLD_NAME: StringName = &"_WorldEnvironment"
## Glow as tuned in forest-test-scene materials/environment_filmic.tres - what the player's Graphics
## settings (render/glow_intensity, render/bloom_intensity) fall back to
const GLOW_INTENSITY_DEFAULT: float = 1.37
const BLOOM_INTENSITY_DEFAULT: float = 0.3
## The player's gamma (render/gamma): the picture's mid-tones as x^(1/gamma) - above 1 brighter,
## below darker, black and white staying put - through the environment's colour correction, a
## curve of this many steps; at 1 there is no correction at all
const GAMMA_DEFAULT: float = 1.0
const GAMMA_CURVE_STEPS: int = 256

const SHADOW_SCENERY_ENABLED_SETTING: StringName = &"maszyna/scenery/shadows/enabled"
const SHADOW_CABIN_ENABLED_SETTING: StringName = &"maszyna/cabin/shadows/enabled"
const SHADOW_SCENERY_MODE_SETTING: StringName = &"maszyna/scenery/shadows/mode"
## The cab light covers the cab alone - two splits over its few metres are enough
const SHADOW_CABIN_MODE_SETTING: StringName = &"maszyna/cabin/shadows/mode"
const SHADOW_SCENERY_BLUR_SETTING: StringName = &"maszyna/scenery/shadows/blur"
const SHADOW_CABIN_BLUR_SETTING: StringName = &"maszyna/cabin/shadows/blur"
const SHADOW_SCENERY_OPACITY_SETTING: StringName = &"maszyna/scenery/shadows/opacity"
const SHADOW_CABIN_OPACITY_SETTING: StringName = &"maszyna/cabin/shadows/opacity"
const SHADOW_SCENERY_BIAS_SETTING: StringName = &"maszyna/scenery/shadows/bias"
const SHADOW_CABIN_BIAS_SETTING: StringName = &"maszyna/cabin/shadows/bias"
const SHADOW_SCENERY_NORMAL_BIAS_SETTING: StringName = &"maszyna/scenery/shadows/normal_bias"
const SHADOW_CABIN_NORMAL_BIAS_SETTING: StringName = &"maszyna/cabin/shadows/normal_bias"
const SHADOW_SCENERY_MAX_DISTANCE_SETTING: StringName = &"maszyna/scenery/shadows/max_distance"
const SHADOW_CABIN_MAX_DISTANCE_SETTING: StringName = &"maszyna/cabin/shadows/max_distance"
const SHADOW_SCENERY_BLEND_SPLITS_SETTING: StringName = &"maszyna/scenery/shadows/blend_splits"
const SHADOW_CABIN_BLEND_SPLITS_SETTING: StringName = &"maszyna/cabin/shadows/blend_splits"
const SHADOW_SCENERY_SPLIT_SETTINGS: Array[StringName] = [
    &"maszyna/scenery/shadows/split_1",
    &"maszyna/scenery/shadows/split_2",
    &"maszyna/scenery/shadows/split_3",
]
const SHADOW_CABIN_SPLIT_SETTINGS: Array[StringName] = [
    &"maszyna/cabin/shadows/split_1",
    &"maszyna/cabin/shadows/split_2",
    &"maszyna/cabin/shadows/split_3",
]
## The original's cascades end at range/32, range/8 and range (opengl33renderer.cpp:1106); Godot
## has four, and the third boundary continues the same x4 step
const SHADOW_SCENERY_SPLITS: Array[float] = [1.0 / 32.0, 1.0 / 8.0, 1.0 / 2.0]
## Original engine: Globals.h:153 shadowtune.range
const SHADOW_SCENERY_MAX_DISTANCE: float = 250.0
## The original has no normal bias at all; this is Godot's own DirectionalLight3D default. The
## lookup moves by normal_bias times the texel of a cascade - at 10 it moved 2.6 m in the last one
## and the shadow of a mast or a person was gone.
const SHADOW_SCENERY_NORMAL_BIAS: float = 1.0
## The cab light's two splits; only the first boundary is used by PARALLEL_2_SPLITS
const SHADOW_CABIN_SPLITS: Array[float] = [0.25, 0.5, 0.75]
## The original draws the cab only into its nearest cascade, SHADOW_SCENERY_MAX_DISTANCE / 32
## (opengl33renderer.cpp:1135)
const SHADOW_CABIN_MAX_DISTANCE: float = 8.0
const SHADOW_CABIN_NORMAL_BIAS: float = 5.0
## The cab gets a sun of its own that lights only MaszynaEnvironmentNode.CABIN_RENDER_LAYER, with
## the cabin/shadows settings, while the world's sun keeps the scenery ones in the cab view too
const CABIN_SHADOWS_IMPROVED_SETTING: StringName = &"maszyna/cabin/improve_shadows_quality"
## Shadow maps drawn with front faces culled (opengl33renderer.cpp:1758)
const REVERSE_CULL_FACE_SETTING: StringName = &"maszyna/lights/reverse_cull_face"
## Two shadowed directional lights halve the directional shadow atlas (Godot's
## light_storage.cpp _get_directional_shadow_rect) - twice the default keeps the world's cascades
## as sharp as with one light
const IMPROVED_CABIN_SHADOW_ATLAS_SIZE: int = 8192
const DIRECTIONAL_SHADOW_16_BITS_SETTING: StringName = &"rendering/lights_and_shadows/directional_shadow/16_bits"
const CABIN_LIGHT_NAME: StringName = &"CabinLight"

## Which settings a directional light takes: the world's sun the scenery ones, the cab light the
## cabin ones
enum ShadowView { SCENERY, CABIN }
## Skydome's volumetric fog volume is 8 m deep by day and 3 m by night. The volume is measured from
## the camera, so a light shaft can only be seen while its lamp is inside it - at 3 m, never. This
## stretches the volume; the density is divided by the same factor, which leaves the optical depth
## (extinction per metre times length) - and so the look of the fog - unchanged. At 24 that is 72 m
## by night and 192 m by day. Raising it further spreads the same froxel depth slices over more
## metres, which softens the fog near the camera, and shafts still stop where the light's shadow
## does (E3DRenderingServer.SCENERY_LIGHT_SHADOW_FADE_DISTANCE) - the two have to be raised together.
const FOG_VOLUMETRIC_LENGTH_SCALE_SETTING: StringName = &"maszyna/weather/fog/volumetric_length_scale"
const FOG_VOLUMETRIC_LENGTH_SCALE_DEFAULT: float = 24.0
const VOLUMETRIC_FOG_ENERGY_SETTING: StringName = &"maszyna/weather/fog/volumetric_energy"
## MaszynaEnvironmentNode.fog_distance is scaled by these for the day and for the night fog of a
## sky backend that tells them apart; the night default keeps Skydome's own 200 m to 470 m ratio
const FOG_DAY_DISTANCE_FACTOR_SETTING: StringName = &"maszyna/weather/fog/day_distance_factor"
const FOG_NIGHT_DISTANCE_FACTOR_SETTING: StringName = &"maszyna/weather/fog/night_distance_factor"
## How the fog builds up with the distance z from the camera: opacity = fog_density *
## (z / fog_distance) ^ exponent (Environment.fog_depth_curve). 1 - evenly, above 1 - clear near the
## camera and thickening further away, below 1 - thick right away. It has nothing to do with how
## fog_density itself is scaled. The original's fog is 1 - exp(-(z / range)^2) (apply_fog.glsl:16);
## an exponent of 1.5 with the fog complete at 1.5 of that range starts as slowly as it does
## (19/54/100% against 22/63/90% at 0.5/1/1.5 of the range) - hence the two defaults.
## How the fog builds up from the camera to MaszynaEnvironmentNode.fog_distance
## (Environment.fog_depth_curve), drawn by the editor as it works: x is the distance (0 at the
## camera, 1 at fog_distance), y the share of fog_density reached there - share = x ^ curve.
## 1 - evenly, above 1 - clear near the camera and thickening further away, below 1 - thick right
## away; FOG_CURVE_MIN keeps it from becoming a wall of fog in front of the camera. The original's
## fog is 1 - exp(-(z / range)^2) (apply_fog.glsl:16); a curve of 1.5 with the fog complete at 1.5
## of that range starts as slowly as it does (19/54/100% against 22/63/90% at 0.5/1/1.5 of the
## range) - hence the two defaults.
const FOG_CURVE_SETTING: StringName = &"maszyna/weather/fog/curve"
const FOG_CURVE_DEFAULT: float = 1.5
const FOG_CURVE_MIN: float = 0.5
## How much fog there is looking up, as a distance: the sky takes the share of the fog a terrain
## that far away would (share = (height / fog_distance) ^ fog_curve, at most 1). A fog reaching
## kilometres is a thin layer and leaves the stars, the moon and the clouds alone; a fog of a
## hundred metres hides them. Rain is no thin layer - it fills the air all the way up - so the fog
## a downpour brings reaches the sky in full.
const FOG_SKY_HEIGHT_SETTING: StringName = &"maszyna/weather/fog/sky_height"
const FOG_SKY_HEIGHT_DEFAULT: float = 1000.0
## How much the fog takes the colour of the sky's radiance instead of its own
## (Environment.fog_aerial_perspective). Off by default: at dusk that radiance is as dark as the
## scene itself, so a fully fogged object stays a dark silhouette against a brighter sky instead of
## fading out - the fog stops reading as fog.
const FOG_AERIAL_PERSPECTIVE_SETTING: StringName = &"maszyna/weather/fog/aerial_perspective"
const FOG_AERIAL_PERSPECTIVE_DEFAULT: float = 0.0
## A downpour brings a fog of its own, whatever fog is set: towards a full precipitation the fog
## distance shortens to this visibility and the fog density rises to this opacity (never the other
## way - a fog already closer or thicker stays as it is).
const RAIN_FOG_DISTANCE_SETTING: StringName = &"maszyna/weather/rain/fog_distance"
const RAIN_FOG_DISTANCE_DEFAULT: float = 800.0
const RAIN_FOG_DENSITY_SETTING: StringName = &"maszyna/weather/rain/fog_density"
const RAIN_FOG_DENSITY_DEFAULT: float = 0.6
## How fast the fog right in front of the camera (the volumetric layer) dies out once fog_distance
## grows past the distance the sky backend is tuned for: its density follows
## (reference / fog_distance) ^ falloff there. 1 - as below that distance; 2 - a fog reaching
## kilometres leaves the view in front of the camera as crisp as no fog at all.
const FOG_VOLUMETRIC_FAR_FALLOFF_SETTING: StringName = &"maszyna/weather/fog/volumetric_far_falloff"
const FOG_VOLUMETRIC_FAR_FALLOFF_DEFAULT: float = 2.0
## Floor under that falloff, as a share of the volumetric density at the reference distance. A
## scenery that declares a fog of kilometres (stary_jawor_noc asks for 2-4 km, which becomes a
## fog_distance of 2250-4500 m) drives the falloff to 0.01-0.04 and leaves the air by the camera
## with no haze at all - and a street lamp with nothing to scatter in casts no visible shaft. Real
## night air is never that clean, so the haze thins towards this share instead of towards nothing.
const FOG_VOLUMETRIC_MINIMUM_SETTING: StringName = &"maszyna/weather/fog/volumetric_minimum"
const FOG_VOLUMETRIC_MINIMUM_DEFAULT: float = 0.25
## fog_distance of a scenery that declares its fog, as a multiple of the original's fog range
const FOG_SCENERY_DISTANCE_FACTOR_SETTING: StringName = &"maszyna/weather/fog/scenery_distance_factor"
const FOG_SCENERY_DISTANCE_FACTOR_DEFAULT: float = 1.5
const FOG_DAY_DISTANCE_FACTOR_DEFAULT: float = 1.0
const FOG_NIGHT_DISTANCE_FACTOR_DEFAULT: float = 0.4255

## The environment this sky draws
@export_node_path("MaszynaEnvironmentNode") var environment_node_path: NodePath

@export_group("Tone Mapping")
@export var tonemap_mode: Environment.ToneMapper = Environment.TONE_MAPPER_AGX:
    set(value):
        tonemap_mode = value
        _dirty_visuals = true

@export_range(0.0, 16.0, 0.01) var tonemap_white: float = 6.0:
    set(value):
        tonemap_white = value
        _dirty_visuals = true

@export_range(0.0, 16.0, 0.01) var tonemap_agx_white: float = 6.19:
    set(value):
        tonemap_agx_white = value
        _dirty_visuals = true

@export_range(0.0, 2.0, 0.01) var tonemap_agx_contrast: float = 1.55:
    set(value):
        tonemap_agx_contrast = value
        _dirty_visuals = true

@export var adjustment_enabled: bool = true:
    set(value):
        adjustment_enabled = value
        _dirty_visuals = true

var environment_node: MaszynaEnvironmentNode
var _environment: Environment
## The cab light of each world light, when CABIN_SHADOWS_IMPROVED_SETTING is on
var _cabin_lights: Dictionary[DirectionalLight3D, DirectionalLight3D] = {}
var _dirty_visuals: bool = true
var _dirty_lights: bool = true
var _dirty_time: bool = true
## The gamma the colour correction curve was made for - it is made again only when that changes -
## and the curve itself, none at the default
var _gamma: float = GAMMA_DEFAULT
var _gamma_curve: ImageTexture = null


func _ready() -> void:
    environment_node = get_node(environment_node_path) as MaszynaEnvironmentNode
    var world_environment: WorldEnvironment = WorldEnvironment.new()
    world_environment.name = GENERATED_WORLD_NAME
    _environment = Environment.new()
    _environment.background_mode = Environment.BG_SKY
    _environment.sky = create_sky()
    _environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
    _environment.ambient_light_color = Color.WHITE
    _environment.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
    _environment.fog_mode = Environment.FOG_MODE_DEPTH
    _environment.fog_light_color = Color(0.3605356, 0.39691955, 0.44612736, 1.0)
    _environment.fog_light_energy = 0.7
    _environment.fog_sun_scatter = 0.07
    _environment.volumetric_fog_anisotropy = 0.0
    _environment.volumetric_fog_detail_spread = 1.0
    # Glow tuned as in forest-test-scene materials/environment_filmic.tres (its intensity and bloom
    # are the player's, _process()); the luminance cap keeps small specular highlights (e.g. rain
    # streaks) from blooming into large blobs.
    _environment.glow_normalized = true
    _environment.glow_strength = 0.8
    _environment.glow_hdr_threshold = 1.37
    _environment.glow_hdr_luminance_cap = 0.18
    world_environment.environment = _environment
    world_environment.camera_attributes = CameraAttributesPractical.new()
    create_nodes(world_environment, _environment)
    add_child(world_environment, false, INTERNAL_MODE_BACK)


func _enter_tree() -> void:
    UserSettings.config_changed.connect(_on_user_settings_changed)
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    SimulationServer.simulation_paused.connect(pause_weather)
    SimulationServer.simulation_unpaused.connect(unpause_weather)


func _exit_tree() -> void:
    UserSettings.config_changed.disconnect(_on_user_settings_changed)
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)
    SimulationServer.simulation_paused.disconnect(pause_weather)
    SimulationServer.simulation_unpaused.disconnect(unpause_weather)


func _process(delta: float) -> void:
    if _dirty_visuals:
        _dirty_visuals = false
        _apply_environment_configuration()
        apply_visual_configuration()
    if _dirty_lights:
        _dirty_lights = false
        apply_light_configuration()
    if _dirty_time:
        _dirty_time = false
        apply_time_configuration()
    sync_cabin_lights()
    process_time(delta)


## The environment node's configuration changed (MaszynaEnvironmentNode.configuration_changed) -
## everything it shows is taken again
func apply_configuration() -> void:
    _dirty_visuals = true
    _dirty_lights = true
    _dirty_time = true


@abstract func create_sky() -> Sky


@abstract func create_nodes(
    world_environment: WorldEnvironment, environment: Environment
) -> void


## The backend's own share of the environment's weather and fog
@abstract func apply_visual_configuration() -> void


## Applies the scenery/shadows and cabin/shadows settings to the backend's directional lights.
@abstract func apply_light_configuration() -> void


## The environment's time and date set, not run - the sky jumps to them
@abstract func apply_time_configuration() -> void


## The running time of the environment, followed by the sky
func process_time(_delta: float) -> void:
    pass


## Holds the weather (rain, lightning) still while the runtime is paused (SimulationServer.simulation_pause()).
## A backend without weather of its own has nothing to hold.
func pause_weather() -> void:
    pass


func unpause_weather() -> void:
    pass


func _apply_environment_configuration() -> void:
    # fog_density is a multiplier; zero fades the fog out instead of switching it off.
    var fog_active: bool = environment_node.fog_enabled
    _environment.tonemap_mode = tonemap_mode
    _environment.tonemap_white = tonemap_white
    _environment.tonemap_agx_white = tonemap_agx_white
    _environment.tonemap_agx_contrast = tonemap_agx_contrast
    _environment.ssr_enabled = bool(UserSettings.get_setting("render", "ssr_enabled", true))
    _environment.ssao_enabled = bool(UserSettings.get_setting("render", "ssao_enabled", true))
    _environment.ssil_enabled = bool(UserSettings.get_setting("render", "ssil_enabled", true))
    _environment.sdfgi_enabled = bool(UserSettings.get_setting("render", "sdfgi_enabled", true))
    _environment.glow_enabled = bool(UserSettings.get_setting("render", "glow_enabled", true))
    _environment.glow_intensity = float(
        UserSettings.get_setting("render", "glow_intensity", GLOW_INTENSITY_DEFAULT))
    _environment.glow_bloom = float(
        UserSettings.get_setting("render", "bloom_intensity", BLOOM_INTENSITY_DEFAULT))
    _environment.adjustment_enabled = adjustment_enabled
    var gamma: float = float(UserSettings.get_setting("render", "gamma", GAMMA_DEFAULT))
    if not is_equal_approx(gamma, _gamma):
        _gamma = gamma
        _gamma_curve = null
        if not is_equal_approx(gamma, GAMMA_DEFAULT):
            # one row, the input along it, the output in every channel
            var curve: Image = Image.create(GAMMA_CURVE_STEPS, 1, false, Image.FORMAT_RGB8)
            for step: int in GAMMA_CURVE_STEPS:
                var value: float = pow(float(step) / float(GAMMA_CURVE_STEPS - 1), 1.0 / gamma)
                curve.set_pixel(step, 0, Color(value, value, value))
            _gamma_curve = ImageTexture.create_from_image(curve)
    _environment.adjustment_color_correction = _gamma_curve
    _environment.fog_enabled = fog_active
    # the sky backends leave these two alone
    _environment.fog_aerial_perspective = float(ProjectSettings.get_setting(
        FOG_AERIAL_PERSPECTIVE_SETTING, FOG_AERIAL_PERSPECTIVE_DEFAULT))
    _environment.fog_depth_curve = maxf(FOG_CURVE_MIN, float(
        ProjectSettings.get_setting(FOG_CURVE_SETTING, FOG_CURVE_DEFAULT)))
    _environment.volumetric_fog_enabled = (
        fog_active and bool(UserSettings.get_setting("render", "volumetric_fog_enabled", true))
    )


func _on_user_settings_changed() -> void:
    _dirty_visuals = true


## The fog, the shadows and the lights follow their project settings while the scenery runs
func _on_project_settings_changed() -> void:
    _dirty_visuals = true
    _dirty_lights = true


## Copies what the sky backend drives on a world light every frame (Skydome writes colour, energy
## and, from the clouds, the shadow opacity and the angular size that softens the shadow -
## Skydome.gd:775 - all with no signal) onto its cab light. The direction comes
## with the node tree: the cab light is a child with an identity transform. At most two entries.
func sync_cabin_lights() -> void:
    for world_light: DirectionalLight3D in _cabin_lights:
        var cabin_light: DirectionalLight3D = _cabin_lights[world_light]
        cabin_light.light_color = world_light.light_color
        cabin_light.light_energy = world_light.light_energy
        cabin_light.shadow_opacity = world_light.shadow_opacity
        cabin_light.light_angular_distance = world_light.light_angular_distance


## Gives a world light a cab light of its own: it lights only the cab layer and the world light no
## longer does. Shadow casters stay on every layer, so a station roof or the vehicle's body still
## shades the cab.
func _create_cabin_light(world_light: DirectionalLight3D) -> void:
    var cabin_light: DirectionalLight3D = DirectionalLight3D.new()
    cabin_light.name = CABIN_LIGHT_NAME
    cabin_light.light_cull_mask = MaszynaEnvironmentNode.CABIN_RENDER_LAYER
    # the world light already draws the sun in the sky and scatters in the fog
    cabin_light.sky_mode = DirectionalLight3D.SKY_MODE_LIGHT_ONLY
    cabin_light.light_volumetric_fog_energy = 0.0
    world_light.add_child(cabin_light, false, Node.INTERNAL_MODE_BACK)
    world_light.light_cull_mask &= ~MaszynaEnvironmentNode.CABIN_RENDER_LAYER
    _cabin_lights[world_light] = cabin_light
    RenderingServer.directional_shadow_atlas_set_size(
        IMPROVED_CABIN_SHADOW_ATLAS_SIZE, bool(ProjectSettings.get_setting(DIRECTIONAL_SHADOW_16_BITS_SETTING, true))
    )


## The world light gives the cab light back the cab layer it took
func _free_cabin_light(world_light: DirectionalLight3D) -> void:
    world_light.light_cull_mask |= MaszynaEnvironmentNode.CABIN_RENDER_LAYER
    _cabin_lights[world_light].queue_free()
    _cabin_lights.erase(world_light)


## A world light takes the scenery settings, its cab light the cabin ones; the cab light is there
## while CABIN_SHADOWS_IMPROVED_SETTING is on, and comes and goes with it
func _apply_sun_settings(world_light: DirectionalLight3D) -> void:
    var improved: bool = bool(ProjectSettings.get_setting(CABIN_SHADOWS_IMPROVED_SETTING, true))
    if improved and not _cabin_lights.has(world_light):
        _create_cabin_light(world_light)
    elif not improved and _cabin_lights.has(world_light):
        _free_cabin_light(world_light)
    _apply_directional_light_settings(world_light, ShadowView.SCENERY)
    if _cabin_lights.has(world_light):
        _apply_directional_light_settings(_cabin_lights[world_light], ShadowView.CABIN)


func _apply_directional_light_settings(light: DirectionalLight3D, view: ShadowView) -> void:
    var cabin: bool = view == ShadowView.CABIN
    light.shadow_reverse_cull_face = bool(ProjectSettings.get_setting(REVERSE_CULL_FACE_SETTING, false))
    # the cab light casts shadows only while there is a cab to look at; it keeps lighting the cab
    # seen from outside
    light.shadow_enabled = (
        bool(ProjectSettings.get_setting(SHADOW_CABIN_ENABLED_SETTING, true))
        and environment_node.cabin_view
        if cabin
        else bool(ProjectSettings.get_setting(SHADOW_SCENERY_ENABLED_SETTING, true))
    )
    light.shadow_opacity = float(
        ProjectSettings.get_setting(SHADOW_CABIN_OPACITY_SETTING, 1.0)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_OPACITY_SETTING, 1.0)
    )
    light.shadow_bias = float(
        ProjectSettings.get_setting(SHADOW_CABIN_BIAS_SETTING, 0.1)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_BIAS_SETTING, 0.1)
    )
    light.shadow_blur = float(
        ProjectSettings.get_setting(SHADOW_CABIN_BLUR_SETTING, 1.0)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_BLUR_SETTING, 1.0)
    )
    # Normal bias pushes the shadow lookup along the surface normal by normal_bias texels of the
    # split - thinner casters than that lose their shadow, so it stays small outside.
    light.shadow_normal_bias = float(
        ProjectSettings.get_setting(SHADOW_CABIN_NORMAL_BIAS_SETTING, SHADOW_CABIN_NORMAL_BIAS)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_NORMAL_BIAS_SETTING, SHADOW_SCENERY_NORMAL_BIAS)
    )
    light.directional_shadow_mode = int(
        ProjectSettings.get_setting(SHADOW_CABIN_MODE_SETTING, DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_MODE_SETTING, DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS)
    ) as DirectionalLight3D.ShadowMode
    light.directional_shadow_max_distance = float(
        ProjectSettings.get_setting(SHADOW_CABIN_MAX_DISTANCE_SETTING, SHADOW_CABIN_MAX_DISTANCE)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_MAX_DISTANCE_SETTING, SHADOW_SCENERY_MAX_DISTANCE)
    )
    var split_settings: Array[StringName] = SHADOW_CABIN_SPLIT_SETTINGS if cabin else SHADOW_SCENERY_SPLIT_SETTINGS
    var split_defaults: Array[float] = SHADOW_CABIN_SPLITS if cabin else SHADOW_SCENERY_SPLITS
    light.directional_shadow_split_1 = float(ProjectSettings.get_setting(split_settings[0], split_defaults[0]))
    light.directional_shadow_split_2 = float(ProjectSettings.get_setting(split_settings[1], split_defaults[1]))
    light.directional_shadow_split_3 = float(ProjectSettings.get_setting(split_settings[2], split_defaults[2]))
    light.directional_shadow_blend_splits = bool(
        ProjectSettings.get_setting(SHADOW_CABIN_BLEND_SPLITS_SETTING, true)
        if cabin
        else ProjectSettings.get_setting(SHADOW_SCENERY_BLEND_SPLITS_SETTING, true)
    )
    if not cabin:
        light.light_volumetric_fog_energy = float(ProjectSettings.get_setting(VOLUMETRIC_FOG_ENERGY_SETTING, 1.0))
