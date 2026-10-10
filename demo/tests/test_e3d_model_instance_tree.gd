extends MaszynaGutTest

## FINDINGS.md 2026-09-30: "Edit FIZ" re-adds a vehicle to the tree, and its model frees and
## rebuilds its submodels on leaving and entering it. The model said so only on reload(), so a
## vehicle kept the freed bogie nodes and the editor aborted on the next placement.

const BOGIE:String = "bogie"

var _events:Array[String] = []


func test_a_model_leaving_and_entering_the_tree_announces_its_submodels() -> void:
    var instance:E3DModelInstance = E3DModelInstance.new()
    var model:E3DModel = E3DModel.new()
    var bogie:E3DSubModel = E3DSubModel.new()
    bogie.resource_name = BOGIE
    bogie.submodel_type = E3DSubModel.SUBMODEL_TRANSFORM
    var submodels:Array[E3DSubModel] = [bogie]
    model.submodels = submodels
    instance.model = model
    var holder:Node3D = Node3D.new()
    add_child_autoqfree(holder)
    holder.add_child(instance)
    instance.e3d_loading.connect(_on_loading)
    instance.e3d_loaded.connect(_on_loaded)
    var first:Node = instance.get_node(NodePath(BOGIE))

    holder.remove_child(instance)
    assert_eq(_events, ["loading"] as Array[String], "leaving the tree frees the submodels, and says so first")
    await wait_idle_frames(1)
    assert_false(is_instance_valid(first), "the submodel is gone")

    holder.add_child(instance)
    assert_eq(_events, ["loading", "loaded"] as Array[String], "entering it builds them again, and says so")
    assert_true(instance.is_e3d_loaded())
    assert_not_null(instance.get_node_or_null(NodePath(BOGIE)))


func _on_loading() -> void:
    _events.append("loading")


func _on_loaded() -> void:
    _events.append("loaded")
