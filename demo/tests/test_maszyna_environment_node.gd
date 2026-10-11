extends MaszynaGutTest

## The clock driven by hand: x100, a quarter of a second a frame (SimulationServer::MAX_FRAME_DELTA)
const CLOCK_SPEED: float = 100.0
const CLOCK_FRAME: float = 0.25
const SECONDS_PER_HOUR: float = 3600.0
## SimulationServer::SPEED_CHANGE_TIME_SETTING - how long the running speed takes to reach one set
const SPEED_CHANGE_TIME_SETTING: String = "maszyna/simulation/speed_change_time"
## SimulationServer::LIGHT_LEVEL_OVERCAST_FACTOR - full cloud cover dims the daylight by this much
const OVERCAST_FACTOR: float = 0.65

var _previous_season: MaszynaEnvironment.Season
var _previous_weather: MaszynaEnvironment.Weather
var _speed_change_time: Variant


func before_each() -> void:
    _previous_season = MaterialManager.season
    _previous_weather = MaterialManager.weather
    # the clock runs at the speed set, not on its way to it
    _speed_change_time = ProjectSettings.get_setting(SPEED_CHANGE_TIME_SETTING)
    ProjectSettings.set_setting(SPEED_CHANGE_TIME_SETTING, 0.0)
    # SimulationServer takes the setting on settings_changed, which Godot emits deferred
    await ProjectSettings.settings_changed


func after_each() -> void:
    MaterialManager.season = _previous_season
    MaterialManager.weather = _previous_weather
    ProjectSettings.set_setting(SPEED_CHANGE_TIME_SETTING, _speed_change_time)
    # the environment is SimulationServer's - every script after this one would find it as set here
    SimulationServer.environment_reset()


func test_generated_environment_is_not_packed() -> void:
    var scene_root: Node = add_child_autofree(Node.new())
    var environment_node: MaszynaEnvironmentNode = MaszynaEnvironmentNode.new()
    var packed_scene: PackedScene = PackedScene.new()

    scene_root.add_child(environment_node)
    environment_node.owner = scene_root

    assert_eq(packed_scene.pack(scene_root), OK)
    assert_eq(packed_scene.get_state().get_node_count(), 2)


func test_weather_preset_sets_weather_controls() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node.weather = MaszynaEnvironment.Weather.WEATHER_RAIN
    environment_node._process(0.0)

    assert_almost_eq(environment_node.precipitation, 0.8, 0.000001)
    assert_almost_eq(environment_node.cloudiness, 0.9, 0.000001)
    assert_almost_eq(environment_node.fog_density, 0.3, 0.000001)
    assert_almost_eq(environment_node.wind_strength, 0.6, 0.000001)
    assert_eq(MaterialManager.weather, MaszynaEnvironment.Weather.WEATHER_RAIN)

    environment_node.weather = MaszynaEnvironment.Weather.WEATHER_SNOW
    environment_node._process(0.0)

    assert_eq(MaterialManager.weather, MaszynaEnvironment.Weather.WEATHER_SNOW)


## The time the clock ran to stays when the speed changes - it is not set back to the configured one
func test_changing_simulation_speed_keeps_running_time() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    environment_node.current_time = 8.0
    environment_node._process(0.0)
    SimulationServer.simulation_speed = CLOCK_SPEED

    for _frame: int in roundi(SECONDS_PER_HOUR / (CLOCK_FRAME * CLOCK_SPEED)):
        SimulationServer.simulation_advance(CLOCK_FRAME)
    environment_node._process(1.0)
    environment_node.simulation_speed = 1.0
    environment_node._process(0.0)
    SimulationServer.simulation_reset_speed()

    assert_almost_eq(environment_node.current_time, 9.0, 0.001)


func test_proxies_season_and_weather_to_material_manager() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node.season = MaszynaEnvironment.Season.SEASON_WINTER

    assert_eq(MaterialManager.season, MaszynaEnvironment.Season.SEASON_WINTER)

    environment_node.precipitation = 0.0
    environment_node._process(0.0)

    assert_eq(MaterialManager.weather, MaszynaEnvironment.Weather.WEATHER_CLEAR)

    environment_node.precipitation = 0.5
    environment_node._process(0.0)

    assert_eq(MaterialManager.weather, MaszynaEnvironment.Weather.WEATHER_RAIN)


func test_sets_season_from_manual_date_thresholds() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    var cases: Array[Array] = [
        [5, 3, 2026, MaszynaEnvironment.Season.SEASON_WINTER],
        [7, 3, 2026, MaszynaEnvironment.Season.SEASON_SPRING],
        [7, 6, 2026, MaszynaEnvironment.Season.SEASON_SPRING],
        [8, 6, 2026, MaszynaEnvironment.Season.SEASON_SUMMER],
        [9, 9, 2026, MaszynaEnvironment.Season.SEASON_SUMMER],
        [10, 9, 2026, MaszynaEnvironment.Season.SEASON_AUTUMN],
        [7, 12, 2026, MaszynaEnvironment.Season.SEASON_AUTUMN],
        [8, 12, 2026, MaszynaEnvironment.Season.SEASON_WINTER],
    ]

    for case_data: Array in cases:
        environment_node.day = case_data[0]
        environment_node.month = case_data[1]
        environment_node.year = case_data[2]
        environment_node._process(0.0)

        assert_eq(environment_node.season, case_data[3])
        assert_eq(MaterialManager.season, case_data[3])


## The environment follows the simulation's clock (SimulationServer): an hour of it past midnight
## is the next day
func test_process_follows_the_clock_past_midnight() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    environment_node.current_time = 23.5
    environment_node.set_date(2026, 12, 31)
    environment_node._process(0.0)
    SimulationServer.simulation_speed = CLOCK_SPEED

    # an hour of simulation, a quarter of a second of real time a frame at a time
    for _frame: int in roundi(SECONDS_PER_HOUR / (CLOCK_FRAME * CLOCK_SPEED)):
        SimulationServer.simulation_advance(CLOCK_FRAME)
    environment_node._process(1.0)
    SimulationServer.simulation_reset_speed()

    assert_almost_eq(environment_node.current_time, 0.5, 0.001)
    assert_eq(Vector3i(environment_node.year, environment_node.month, environment_node.day), Vector3i(2027, 1, 1))


## The light level comes from the sun's altitude at the environment's place and time (sun.cpp),
## worked out by SimulationServer: full day at a summer noon, night at midnight, dimmed by the
## overcast
func test_light_level_follows_the_sun() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    environment_node.latitude = 50.271
    environment_node.longitude = 19.04
    environment_node.timezone_offset = 2
    environment_node.cloudiness = 0.0
    environment_node.set_date(2026, 6, 21)

    environment_node.current_time = 13.0
    assert_almost_eq(SimulationServer.light_level, 1.0, 0.000001)
    environment_node.current_time = 1.0
    assert_almost_eq(SimulationServer.light_level, 0.0, 0.000001)

    environment_node.current_time = 13.0
    environment_node.cloudiness = 1.0
    assert_almost_eq(SimulationServer.light_level, 1.0 - OVERCAST_FACTOR, 0.000001)


## The node hands the light level SimulationServer announces to E3DRenderingServer, with no frame
## between: a scenery lamp set to come on when dark lights at midnight and goes out at noon
func test_light_level_reaches_the_scenery_lights_through_the_node() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()
    environment_node.cloudiness = 0.0
    environment_node.set_date(2026, 6, 21)
    var parent: Node3D = add_child_autoqfree(Node3D.new())
    var rid: RID = build_lit_instance(parent)
    E3DRenderingServer.instance_set_lights_modes(rid, [float(E3DRenderingServer.LIGHT_MODE_DARK)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    environment_node.current_time = 1.0
    assert_true(light_on.visible, "lit at night")
    environment_node.current_time = 13.0
    assert_false(light_on.visible, "unlit in daylight")

    E3DRenderingServer.instance_free(rid)


func test_set_date_normalizes_invalid_date() -> void:
    var environment_node: MaszynaEnvironmentNode = _create_environment_node()

    environment_node.set_date(2025, 4, 31)

    assert_eq(environment_node.day, 1)
    assert_eq(environment_node.month, 5)
    assert_eq(environment_node.year, 2025)


func _create_environment_node() -> MaszynaEnvironmentNode:
    return add_child_autofree(MaszynaEnvironmentNode.new()) as MaszynaEnvironmentNode
