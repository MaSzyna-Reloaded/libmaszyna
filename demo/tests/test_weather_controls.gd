extends MaszynaGutTest

const WEATHER_CONTROLS_SCENE: PackedScene = preload("res://addons/libmaszyna/debug_hud/weather_controls.tscn")

var _previous_weather: MaszynaEnvironment.Weather


func before_each() -> void:
    _previous_weather = MaterialManager.weather


func after_each() -> void:
    MaterialManager.weather = _previous_weather
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
    environment_node.weather = MaszynaEnvironment.Weather.WEATHER_RAIN
    environment_node._process(0.0)
    controls._on_refresh_timeout()

    assert_almost_eq(controls._rain_slider.value, 0.8, 0.000001)
    assert_almost_eq(controls._cloud_slider.value, 0.9, 0.000001)
    assert_eq(environment_node.weather, MaszynaEnvironment.Weather.WEATHER_RAIN)
