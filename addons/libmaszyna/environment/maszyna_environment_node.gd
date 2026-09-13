@tool
extends Node
class_name MaszynaEnvironmentNode

const GENERATED_WORLD_NAME: StringName = &"_WorldEnvironment"
const MINIMUM_FOG_RANGE: float = 0.001
const MINIMUM_VOLUMETRIC_FOG_LENGTH: float = 64.0
const MAXIMUM_FOG_OPACITY: float = 0.999
const VOLUMETRIC_FOG_OPACITY_SCALE: float = 0.1

@export_category("Current Time")
@export var use_system_time: bool = false:
    set(value):
        use_system_time = value
        _dirty_time = true

@export_range(0.0, 23.9998) var current_time: float = 8.0:
    set(value):
        if not value == current_time:
            current_time = value
            _dirty_time = true

@export_range(1, 31) var day: int = 3:
    set(value):
        if not value == day:
            day = value
            _dirty_time = true

@export_range(1, 12) var month: int = 5:
    set(value):
        if not value == month:
            month = value
            _dirty_time = true

@export_range(0, 9999) var year: int = 2026:
    set(value):
        if not value == year:
            year = value
            _dirty_time = true

@export var weather: MaszynaEnvironment.Weather = MaszynaEnvironment.Weather.WEATHER_CLEAR:
    set(value):
        if not value == weather:
            weather = value
            MaterialManager.weather = weather

@export_category("Location")
@export_range(-90.0, 90.0, 0.001, "suffix:°") var latitude: float = 50.271:
    set(value):
        if not value == latitude:
            latitude = value
            _dirty_time = true

@export_range(-180.0, 180.0, 0.001, "suffix:°") var longitude: float = 19.04:
    set(value):
        if not value == longitude:
            longitude = value
            _dirty_time = true

@export_category("Configuration")
@export_range(-12, 14, 1) var timezone_offset: int = 1:
    set(value):
        timezone_offset = value
        _dirty_time = true

@export_range(0.0, 1000.0) var simulation_speed: float = 1.0:
    set(value):
        simulation_speed = value
        _dirty_time = true

@export_category("Visuals")
@export_range(0.0, 1.0, 0.01) var cloudiness: float = 0.5:
    set(value):
        cloudiness = value
        _dirty_visuals = true

@export_custom(PROPERTY_HINT_RANGE, "-180,180,0.1,radians_as_degrees")
var wind_direction: float = deg_to_rad(135.0):
    set(value):
        wind_direction = value
        _dirty_visuals = true

@export_group("Day", "day_")
@export_subgroup("Light", "day_light_")
@export var day_light_shadow_enabled: bool = true:
    set(value):
        day_light_shadow_enabled = value
        _dirty_visuals = true

@export var day_light_shadow_mode: DirectionalLight3D.ShadowMode = (
    DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS
):
    set(value):
        day_light_shadow_mode = value
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.01, "or_greater") var day_light_shadow_blur: float = 1.0:
    set(value):
        day_light_shadow_blur = value
        _dirty_visuals = true

@export_range(0.0, 1.0, 0.01) var day_light_shadow_opacity: float = 1.0:
    set(value):
        day_light_shadow_opacity = value
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.001, "or_greater") var day_light_shadow_bias: float = 0.1:
    set(value):
        day_light_shadow_bias = value
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.001, "or_greater")
var day_light_shadow_normal_bias: float = 2.0:
    set(value):
        day_light_shadow_normal_bias = value
        _dirty_visuals = true

@export_range(0.0, 10000.0, 1.0, "suffix:m")
var day_light_shadow_max_distance: float = 100.0:
    set(value):
        day_light_shadow_max_distance = value
        _dirty_visuals = true

@export_range(0.0, 16.0, 0.001, "or_greater")
var day_light_volumetric_fog_energy: float = 1.0:
    set(value):
        day_light_volumetric_fog_energy = value
        _dirty_visuals = true

@export_group("Night", "night_")
@export_subgroup("Light", "night_light_")
@export var night_light_shadow_enabled: bool = true:
    set(value):
        night_light_shadow_enabled = value
        _dirty_visuals = true

@export var night_light_shadow_mode: DirectionalLight3D.ShadowMode = (
    DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS
):
    set(value):
        night_light_shadow_mode = value
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.01, "or_greater") var night_light_shadow_blur: float = 1.0:
    set(value):
        night_light_shadow_blur = value
        _dirty_visuals = true

@export_range(0.0, 1.0, 0.01) var night_light_shadow_opacity: float = 1.0:
    set(value):
        night_light_shadow_opacity = value
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.001, "or_greater") var night_light_shadow_bias: float = 0.1:
    set(value):
        night_light_shadow_bias = value
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.001, "or_greater")
var night_light_shadow_normal_bias: float = 2.0:
    set(value):
        night_light_shadow_normal_bias = value
        _dirty_visuals = true

@export_range(0.0, 10000.0, 1.0, "suffix:m")
var night_light_shadow_max_distance: float = 100.0:
    set(value):
        night_light_shadow_max_distance = value
        _dirty_visuals = true

@export_range(0.0, 16.0, 0.001, "or_greater")
var night_light_volumetric_fog_energy: float = 1.0:
    set(value):
        night_light_volumetric_fog_energy = value
        _dirty_visuals = true

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

@export_group("Fog")
@export var fog_enabled: bool = true:
    set(value):
        fog_enabled = value
        _dirty_visuals = true

@export_range(0.0, 1.0, 0.001) var fog_density: float = 0.2:
    set(value):
        fog_density = value
        _dirty_visuals = true

@export_range(0.0, 5000.0, 1.0, "suffix:m") var fog_range_start: float = 200.0:
    set(value):
        fog_range_start = value
        _dirty_visuals = true

@export_range(0.0, 5000.0, 1.0, "suffix:m") var fog_range_end: float = 1000.0:
    set(value):
        fog_range_end = value
        _dirty_visuals = true

var season: MaszynaEnvironment.Season = MaszynaEnvironment.Season.SEASON_SUMMER:
    set(value):
        if not value == season:
            season = value
            MaterialManager.season = season

var _world_environment: WorldEnvironment
var _environment: Environment
var _sky_environment: MaszynaSkyEnvironment
var _dirty_time: bool = true
var _dirty_visuals: bool = true


func _ready() -> void:
    _ensure_environment()
    update()
    _process_dirty()


func _enter_tree() -> void:
    UserSettings.config_changed.connect(_on_user_settings_changed)


func _exit_tree() -> void:
    UserSettings.config_changed.disconnect(_on_user_settings_changed)


func _process(_delta: float) -> void:
    _process_dirty()


func update() -> void:
    _dirty_time = true
    _dirty_visuals = true


func set_date(next_year: int, next_month: int, next_day: int) -> void:
    if not _sky_environment:
        year = next_year
        month = next_month
        day = next_day
        return

    var normalized_date: Vector3i = _sky_environment.set_date(
        next_year, next_month, next_day
    )
    year = normalized_date.x
    month = normalized_date.y
    day = normalized_date.z


func _process_dirty() -> void:
    if _dirty_visuals:
        _dirty_visuals = false
        _apply_visual_configuration()

    if _dirty_time:
        _dirty_time = false
        _apply_time_configuration()


func _ensure_environment() -> void:
    if is_instance_valid(_world_environment):
        return

    _world_environment = get_node_or_null(NodePath(GENERATED_WORLD_NAME)) as WorldEnvironment
    if _world_environment:
        _environment = _world_environment.environment
        _sky_environment = _create_sky_environment()
        _sky_environment.bind_nodes(_world_environment)
        return

    _world_environment = _create_world_environment()
    add_child(_world_environment, false, INTERNAL_MODE_BACK)


func _create_world_environment() -> WorldEnvironment:
    var world_environment: WorldEnvironment = WorldEnvironment.new()
    world_environment.name = GENERATED_WORLD_NAME

    _sky_environment = _create_sky_environment()
    _environment = _create_environment()
    world_environment.environment = _environment
    world_environment.camera_attributes = CameraAttributesPractical.new()
    _sky_environment.create_nodes(world_environment, _environment)

    return world_environment


func _create_sky_environment() -> MaszynaSkyEnvironment:
    return TokisanSky3DMaszynaEnvironment.new(self)


func _create_environment() -> Environment:
    var environment: Environment = Environment.new()
    environment.background_mode = Environment.BG_SKY
    environment.sky = _sky_environment.create_sky()
    environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
    environment.ambient_light_color = Color.WHITE
    environment.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
    environment.fog_mode = Environment.FOG_MODE_DEPTH
    environment.fog_light_color = Color(0.3605356, 0.39691955, 0.44612736, 1.0)
    environment.fog_light_energy = 0.7
    environment.fog_sun_scatter = 0.07
    environment.volumetric_fog_anisotropy = 0.0
    environment.volumetric_fog_detail_spread = 1.0
    return environment


func _apply_visual_configuration() -> void:
    if not _environment or not _sky_environment:
        return

    var effective_fog_end: float = maxf(fog_range_end, fog_range_start + MINIMUM_FOG_RANGE)
    var volumetric_fog_length: float = maxf(
        effective_fog_end, MINIMUM_VOLUMETRIC_FOG_LENGTH
    )
    var fog_active: bool = fog_enabled and fog_density > 0.0

    _sky_environment.apply_visual_configuration()

    _environment.tonemap_mode = tonemap_mode
    _environment.tonemap_white = tonemap_white
    _environment.tonemap_agx_white = tonemap_agx_white
    _environment.tonemap_agx_contrast = tonemap_agx_contrast
    _environment.ssr_enabled = bool(UserSettings.get_setting("render", "ssr_enabled", true))
    _environment.ssao_enabled = bool(UserSettings.get_setting("render", "ssao_enabled", true))
    _environment.ssil_enabled = bool(UserSettings.get_setting("render", "ssil_enabled", true))
    _environment.sdfgi_enabled = bool(UserSettings.get_setting("render", "sdfgi_enabled", true))
    _environment.glow_enabled = true
    _environment.adjustment_enabled = adjustment_enabled
    _environment.fog_enabled = fog_active
    _environment.fog_density = fog_density
    _environment.fog_depth_begin = fog_range_start
    _environment.fog_depth_end = effective_fog_end
    _environment.fog_sky_affect = fog_density
    _environment.volumetric_fog_enabled = (
        fog_active and bool(UserSettings.get_setting("render", "volumetric_fog_enabled", true))
    )
    _environment.volumetric_fog_density = _fog_opacity_to_exponential_density(
        fog_density * VOLUMETRIC_FOG_OPACITY_SCALE, volumetric_fog_length
    )
    _environment.volumetric_fog_length = volumetric_fog_length
    _environment.volumetric_fog_sky_affect = 1.0


func _fog_opacity_to_exponential_density(opacity: float, distance: float) -> float:
    var effective_opacity: float = clampf(opacity, 0.0, MAXIMUM_FOG_OPACITY)
    return -log(1.0 - effective_opacity) / distance


func _on_user_settings_changed() -> void:
    _dirty_visuals = true


func _apply_time_configuration() -> void:
    if not _sky_environment:
        return

    _sky_environment.apply_time_configuration()

    var normalized_date: Vector3i = _sky_environment.get_date()
    year = normalized_date.x
    month = normalized_date.y
    day = normalized_date.z
    current_time = _sky_environment.get_current_time()
    season = _season_from_year_day(_get_year_day(day, month, year))


func _season_from_year_day(year_day: int) -> MaszynaEnvironment.Season:
    # Thresholds are taken from the original simulator code.
    if year_day <= 65:
        return MaszynaEnvironment.Season.SEASON_WINTER
    if year_day <= 158:
        return MaszynaEnvironment.Season.SEASON_SPRING
    if year_day <= 252:
        return MaszynaEnvironment.Season.SEASON_SUMMER
    if year_day <= 341:
        return MaszynaEnvironment.Season.SEASON_AUTUMN
    return MaszynaEnvironment.Season.SEASON_WINTER


func _get_year_day(current_day: int, current_month: int, current_year: int) -> int:
    var month_lengths: Array[int] = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
    var year_day: int = current_day
    var month_index: int = 0

    if _is_leap_year(current_year):
        month_lengths[1] = 29

    while month_index < current_month - 1:
        year_day += month_lengths[month_index]
        month_index += 1

    return year_day


func _is_leap_year(current_year: int) -> bool:
    return ((current_year % 4) == 0 and not (current_year % 100) == 0) or (current_year % 400) == 0
