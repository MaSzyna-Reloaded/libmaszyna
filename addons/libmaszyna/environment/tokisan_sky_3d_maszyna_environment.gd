@tool
extends MaszynaSkyEnvironment
class_name TokisanSky3DMaszynaEnvironment

const SKY_SHADER: Shader = preload("res://addons/sky_3d/shaders/SkyMaterial.gdshader")
const SKY_DOME_NAME: StringName = &"SkyDome"
const TIME_OF_DAY_NAME: StringName = &"TimeOfDay"
const CELESTIAL_FADE_CLOUDINESS_START: float = 0.5
const SUN_DISK_INTENSITY: float = 30.0
const MOON_COLOR: Color = Color.WHITE
const STARMAP_COLOR: Color = Color(0.709804, 0.709804, 0.709804, 0.854902)
const STAR_FIELD_COLOR: Color = Color.WHITE

var sky_dome: SkyDome
var time_of_day: TimeOfDay
var sun_light: DirectionalLight3D
var moon_light: DirectionalLight3D


func create_sky() -> Sky:
    var sky_material: ShaderMaterial = ShaderMaterial.new()
    sky_material.shader = SKY_SHADER

    var sky: Sky = Sky.new()
    sky.sky_material = sky_material
    return sky


func create_nodes(world_environment: WorldEnvironment, environment: Environment) -> void:
    sun_light = DirectionalLight3D.new()
    sun_light.name = &"SunLight"
    sun_light.shadow_enabled = true
    world_environment.add_child(sun_light, false, Node.INTERNAL_MODE_BACK)

    moon_light = DirectionalLight3D.new()
    moon_light.name = &"MoonLight"
    moon_light.shadow_enabled = true
    world_environment.add_child(moon_light, false, Node.INTERNAL_MODE_BACK)

    sky_dome = SkyDome.new()
    sky_dome.name = SKY_DOME_NAME
    sky_dome.environment = environment
    sky_dome.fog_visible = false
    world_environment.add_child(sky_dome, false, Node.INTERNAL_MODE_BACK)
    sky_dome.sun_light_path = NodePath("../SunLight")
    sky_dome.moon_light_path = NodePath("../MoonLight")

    time_of_day = TimeOfDay.new()
    time_of_day.name = TIME_OF_DAY_NAME
    time_of_day.editor_time_enabled = false
    world_environment.add_child(time_of_day, false, Node.INTERNAL_MODE_BACK)
    time_of_day.dome_path = NodePath("../SkyDome")


func bind_nodes(world_environment: WorldEnvironment) -> void:
    sun_light = world_environment.get_node_or_null("SunLight") as DirectionalLight3D
    moon_light = world_environment.get_node_or_null("MoonLight") as DirectionalLight3D
    sky_dome = world_environment.get_node_or_null(NodePath(SKY_DOME_NAME)) as SkyDome
    time_of_day = world_environment.get_node_or_null(NodePath(TIME_OF_DAY_NAME)) as TimeOfDay


func apply_visual_configuration() -> void:
    if not sky_dome:
        return

    var clouds_visible: bool = environment_node.cloudiness > 0.0
    var celestial_visibility: float = 1.0 - smoothstep(
        CELESTIAL_FADE_CLOUDINESS_START, 1.0, environment_node.cloudiness
    )

    sky_dome.cirrus_visible = clouds_visible
    sky_dome.cumulus_visible = clouds_visible
    sky_dome.cirrus_coverage = environment_node.cloudiness
    sky_dome.cumulus_coverage = clampf(environment_node.cloudiness * 1.1, 0.0, 1.0)
    sky_dome.wind_direction = environment_node.wind_direction

    # Tokisan's screen-space fog bypasses Godot's Environment fog.
    sky_dome.fog_visible = false

    sky_dome.sun_disk_intensity = SUN_DISK_INTENSITY * celestial_visibility
    sky_dome.moon_color = MOON_COLOR.lerp(
        Color(0.0, 0.0, 0.0, 1.0), 1.0 - celestial_visibility
    )
    sky_dome.starmap_color = STARMAP_COLOR.lerp(
        Color(0.0, 0.0, 0.0, 1.0), 1.0 - celestial_visibility
    )
    sky_dome.star_field_color = STAR_FIELD_COLOR.lerp(
        Color(0.0, 0.0, 0.0, 1.0), 1.0 - celestial_visibility
    )
    _apply_directional_light_configuration(
        sun_light,
        environment_node.day_light_shadow_enabled,
        environment_node.day_light_shadow_mode,
        environment_node.day_light_shadow_blur,
        environment_node.day_light_shadow_opacity,
        environment_node.day_light_shadow_bias,
        environment_node.day_light_shadow_normal_bias,
        environment_node.day_light_shadow_max_distance,
        environment_node.day_light_volumetric_fog_energy
    )
    _apply_directional_light_configuration(
        moon_light,
        environment_node.night_light_shadow_enabled,
        environment_node.night_light_shadow_mode,
        environment_node.night_light_shadow_blur,
        environment_node.night_light_shadow_opacity,
        environment_node.night_light_shadow_bias,
        environment_node.night_light_shadow_normal_bias,
        environment_node.night_light_shadow_max_distance,
        environment_node.night_light_volumetric_fog_energy
    )


func _apply_directional_light_configuration(
    light: DirectionalLight3D,
    shadow_enabled: bool,
    shadow_mode: DirectionalLight3D.ShadowMode,
    shadow_blur: float,
    shadow_opacity: float,
    shadow_bias: float,
    shadow_normal_bias: float,
    shadow_max_distance: float,
    volumetric_fog_energy: float
) -> void:
    light.shadow_enabled = shadow_enabled
    light.directional_shadow_mode = shadow_mode
    light.shadow_blur = shadow_blur
    light.shadow_opacity = shadow_opacity
    light.shadow_bias = shadow_bias
    light.shadow_normal_bias = shadow_normal_bias
    light.directional_shadow_max_distance = shadow_max_distance
    light.light_volumetric_fog_energy = volumetric_fog_energy


func set_date(year: int, month: int, day: int) -> Vector3i:
    if not time_of_day:
        return Vector3i(year, month, day)

    # The order matters because TimeOfDay normalizes each component in its setter.
    time_of_day.year = year
    time_of_day.month = month
    time_of_day.day = day
    return Vector3i(time_of_day.year, time_of_day.month, time_of_day.day)


func apply_time_configuration() -> void:
    if not time_of_day:
        return

    time_of_day.editor_time_enabled = false
    time_of_day.system_sync = environment_node.use_system_time
    time_of_day.minutes_per_day = (
        1440.0 / environment_node.simulation_speed
        if environment_node.simulation_speed > 0.0
        else 0.0
    )
    time_of_day.update_interval = time_of_day.minutes_per_day * 0.001
    time_of_day.latitude = deg_to_rad(environment_node.latitude)
    time_of_day.longitude = deg_to_rad(environment_node.longitude)
    time_of_day.utc = float(environment_node.timezone_offset)

    if environment_node.use_system_time:
        time_of_day.set_from_datetime_dict(Time.get_datetime_dict_from_system())
    else:
        set_date(environment_node.year, environment_node.month, environment_node.day)
        time_of_day.current_time = environment_node.current_time


func get_date() -> Vector3i:
    if not time_of_day:
        return Vector3i.ZERO
    return Vector3i(time_of_day.year, time_of_day.month, time_of_day.day)


func get_current_time() -> float:
    return time_of_day.current_time if time_of_day else 0.0
