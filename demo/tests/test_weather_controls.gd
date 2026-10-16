extends MaszynaGutTest

const WEATHER_CONTROLS_SCENE: PackedScene = preload("res://addons/libmaszyna/debug_hud/weather_controls.tscn")
## RoadTrafficServer's density setting (not bound) and its default
const TRAFFIC_DENSITY_SETTING: String = "maszyna/traffic/density"
const DEFAULT_TRAFFIC_DENSITY: float = 0.5
const TRAFFIC_DENSITY: float = 0.3
const OTHER_TRAFFIC_DENSITY: float = 0.8

var _previous_weather: SimulationServer.Weather
var _previous_traffic_density: float = 0.0


func before_each() -> void:
    _previous_weather = MaterialManager.weather
    _previous_traffic_density = ProjectSettings.get_setting(TRAFFIC_DENSITY_SETTING, DEFAULT_TRAFFIC_DENSITY)


func after_each() -> void:
    MaterialManager.weather = _previous_weather
    ProjectSettings.set_setting(TRAFFIC_DENSITY_SETTING, _previous_traffic_density)
    # the time scale slider sets SimulationServer's speed - every script after this one runs at it
    SimulationServer.simulation_reset_speed()
    # and the time and the weather are its too
    SimulationServer.environment_reset()


func test_controls_drive_environment_node() -> void:
    var environment_node: MaszynaEnvironmentNode = add_child_autofree(MaszynaEnvironmentNode.new())
    var controls: HFlowContainer = WEATHER_CONTROLS_SCENE.instantiate() as HFlowContainer
    add_child_autofree(controls)
    controls.attach_environment(environment_node)

    controls._rain_slider.value = 0.5
    controls._cloud_slider.value = 0.7
    controls._wind_strength_slider.value = 0.9
    controls._wind_direction_slider.value = 200.0
    controls._time_scale_slider.value = controls._time_scale_slider.max_value
    controls._time_slider.value = 18.5

    assert_almost_eq(environment_node.precipitation, 0.5, 0.000001)
    assert_almost_eq(environment_node.cloudiness, 0.7, 0.000001)
    assert_almost_eq(environment_node.wind_strength, 0.9, 0.000001)
    assert_almost_eq(environment_node.wind_direction, 200.0, 0.000001)
    assert_almost_eq(environment_node.simulation_speed, 20.0, 0.000001)
    assert_almost_eq(environment_node.current_time, 18.5, 0.000001)


func test_controls_follow_weather_preset() -> void:
    var environment_node: MaszynaEnvironmentNode = add_child_autofree(MaszynaEnvironmentNode.new())
    var controls: HFlowContainer = WEATHER_CONTROLS_SCENE.instantiate() as HFlowContainer
    add_child_autofree(controls)
    controls.attach_environment(environment_node)

    controls.visible = true
    environment_node.weather = SimulationServer.WEATHER_RAIN
    environment_node._process(0.0)
    controls._on_refresh_timeout()

    assert_almost_eq(controls._rain_slider.value, 0.8, 0.000001)
    assert_almost_eq(controls._cloud_slider.value, 0.9, 0.000001)
    assert_eq(environment_node.weather, SimulationServer.WEATHER_RAIN)


func test_traffic_slider_sets_the_traffic_density_and_follows_it() -> void:
    var controls: HFlowContainer = WEATHER_CONTROLS_SCENE.instantiate() as HFlowContainer
    add_child_autofree(controls)

    controls._traffic_density_slider.value = TRAFFIC_DENSITY
    assert_almost_eq(float(ProjectSettings.get_setting(TRAFFIC_DENSITY_SETTING)), TRAFFIC_DENSITY, 0.000001)

    # set elsewhere - the settings screen - it shows once the settings announce the change
    ProjectSettings.set_setting(TRAFFIC_DENSITY_SETTING, OTHER_TRAFFIC_DENSITY)
    await ProjectSettings.settings_changed
    assert_almost_eq(controls._traffic_density_slider.value, OTHER_TRAFFIC_DENSITY, 0.000001)
