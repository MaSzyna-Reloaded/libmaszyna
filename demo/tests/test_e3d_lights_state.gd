extends MaszynaGutTest

const MATERIALS_GAME_DIR = "res://tests/materials"

var _previous_game_dir: String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(MATERIALS_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_model_instance_synchronizes_lights_state_with_model() -> void:
    var instance: E3DModelInstance = E3DModelInstance.new()
    instance.instancer = E3DModelInstance.Instancer.OPTIMIZED
    instance.model = _create_model_with_lights(["front", "rear"])
    add_child_autoqfree(instance)

    instance.lights_state = {
        "front": true,
        "stale": true,
    }

    assert_eq(instance.lights_state.size(), 2, "Only lights declared by the model should remain in state")
    assert_true(instance.lights_state["front"], "Existing state value should be preserved")
    assert_false(instance.lights_state["rear"], "Missing light should be initialized as disabled")


func test_nodes_instancer_maps_light_on_prefix_family() -> void:
    var target_node: E3DModelInstance = E3DModelInstance.new()
    var model: E3DModel = E3DModel.new()
    var light_on: E3DSubModel = _create_transform_submodel("light_on00", false)
    var light_off: E3DSubModel = _create_transform_submodel("light_off00", true)
    var spotlight: E3DSubModel = _create_spotlight_submodel("spotlight", false)
    var light_definition: E3DModelLightDefinition = E3DModelLightDefinition.new()

    light_on.submodels = [spotlight]
    model.submodels = [light_on, light_off]
    light_definition.on_submodel_path = NodePath("light_on00")
    light_definition.off_submodel_path = NodePath("light_off00")
    model.register_light("00", light_definition)

    target_node.model = model
    target_node.lights_state = {"00": false}
    add_child_autoqfree(target_node)

    var light_on_node: Node3D = target_node.get_node(NodePath("light_on00"))
    var light_off_node: Node3D = target_node.get_node(NodePath("light_off00"))
    var spotlight_node: SpotLight3D = target_node.get_node(NodePath("light_on00/spotlight"))

    assert_false(light_on_node.visible, "On node should start hidden when light is disabled")
    assert_true(light_off_node.visible, "Off node should start visible when light is disabled")
    assert_false(spotlight_node.visible, "Spotlight should start hidden when light is disabled")

    target_node.lights_state = {"00": true}

    assert_true(light_on_node.visible, "On node should become visible when light is enabled")
    assert_false(light_off_node.visible, "Off node should become hidden when light is enabled")
    assert_true(spotlight_node.visible, "Spotlight should become visible when light is enabled")


## Regression: a moving model re-applied its whole lights state once per frame, which wiped the
## visibility of every light submodel another owner had set - the cabin's own MMD indicators show
## and hide exactly those submodels, so the instrument backlight and the cab lamp blinked on for
## one frame in ten and were dark in between (see FINDINGS.md, 2026-09-23).
func test_moving_a_model_does_not_reapply_its_lights_state() -> void:
    var target_node: E3DModelInstance = E3DModelInstance.new()
    var model: E3DModel = E3DModel.new()
    var light_on: E3DSubModel = _create_transform_submodel("light_on00", false)
    var light_off: E3DSubModel = _create_transform_submodel("light_off00", true)
    var light_definition: E3DModelLightDefinition = E3DModelLightDefinition.new()

    model.submodels = [light_on, light_off]
    light_definition.on_submodel_path = NodePath("light_on00")
    light_definition.off_submodel_path = NodePath("light_off00")
    model.register_light("00", light_definition)

    target_node.model = model
    target_node.lights_state = {"00": true}
    add_child_autoqfree(target_node)

    # what the cabin's indicator widget does with the very same submodels
    var light_on_node: Node3D = target_node.get_node(NodePath("light_on00"))
    light_on_node.visible = false

    target_node.position = Vector3(0.0, 0.0, 1.0)
    await wait_idle_frames(2)

    assert_false(light_on_node.visible, "moving the model must leave its light submodels alone")


## Regression: the E3D submodel material override (now MaterialManager.get_submodel_material())
## never populated MaterialOptions.diffuse_color from the real parsed submodel (left as a TODO, silently leaving
## every textured/named-material submodel's albedo at the shader's default white) - unlike
## test_material_manager_variants.gd's MaszynaMaterialFactory-level coverage, which only ever built
## MaterialOptions by hand and so never exercised this real instancing call path, this test goes
## through the actual material resolver entry point a real model load uses. Confirmed real: EP07's
## "wylszybki_on"/"_off" main-breaker indicator lamp submodels, diffuse (0, 0.749, 0) over a
## textured material, rendered white in-game despite the model file's own diffuse being green.
func test_get_material_override_uses_real_submodel_diffuse_color() -> void:
    var submodel: E3DSubModel = E3DSubModel.new()
    submodel.resource_name = "wylszybki_on"
    submodel.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    submodel.material_name = "nontransparent_manager"
    submodel.diffuse_color = Color(0.0, 0.749, 0.0, 1.0)

    var material: ShaderMaterial = MaterialManager.get_submodel_material(
        submodel, "test", [], E3DRenderingServer.TRANSLUCENCY_CUTOUT,
        MaterialManager.MAX_TEXTURE_SIZE_PROJECT_DEFAULT
    ) as ShaderMaterial

    assert_not_null(material, "a submodel with material_name should get a material override")
    if material:
        assert_eq(material.get_shader_parameter("albedo"), submodel.diffuse_color)


func _create_model_with_lights(light_names: Array[String]) -> E3DModel:
    var model: E3DModel = E3DModel.new()

    for light_name: String in light_names:
        model.register_light(light_name, E3DModelLightDefinition.new())

    return model


func _create_transform_submodel(name: String, visible: bool) -> E3DSubModel:
    var submodel: E3DSubModel = E3DSubModel.new()
    submodel.resource_name = name
    submodel.submodel_type = E3DSubModel.SUBMODEL_TRANSFORM
    submodel.visible = visible
    return submodel


func _create_spotlight_submodel(name: String, visible: bool) -> E3DSubModel:
    var submodel: E3DSubModel = E3DSubModel.new()
    submodel.resource_name = name
    submodel.submodel_type = E3DSubModel.SUBMODEL_FREE_SPOTLIGHT
    submodel.visible = visible
    return submodel
