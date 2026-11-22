extends MaszynaGutTest

## A scenery model placement that declares a light gets a real RenderingServer light: registered
## with MaszynaE3DRenderingServer, streamed in by SceneryStreamingServer around the camera, switched by
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
    MaszynaE3DRenderingServer.model_set_loader(_load_model)


func after_each() -> void:
    if _instance.is_valid():
        MaszynaE3DRenderingServer.instance_free(_instance)
        _instance = RID()
    SceneryStreamingServer.streaming_set_camera(0)
    MaszynaE3DRenderingServer.model_set_loader(MaszynaE3DModelManager.load_model)
    MaszynaE3DRenderingServer.environment_set_time(MIDDAY)
    MaszynaE3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)


func test_declared_spotlight_is_streamed_in_and_follows_the_light_level() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    _instance = _register_lamp()
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"], 1, "the model's FREE_SPOTLIGHT became a real spot light")
    assert_eq(statistics["omni"], 0)
    assert_eq(statistics["lit"], 1, "and it is lit, because it is dark")

    MaszynaE3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)
    assert_eq(MaszynaE3DRenderingServer.light_get_statistics()["lit"], 0, "unlit again in daylight")


func test_wide_cone_becomes_an_omni_light() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    # elektryczne/lampa_parkowa01 declares 117 degrees, past Godot's spot limit
    _instance = _register_lamp(117.2)
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["omni"], 1, "a cone wider than 90 degrees cannot be a spot")
    assert_eq(statistics["spot"], 0)


func test_economy_mode_merges_a_multi_armed_lamp_into_one_light() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", ECONOMY)
    _instance = _register_lamp(40.0, 5)
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"] + statistics["omni"], 1, "five arms become one light")
    assert_eq(statistics["lit"], 1)


func test_lamps_cast_their_shadows_within_a_distance_of_their_own() -> void:
    # their shadows end long before the lights do, so the lamps near the camera share the atlas
    assert_eq(ProjectSettings.get_setting("maszyna/scenery/lights/shadow_distance"), 50.0)
    assert_lt(float(ProjectSettings.get_setting("maszyna/scenery/lights/shadow_distance")),
            float(ProjectSettings.get_setting("reloaded/scenery/lights/distance")))


func test_a_lamps_cone_covers_the_patch_its_model_draws() -> void:
    assert_eq(ProjectSettings.get_setting("maszyna/scenery/lights/lamp_cone_scale"), 1.0)


func test_lights_off_creates_no_real_lights() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", LIGHTS_OFF)
    _instance = _register_lamp(40.0, 5)
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    await wait_seconds(1.0)
    var statistics: Dictionary = MaszynaE3DRenderingServer.light_get_statistics()
    assert_eq(statistics["spot"] + statistics["omni"], 0, "only the lit submodels, no real lights")


func test_high_quality_keeps_every_arm() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", HIGH_QUALITY)
    _instance = _register_lamp(40.0, 5)
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"] + statistics["omni"], 5, "one light per arm")


func test_a_lamp_without_a_spotlight_is_lit_from_its_halo_over_its_patch() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    # elektryczne/latarnie_cnodax/slup_betonowy_6m_wysiegnik_k_oprawa-orz7: a halo and a patch,
    # named after its fixture
    _instance = _register_model("models/test/halo_lamp_patch")
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"], 1, "the halo became a spot light over the patch")
    assert_eq(statistics["lit"], 1)


func test_a_lamp_fixture_without_its_patch_is_lit_from_its_halo() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    # elektryczne/opr_sgp340: the halo alone, at the fixture's own origin; its patch is a model of
    # its own
    _instance = _register_model("models/test/halo_lamp_fixture")
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"], 1, "the fixture shines down from its halo")
    assert_eq(statistics["lit"], 1)


func test_economy_mode_lights_the_fixtures_of_one_pole_once() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", ECONOMY)
    var pole: Array[RID] = _register_pole()

    var statistics: Dictionary = await _await_lights()
    assert_eq(statistics["spot"] + statistics["omni"], 1, "two fixtures on a pole are one light")
    _free_all(pole)


func test_high_quality_lights_every_fixture_of_a_pole() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    ProjectSettings.set_setting("maszyna/scenery/lights/mode", HIGH_QUALITY)
    var pole: Array[RID] = _register_pole()

    await _await_lights()
    await wait_seconds(0.5)
    var statistics: Dictionary = MaszynaE3DRenderingServer.light_get_statistics()
    assert_eq(statistics["spot"] + statistics["omni"], 2, "one light a fixture")
    _free_all(pole)


func test_a_model_with_a_light_and_no_halo_gets_no_real_light() -> void:
    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    # a house with lit windows under light_on00
    _instance = _register_model("models/test/halo_lamp_none")
    MaszynaE3DRenderingServer.instance_set_lights_modes(_instance, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])

    await wait_seconds(1.0)
    var statistics: Dictionary = MaszynaE3DRenderingServer.light_get_statistics()
    assert_eq(statistics["spot"] + statistics["omni"], 0, "only its lit submodels")


func _register_lamp(light_angle: float = 40.0, arms: int = 1) -> RID:
    return _register_model("models/test/test_lamp_%d_%d" % [int(light_angle), arms])


## Two lamp fixtures on one pole, as Bałtyk places elektryczne/opr_sgp340: 9.35 m up, 0.93 m apart
func _register_pole() -> Array[RID]:
    var pole: Array[RID] = []
    for side: float in [-0.465, 0.465]:
        var fixture: RID = MaszynaE3DRenderingServer.instance_register(
            "models/test/halo_lamp_fixture", [], Transform3D(Basis(), Vector3(0.0, 9.35, side)), 0.0, 100.0,
            _camera.get_world_3d().scenario,
        )
        MaszynaE3DRenderingServer.instance_set_lights_modes(fixture, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])
        pole.append(fixture)
    return pole


func _free_all(registered: Array[RID]) -> void:
    for instance: RID in registered:
        MaszynaE3DRenderingServer.instance_free(instance)


func _register_model(model_filename: String) -> RID:
    return MaszynaE3DRenderingServer.instance_register(
        model_filename, [], Transform3D(), 0.0, 100.0, _camera.get_world_3d().scenario,
    )


## Waits for the streaming server to build the registered placement
func _await_lights() -> Dictionary:
    var waited: float = 0.0
    while waited < STREAM_TIMEOUT:
        var statistics: Dictionary = MaszynaE3DRenderingServer.light_get_statistics()
        if statistics["total"] > 0:
            return statistics
        await wait_seconds(0.1)
        waited += 0.1
    return MaszynaE3DRenderingServer.light_get_statistics()


## Stands in for MaszynaE3DModelManager: a lamp with one light_on00 holding a FREE_SPOTLIGHT, or
## one without (_load_halo_lamp())
func _load_model(_data_path: String, model_filename: String) -> MaszynaE3DModel:
    if model_filename.get_file().begins_with("halo_lamp_"):
        return _load_halo_lamp(model_filename.get_file().trim_prefix("halo_lamp_"))
    var angle: float = float(model_filename.get_slice("_", 2))
    var arms: int = int(model_filename.get_slice("_", 3))
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var light_on: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    light_on.resource_name = "light_on00"
    light_on.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    var arm_submodels: Array[MaszynaE3DSubModel] = []
    for i in range(maxi(arms, 1)):
        var spotlight: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
        spotlight.resource_name = "fspot%02d" % i
        spotlight.submodel_type = MaszynaE3DSubModel.SUBMODEL_FREE_SPOTLIGHT
        spotlight.light_angle = angle
        spotlight.light_range = 40.0
        spotlight.transform = Transform3D(Basis(), Vector3(float(i) - 2.0, 8.0, 0.0))
        arm_submodels.append(spotlight)
    light_on.submodels = arm_submodels
    model.submodels = [light_on]
    var definition: MaszynaE3DModelLightDefinition = MaszynaE3DModelLightDefinition.new()
    definition.on_submodel_path = NodePath("light_on00")
    model.register_light("00", definition)
    return model


## A light_on00 with no FREE_SPOTLIGHT: with `kind` "patch" the halo of a head 7 m up and the lit
## patch on the ground, "fixture" the halo alone at the model's origin, "none" a lit window
func _load_halo_lamp(kind: String) -> MaszynaE3DModel:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var light_on: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    light_on.resource_name = "light_on00"
    light_on.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    var lit: Array[MaszynaE3DSubModel] = []
    var halo: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    halo.resource_name = "plane02"
    halo.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    halo.material_name = "elektryczne/poswiata" if not kind == "none" else "okno_on"
    halo.transform = Transform3D(Basis(), Vector3(0.0, 7.0 if kind == "patch" else 0.0, 0.0))
    lit.append(halo)
    if kind == "patch":
        var patch: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
        patch.resource_name = "plane01"
        patch.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
        patch.material_name = "elektryczne/light1"
        var quad: BoxMesh = BoxMesh.new()
        quad.size = Vector3(15.0, 0.01, 9.0)
        patch.mesh = quad
        lit.append(patch)
    light_on.submodels = lit
    model.submodels = [light_on]
    var definition: MaszynaE3DModelLightDefinition = MaszynaE3DModelLightDefinition.new()
    definition.on_submodel_path = NodePath("light_on00")
    model.register_light("00", definition)
    return model
