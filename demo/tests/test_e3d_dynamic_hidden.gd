extends MaszynaGutTest

## A shunting signal dwarf (sem/karzelki/ktmnb.e3d) keeps its lamps under a transform named "_on",
## and the lens of each lamp under a light_on00 transform of the same name. The original hides a
## "_on" submodel by default only in a dynamic (vehicle) model (Model3d.cpp:275, 2221), and
## switches only the first light_on00 it finds, whose children show with it (AnimModel.cpp:306).
## Hidden for every model, the dwarf's lamps never lit in the scenery.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const DATA_PATH:String = "models/sem/karzelki"
const MODEL:String = "ktmnb"
const LAMP_PATH:NodePath = NodePath("karzel2/_on/light_on00")

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func _instance(kind:E3DRenderingServer.InstanceKind, lights:Dictionary[String, bool]) -> E3DModelInstance:
    var instance:E3DModelInstance = E3DModelInstance.new()
    instance.instance_kind = kind
    instance.data_path = DATA_PATH
    instance.model = E3DModelManager.load_model(DATA_PATH, MODEL)
    instance.lights_state = lights
    add_child_autoqfree(instance)
    return instance


func _lens(lamp:Node3D) -> MeshInstance3D:
    # the backend adds the submodel nodes as internal children
    for child:Node in lamp.get_children(true):
        if child is MeshInstance3D:
            return child
    return null


func test_scenery_dwarf_lamp_lights_with_its_lens() -> void:
    var lights:Dictionary[String, bool] = {"00": true}
    var instance:E3DModelInstance = _instance(E3DRenderingServer.INSTANCE_KIND_STATIC, lights)
    var lamp:Node3D = instance.get_node(LAMP_PATH)
    var lens:MeshInstance3D = _lens(lamp)
    assert_not_null(lens, "the lamp should carry its lens")
    if not lens:
        return
    assert_true(lens.is_visible_in_tree(), "a lit lamp of a scenery model should show its lens")


func test_vehicle_model_hides_its_on_submodels() -> void:
    var lights:Dictionary[String, bool] = {}
    var instance:E3DModelInstance = _instance(E3DRenderingServer.INSTANCE_KIND_DYNAMIC, lights)
    var on_node:Node3D = instance.get_node(NodePath("karzel2/_on"))
    assert_false(on_node.visible, "a dynamic model should hide a \"_on\" submodel by default")


func test_a_client_can_show_a_dynamic_hidden_submodel() -> void:
    var lights:Dictionary[String, bool] = {}
    var instance:E3DModelInstance = _instance(E3DRenderingServer.INSTANCE_KIND_DYNAMIC, lights)
    var model:RID = instance.get_e3d_instance()

    E3DRenderingServer.instance_set_submodel_visible(model, "_on", true)
    assert_true((instance.get_node(NodePath("karzel2/_on")) as Node3D).visible,
            "an explicit visibility setting should override the vehicle default")

    E3DRenderingServer.instance_set_instancer(model, E3DRenderingServer.INSTANCER_OPTIMIZED)
    E3DRenderingServer.instance_set_instancer(model, E3DRenderingServer.INSTANCER_NODES)
    assert_true((instance.get_node(NodePath("karzel2/_on")) as Node3D).visible,
            "the explicit setting should survive an instancer rebuild")

    E3DRenderingServer.instance_set_submodel_visible(model, "_on", false)
    assert_false((instance.get_node(NodePath("karzel2/_on")) as Node3D).visible,
            "an explicit hide should still win")
