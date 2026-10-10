extends MaszynaGutTest

## A scenery model placement that declares a light gets a real RenderingServer light: registered
## with E3DRenderingServer, streamed in by SceneryStreamingServer around the camera, switched by
## the light level. This is the whole path a street lamp in a scenery takes, with a model loader
## of its own instead of the game directory.

const NIGHT_LIGHT_LEVEL: float = 0.1
const DAY_LIGHT_LEVEL: float = 1.0
const MIDDAY: float = 12.0
## maszyna/scenery/lights/mode
const LIGHTS_OFF: int = 0
const ECONOMY: int = 1
const HIGH_QUALITY: int = 2
## Long enough for a planning pass (SceneryStreamingServer.INTERVAL_MSEC) plus the build budget
const STREAM_TIMEOUT: float = 3.0

var _camera: Camera3D
var _instance: RID


func before_each() -> void:
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", HIGH_QUALITY)
    _camera = Camera3D.new()
    add_child_autoqfree(_camera)
    _camera.global_position = Vector3.ZERO
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())
    E3DRenderingServer.model_set_loader(_load_model)


func after_each() -> void:
    if _instance.is_valid():
        E3DRenderingServer.instance_free(_instance)
        _instance = RID()
    SceneryStreamingServer.streaming_set_camera(0)
    E3DRenderingServer.model_set_loader(Callable())
    E3DRenderingServer.environment_set_time(MIDDAY)
    E3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)


func test_declared_spotlight_is_streamed_in_and_follows_the_light_level() -> void:
    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    _instance = _register_lamp()
    E3DRenderingServer.instance_set_lights_modes(_instance, [float(E3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"], 1, "the model's FREE_SPOTLIGHT became a real spot light")
    assert_eq(statistics["omni"], 0)
    assert_eq(statistics["lit"], 1, "and it is lit, because it is dark")

    E3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)
    assert_eq(E3DRenderingServer.light_get_statistics()["lit"], 0, "unlit again in daylight")


func test_wide_cone_becomes_an_omni_light() -> void:
    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    # elektryczne/lampa_parkowa01 declares 117 degrees, past Godot's spot limit
    _instance = _register_lamp(117.2)
    E3DRenderingServer.instance_set_lights_modes(_instance, [float(E3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["omni"], 1, "a cone wider than 90 degrees cannot be a spot")
    assert_eq(statistics["spot"], 0)


func test_economy_mode_merges_a_multi_armed_lamp_into_one_light() -> void:
    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", ECONOMY)
    _instance = _register_lamp(40.0, 5)
    E3DRenderingServer.instance_set_lights_modes(_instance, [float(E3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"] + statistics["omni"], 1, "five arms become one light")
    assert_eq(statistics["lit"], 1)


func test_lights_off_creates_no_real_lights() -> void:
    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", LIGHTS_OFF)
    _instance = _register_lamp(40.0, 5)
    E3DRenderingServer.instance_set_lights_modes(_instance, [float(E3DRenderingServer.LIGHT_MODE_DARK)])

    await wait_seconds(1.0)
    var statistics: Dictionary = E3DRenderingServer.light_get_statistics()
    assert_eq(statistics["spot"] + statistics["omni"], 0, "only the lit submodels, no real lights")


func test_high_quality_keeps_every_arm() -> void:
    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", HIGH_QUALITY)
    _instance = _register_lamp(40.0, 5)
    E3DRenderingServer.instance_set_lights_modes(_instance, [float(E3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"] + statistics["omni"], 5, "one light per arm")


func _register_lamp(light_angle: float = 40.0, arms: int = 1) -> RID:
    return E3DRenderingServer.instance_register(
        "models/test", "test_lamp_%d_%d" % [int(light_angle), arms], [], Transform3D(), 0.0, 100.0,
        _camera.get_world_3d().scenario,
    )


## Waits for the streaming server to build the registered placement
func _await_lights() -> Dictionary:
    var waited: float = 0.0
    while waited < STREAM_TIMEOUT:
        var statistics: Dictionary = E3DRenderingServer.light_get_statistics()
        if statistics["total"] > 0:
            return statistics
        await wait_seconds(0.1)
        waited += 0.1
    return E3DRenderingServer.light_get_statistics()


## Stands in for E3DModelManager: a lamp with one light_on00 holding a FREE_SPOTLIGHT
func _load_model(_data_path: String, model_filename: String) -> E3DModel:
    var angle: float = float(model_filename.get_slice("_", 2))
    var arms: int = int(model_filename.get_slice("_", 3))
    var model: E3DModel = E3DModel.new()
    var light_on: E3DSubModel = E3DSubModel.new()
    light_on.resource_name = "light_on00"
    light_on.submodel_type = E3DSubModel.SUBMODEL_TRANSFORM
    var arm_submodels: Array[E3DSubModel] = []
    for i in range(maxi(arms, 1)):
        var spotlight: E3DSubModel = E3DSubModel.new()
        spotlight.resource_name = "fspot%02d" % i
        spotlight.submodel_type = E3DSubModel.SUBMODEL_FREE_SPOTLIGHT
        spotlight.light_angle = angle
        spotlight.light_range = 40.0
        spotlight.transform = Transform3D(Basis(), Vector3(float(i) - 2.0, 8.0, 0.0))
        arm_submodels.append(spotlight)
    light_on.submodels = arm_submodels
    model.submodels = [light_on]
    var definition: E3DModelLightDefinition = E3DModelLightDefinition.new()
    definition.on_submodel_path = NodePath("light_on00")
    model.register_light("00", definition)
    return model
