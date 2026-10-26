extends MaszynaGutTest

## demo_3d.tscn's environment is the core's ProceduralSkyEnvironment, following its EnvironmentNode:
## the render settings of the panel under Esc reach it as they reach the game's sky (libmaszyna#330:
## a plain WorldEnvironment took none of them). The scene is read, not instanced: its vehicles' data
## is the game directory's.

const DEMO_3D_SCENE:String = "res://demo_3d.tscn"
const SKY_SCRIPT:GDScript = preload("res://addons/libreloaded/environment/procedural_sky_environment.gd")


func test_the_environment_of_demo_3d_takes_the_render_settings() -> void:
    var state:SceneState = (load(DEMO_3D_SCENE) as PackedScene).get_state()
    var sky_script:Variant = null
    var world_environments:int = 0
    for node:int in state.get_node_count():
        if state.get_node_type(node) == &"WorldEnvironment":
            world_environments += 1
        if not state.get_node_name(node) == &"Sky":
            continue
        for property:int in state.get_node_property_count(node):
            if state.get_node_property_name(node, property) == &"script":
                sky_script = state.get_node_property_value(node, property)
    assert_eq(sky_script, SKY_SCRIPT, "the scene's Sky is the core's procedural sky")
    assert_eq(world_environments, 0, "and no environment of its own beside it")
    var followed:bool = false
    for connection:int in state.get_connection_count():
        if state.get_connection_signal(connection) == &"configuration_changed" \
                and state.get_connection_method(connection) == &"apply_configuration":
            followed = true
    assert_true(followed, "the Sky follows its EnvironmentNode")
