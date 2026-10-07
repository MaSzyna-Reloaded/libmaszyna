@tool
extends Node
class_name MaszynaEnvironmentNode

const GENERATED_WORLD_NAME: StringName = &"_WorldEnvironment"
## Group the player sets cabin_view on when switching between the cabin and the exterior view
const GROUP: StringName = &"maszyna_environment"
## Render layer of the cab interior - with maszyna/cabin/improve_shadows_quality the cab is lit by a
## sun of its own that reaches only this layer (MaszynaSkyEnvironment._create_cabin_light())
const CABIN_RENDER_LAYER: int = 1 << 18
## How often the time of day, the light level and the wind are pushed to E3DRenderingServer, which
## decides from the first two which scenery lights are lit (see _push_environment_state())
const LIGHT_STATE_UPDATE_INTERVAL: float = 1.0
## The original's Global.Overcast runs 0-1 for the cloud cover and on up to 2 for precipitation
## (simulationenvironment.cpp:64-71); MaszynaScenery turns its heaviest step into a precipitation
## of 0.4 (maszyna_scenery.gd PRECIPITATION_MEDIUM), which here is an overcast of 2 again
const OVERCAST_MAX: float = 2.0
const OVERCAST_FULL_PRECIPITATION: float = 0.4
## The original's fog range with the fog switched off - the longest a scenery may declare
## (simulationstateserializer.cpp:216)
const FOG_RANGE_MAX: float = 25000.0
## Glow as tuned in forest-test-scene materials/environment_filmic.tres - what the player's Graphics
## settings (render/glow_intensity, render/bloom_intensity) fall back to
const GLOW_INTENSITY_DEFAULT: float = 1.37
const BLOOM_INTENSITY_DEFAULT: float = 0.3
## The player's gamma (render/gamma): the picture's mid-tones as x^(1/gamma) - above 1 brighter,
## below darker, black and white staying put - through the environment's colour correction, a
## curve of this many steps; at 1 there is no correction at all
const GAMMA_DEFAULT: float = 1.0
const GAMMA_CURVE_STEPS: int = 256
const WEATHER_PRESETS: Dictionary = {
    MaszynaEnvironment.Weather.WEATHER_CLEAR: {
        "precipitation": 0.0, "cloudiness": 0.1, "fog_density": 0.075, "wind_strength": 0.2,
    },
    MaszynaEnvironment.Weather.WEATHER_CLOUDY: {
        "precipitation": 0.0, "cloudiness": 0.7, "fog_density": 0.15, "wind_strength": 0.4,
    },
    MaszynaEnvironment.Weather.WEATHER_RAIN: {
        "precipitation": 0.8, "cloudiness": 0.9, "fog_density": 0.3, "wind_strength": 0.6,
    },
    MaszynaEnvironment.Weather.WEATHER_SNOW: {
        "precipitation": 0.0, "cloudiness": 0.9, "fog_density": 0.3, "wind_strength": 0.5,
    },
}

## The environment applied a change of its configuration - a preset, a scenery's own declarations
## or a property written from anywhere. Whoever shows this state reacts to this instead of reading
## the node every frame; the running clock is deliberately not announced here (see _process()).
signal configuration_changed

@export_category("Time")
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

@export_range(-12, 14, 1) var timezone_offset: int = 1:
    set(value):
        timezone_offset = value
        _dirty_time = true

## The fastest the simulation runs, as many times the wall clock
const MAX_SIMULATION_SPEED: float = 100.0

## How many times the wall clock the simulation runs - SimulationServer's; the node only sets it,
## from the scene and the inspector
@export_range(0.0, MAX_SIMULATION_SPEED) var simulation_speed: float = 1.0:
    set(value):
        SimulationServer.simulation_speed = clampf(value, 0.0, MAX_SIMULATION_SPEED)
    get:
        return SimulationServer.simulation_speed

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

@export_category("Weather")
## Preset: changing it after the node is ready sets precipitation, cloudiness, fog density and
## wind strength (WEATHER_PRESETS); loading a scene keeps the saved values.
@export var weather: MaszynaEnvironment.Weather = MaszynaEnvironment.Weather.WEATHER_CLEAR:
    set(value):
        if not value == weather:
            weather = value
            _dirty_weather_preset = is_node_ready()
            _dirty_visuals = true

@export_range(0.0, 1.0, 0.01) var cloudiness: float = 0.5:
    set(value):
        cloudiness = value
        _dirty_visuals = true

## Compass bearing the wind blows towards, in degrees. A plain angle rather than a vector: the
## weather backends and the particle emitters only ever need a horizontal direction.
@export_range(0.0, 360.0, 0.1, "suffix:°") var wind_direction: float = 135.0:
    set(value):
        wind_direction = wrapf(value, 0.0, 360.0)
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.01) var wind_strength: float = 0.3:
    set(value):
        wind_strength = value
        _dirty_visuals = true

@export_range(0.0, 1.0, 0.01) var precipitation: float = 0.0:
    set(value):
        precipitation = value
        _dirty_visuals = true

## Air temperature, published to SimulationServer
@export_range(-15.0, 45.0, 0.1, "suffix:°C") var temperature: float = 15.0

@export_group("Fog")
@export var fog_enabled: bool = true:
    set(value):
        fog_enabled = value
        _dirty_visuals = true

## How much of the view the fog covers at fog_distance: 0 - none, 1 - fully opaque. The sky backend
## adds its own share on top (day/night base fog, rain), so this is the scenery's part of it.
@export_range(0.0, 1.0, 0.001) var fog_density: float = 0.15:
    set(value):
        fog_density = value
        _dirty_visuals = true

## Distance the fog reaches fog_density at, growing linearly up to it. The sky backend may tell
## day from night
## (maszyna/weather/fog/day_distance_factor, night_distance_factor).
@export_range(10.0, 25000.0, 1.0, "suffix:m") var fog_distance: float = 470.0:
    set(value):
        fog_distance = value
        _dirty_visuals = true

@export_category("Adjustments")
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
var _dirty_weather_preset: bool = false
var _dirty_lights: bool = false
var _light_state_elapsed: float = 0.0
## The gamma the colour correction curve was made for - it is made again only when that changes -
## and the curve itself, none at the default
var _gamma: float = GAMMA_DEFAULT
var _gamma_curve: ImageTexture = null

## Cabin view (the player in a cab) - the cab light casts its shadows then
var cabin_view: bool = false:
    set(value):
        if not value == cabin_view:
            cabin_view = value
            _dirty_lights = true


func _ready() -> void:
    _ensure_environment()
    _sky_environment.apply_light_configuration()
    update()
    _process_dirty()


func _enter_tree() -> void:
    add_to_group(GROUP)
    # the time of day passes while the environment is there (SimulationServer's clock)
    if not Engine.is_editor_hint():
        SimulationServer.clock_hold()
    UserSettings.config_changed.connect(_on_user_settings_changed)
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    SimulationServer.simulation_paused.connect(_on_runtime_paused)
    SimulationServer.simulation_unpaused.connect(_on_runtime_unpaused)


func _exit_tree() -> void:
    if not Engine.is_editor_hint():
        SimulationServer.clock_release()
    UserSettings.config_changed.disconnect(_on_user_settings_changed)
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)
    SimulationServer.simulation_paused.disconnect(_on_runtime_paused)
    SimulationServer.simulation_unpaused.disconnect(_on_runtime_unpaused)


func _process(delta: float) -> void:
    _process_dirty()
    _sky_environment.sync_cabin_lights()
    _sky_environment.process(delta)
    _sync_time()
    _push_environment_state(delta)
    # Running time is only mirrored here; it must not be re-applied as a configuration change.
    _dirty_time = false


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
    var applied: bool = false

    if _dirty_weather_preset:
        _dirty_weather_preset = false
        _apply_weather_preset()
        applied = true

    if _dirty_visuals:
        _dirty_visuals = false
        _apply_visual_configuration()
        applied = true

    if _dirty_lights:
        _dirty_lights = false
        _sky_environment.apply_light_configuration()
        applied = true

    if _dirty_time:
        _dirty_time = false
        _apply_time_configuration()
        applied = true

    if applied:
        configuration_changed.emit()


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
    return GndSkydomeMaszynaEnvironment.new(self)


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
    # Glow tuned as in forest-test-scene materials/environment_filmic.tres (its intensity and bloom
    # are the player's, _apply_visual_configuration()); the luminance cap keeps small specular
    # highlights (e.g. rain streaks) from blooming into large blobs.
    environment.glow_normalized = true
    environment.glow_strength = 0.8
    environment.glow_hdr_threshold = 1.37
    environment.glow_hdr_luminance_cap = 0.18
    return environment


func _apply_weather_preset() -> void:
    var preset: Dictionary = WEATHER_PRESETS[weather]
    precipitation = preset["precipitation"]
    cloudiness = preset["cloudiness"]
    fog_density = preset["fog_density"]
    wind_strength = preset["wind_strength"]


func _apply_visual_configuration() -> void:
    # Any precipitation switches the materials to their "rain" variant. It was blocked for a while
    # as bad looking: the wet texture ("rain: { texture2: ... }") is a reflection map and was bound
    # as a normal map back then (FINDINGS.md, "texture2: is not always the normal map").
    MaterialManager.weather = (
        MaszynaEnvironment.Weather.WEATHER_RAIN if precipitation > 0.0 else weather
    )
    # rain_params of the "rain_windscreen" materials (opengl33renderer.cpp:752-754): the share of
    # active droplets and the time they take to return after a wiper pass
    RenderingServer.global_shader_parameter_set("maszyna_rain_intensity", precipitation)
    RenderingServer.global_shader_parameter_set("maszyna_wiper_regen_time", lerpf(15.0, 1.0, precipitation))
    # the free spotlights' points and glare (types/free_spotlight*.gdshader) follow the original's
    # Global.Overcast and m_fogrange (opengl33renderer.cpp:5009), which fog_distance is a multiple of
    RenderingServer.global_shader_parameter_set(
        "maszyna_overcast", clampf(cloudiness + precipitation / OVERCAST_FULL_PRECIPITATION, 0.0, OVERCAST_MAX))
    RenderingServer.global_shader_parameter_set(
        "maszyna_fog_range",
        fog_distance / float(ProjectSettings.get_setting(
            MaszynaSkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
            MaszynaSkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT))
        if fog_enabled
        else FOG_RANGE_MAX)
    if not _environment or not _sky_environment:
        return

    # fog_density is a multiplier; zero fades the fog out instead of switching it off.
    var fog_active: bool = fog_enabled

    _sky_environment.apply_visual_configuration()

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
        MaszynaSkyEnvironment.FOG_AERIAL_PERSPECTIVE_SETTING,
        MaszynaSkyEnvironment.FOG_AERIAL_PERSPECTIVE_DEFAULT))
    _environment.fog_depth_curve = maxf(MaszynaSkyEnvironment.FOG_CURVE_MIN, float(
        ProjectSettings.get_setting(
            MaszynaSkyEnvironment.FOG_CURVE_SETTING, MaszynaSkyEnvironment.FOG_CURVE_DEFAULT)))
    _environment.volumetric_fog_enabled = (
        fog_active and bool(UserSettings.get_setting("render", "volumetric_fog_enabled", true))
    )


func _on_user_settings_changed() -> void:
    _dirty_visuals = true


## The fog, the shadows and the lights follow their project settings while the scenery runs
func _on_project_settings_changed() -> void:
    _dirty_visuals = true
    _dirty_lights = true


func _on_runtime_paused() -> void:
    _sky_environment.pause_weather()


func _on_runtime_unpaused() -> void:
    _sky_environment.unpause_weather()


func _apply_time_configuration() -> void:
    if not _sky_environment:
        return

    _sky_environment.apply_time_configuration()
    _sync_time()
    # a time set, not run: the clock jumps to it
    SimulationServer.time_of_day = current_time


## Scenery lights set to come on automatically are decided by E3DRenderingServer out of the time of
## day and the light level, and the particle emitters drift with the wind; SimulationServer carries
## the time, the light level and the temperature for everything else (a cab screen's clock, once
## a second, is exactly the resolution it shows). None of the three
## changes fast enough to be worth pushing every frame - a whole scenery is re-resolved on each
## push - so they go at a fixed interval, and at once when the time was jumped rather than merely
## running.
func _push_environment_state(delta: float) -> void:
    _light_state_elapsed += delta
    if _light_state_elapsed < LIGHT_STATE_UPDATE_INTERVAL and not _dirty_time:
        return
    _light_state_elapsed = 0.0
    var light_level:float = _sky_environment.get_light_level()
    E3DRenderingServer.environment_set_time(current_time)
    E3DRenderingServer.environment_set_light_level(light_level)
    # Global.fLuminance of the free spotlights' glare (types/free_spotlight_glare.gdshader)
    RenderingServer.global_shader_parameter_set("maszyna_light_level", light_level)
    SimulationServer.light_level = light_level
    SimulationServer.air_temperature = temperature
    E3DRenderingServer.environment_set_wind(
        _sky_environment.get_wind_strength(), _sky_environment.get_wind_direction()
    )


func _sync_time() -> void:
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
