extends MaszynaGutTest

const TEST_GAME_DIR: String = "user://gut/nodebank_library_builder"

var builder: Node
var library_roots: Array[Node] = []
var _previous_game_dir: String = ""
var _temp_nodebank_path: String


func before_each() -> void:
    builder = load("res://addons/libmaszyna/editor/nodebank/nodebank_library_builder.gd").new()
    add_child_autoqfree(builder)
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    DirAccess.make_dir_recursive_absolute(TEST_GAME_DIR)
    MaszynaGameDirectory.save_game_dir(TEST_GAME_DIR)
    _temp_nodebank_path = TEST_GAME_DIR.path_join("nodebank.txt")


func after_each() -> void:
    for root: Node in library_roots:
        if is_instance_valid(root):
            root.free()
    library_roots.clear()
    if FileAccess.file_exists(_temp_nodebank_path):
        DirAccess.remove_absolute(_temp_nodebank_path)
    DirAccess.remove_absolute(TEST_GAME_DIR)
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_builds_grouped_e3d_instances() -> void:
    var root: Node = _build_library("""
 Domki
[
node 2000 0 none model 0.0 0.0 0.0 0.0 mieszkalne\\dom.t3d mieszkalne/dom_skin endmodel
node 2000 0 none model 0.0 0.0 0.0 0.0 root_model.t3d none endmodel
]
""")

    var group: Node = root.get_node("Domki")
    assert_eq(group.get_child_count(), 2, "Group should contain parsed E3D items")

    var first: MaszynaE3DModelInstance = group.get_child(0) as MaszynaE3DModelInstance
    assert_eq(first.data_path, "models/mieszkalne", "Data path should point to the Maszyna models directory")
    assert_eq(first.model_filename, "dom", "Model filename should not contain .t3d")
    assert_eq(first.skins, ["mieszkalne/dom_skin"], "First skin should be configured")

    var second: MaszynaE3DModelInstance = group.get_child(1) as MaszynaE3DModelInstance
    assert_eq(second.data_path, "models/", "Root-level models should use the Maszyna models directory")
    assert_eq(second.model_filename, "root_model", "Root-level model filename should be parsed")
    assert_eq(second.skins, [], "none skin should produce empty skins")


func test_merges_duplicate_group_labels() -> void:
    var root: Node = _build_library("""
 Krzaki
[
node 2000 0 none model 0.0 0.0 0.0 0.0 flora\\a.t3d none endmodel
]
 Krzaki
[
node 2000 0 none model 0.0 0.0 0.0 0.0 flora\\b.t3d none endmodel
]
""")

    assert_eq(root.get_child_count(), 1, "Duplicate groups should be merged")
    assert_eq(root.get_node("Krzaki").get_child_count(), 2, "Merged group should contain both items")


func test_ignores_extra_tokens_after_skin() -> void:
    var root: Node = _build_library("""
 Kamienice
[
node 2000 0 none model 0.0 0.0 0.0 0.0 mieszkalne/tkamienica.t3d mieszkalne/skin lights 4 endmodel
]
""")

    var item: MaszynaE3DModelInstance = root.get_node("Kamienice").get_child(0) as MaszynaE3DModelInstance
    assert_eq(item.skins, ["mieszkalne/skin"], "Only the first skin token should be used")


func test_skips_malformed_records() -> void:
    var root: Node = _build_library("""
 Obiekty
[
node 2000 0 none model 0.0 0.0 0.0 0.0 good.t3d none endmodel
node 2000 0 none model 0.0 0.0 0.0 0.0 bad.t3d none endmode
]
""")

    var group: Node = root.get_node("Obiekty")
    assert_eq((group.get_child(0) as MaszynaE3DModelInstance).model_filename, "good", "Valid records should remain")
    assert_eq((group.get_child(1) as MaszynaE3DModelInstance).model_filename, "bad", "endmode is a workaround for broken nodebank.txt: assume endmodel")


func _build_library(text: String) -> Node:
    var f := FileAccess.open(_temp_nodebank_path, FileAccess.WRITE)
    f.store_string(text)
    f.close()

    var scene: PackedScene = builder.call("load_library", true) as PackedScene
    var root: Node = scene.instantiate()
    library_roots.append(root)
    return root
