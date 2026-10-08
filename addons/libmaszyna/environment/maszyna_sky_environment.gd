@tool
@abstract
extends RefCounted
class_name MaszynaSkyEnvironment

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
## Sun altitude (degrees) between which get_light_level() ramps from night to full day. The
## original lights a scenery light set to "on when dark" below a light level of 0.325
## (AnimModel.cpp:598), which on this ramp falls at about 1.4 degrees below the horizon. Both ends
## have to stay clear of a winter noon - at 50 N the sun peaks at 16-19 degrees in January, so a
## day threshold anywhere near that would light the whole town at midday (see FINDINGS.md).
const LIGHT_LEVEL_NIGHT_ALTITUDE_SETTING: StringName = &"maszyna/lights/night_altitude"
const LIGHT_LEVEL_DAY_ALTITUDE_SETTING: StringName = &"maszyna/lights/day_altitude"
const LIGHT_LEVEL_NIGHT_ALTITUDE: float = -6.0
const LIGHT_LEVEL_DAY_ALTITUDE: float = 6.0
## Overcast dims the key light in the original by this much at full cover
## (simulationenvironment.cpp:163)
const LIGHT_LEVEL_OVERCAST_FACTOR: float = 0.65
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

## Wind speed the strength of the environment node maps onto, m/s
const WIND_SPEED_MIN: float = 0.15
const WIND_SPEED_MAX: float = 3.0

var environment_node: Node
## The cab light of each world light, when CABIN_SHADOWS_IMPROVED_SETTING is on
var _cabin_lights: Dictionary[DirectionalLight3D, DirectionalLight3D] = {}


func _init(node: Node) -> void:
    environment_node = node


@abstract func create_sky() -> Sky


@abstract func create_nodes(
    world_environment: WorldEnvironment, environment: Environment
) -> void


@abstract func bind_nodes(world_environment: WorldEnvironment) -> void


@abstract func apply_visual_configuration() -> void


## Applies the scenery/shadows and cabin/shadows settings to the backend's directional lights.
@abstract func apply_light_configuration() -> void


@abstract func set_date(year: int, month: int, day: int) -> Vector3i


@abstract func apply_time_configuration() -> void


@abstract func get_date() -> Vector3i


@abstract func get_current_time() -> float


## How bright the scene is, the equivalent of the original's Global.fLuminance
## (simulationenvironment.cpp:184). It is what decides whether a scenery light that is set to come
## on automatically is on: the original compares it against DefaultDarkThresholdLevel of 0.325
## (AnimModel.cpp:598). A backend that cannot tell day from night returns 1.0 and leaves every such
## light off.
func get_light_level() -> float:
    return 1.0


## Unit vector the wind blows along. Horizontal for now - the environment node carries a compass
## bearing - but a vector so that a backend with a vertical component needs no new API. The
## original keeps one wind for the whole simulation (simulationenvironment.cpp:255-268) and the
## smoke emitters drift with it.
func get_wind_direction() -> Vector3:
    var bearing:float = deg_to_rad((environment_node as MaszynaEnvironmentNode).wind_direction)
    return Vector3(cos(bearing), 0.0, sin(bearing))


## Wind speed in metres per second. A backend without weather of its own maps the environment
## node's own 0-1 wind_strength onto WIND_SPEED_MIN..WIND_SPEED_MAX.
func get_wind_strength() -> float:
    return lerpf(WIND_SPEED_MIN, WIND_SPEED_MAX, (environment_node as MaszynaEnvironmentNode).wind_strength)


func process(_delta: float) -> void:
    pass


## Holds the weather (rain, lightning) still while the runtime is paused (SimulationServer.simulation_pause()).
## A backend without weather of its own has nothing to hold.
func pause_weather() -> void:
    pass


func unpause_weather() -> void:
    pass


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


func _bind_cabin_light(world_light: DirectionalLight3D) -> void:
    var cabin_light: DirectionalLight3D = world_light.get_node_or_null(NodePath(CABIN_LIGHT_NAME)) as DirectionalLight3D
    if cabin_light:
        _cabin_lights[world_light] = cabin_light


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
        and (environment_node as MaszynaEnvironmentNode).cabin_view
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
