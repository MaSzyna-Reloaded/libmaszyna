extends MaszynaGutTest

## A `pyscreen:` of an MMD whose script has a `.lua` file beside it is drawn by Lua
## (MaszynaScreenLuaNode) on the screen's submodel, with the screen's parameters and interval.

const MMD:String = "res://tests/fixtures/dynamic/test/animated_v1/lua_screen.mmd"
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## As the game hands it, from the game directory with a leading slash (MaszynaMaterialManager drops the
## first segment of it)
const DATA_PATH:String = "/dynamic/test/animated_v1"
const TARGET:String = "ekran"
## The size of the texture the screen replaces (screen.dds)
const SCREEN_TEXTURE_SIZE:Vector2i = Vector2i(4, 4)


func test_a_screen_with_a_lua_script_is_drawn_by_lua() -> void:
    var previous_game_dir:String = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(ProjectSettings.globalize_path(FIXTURES_GAME_DIR))
    var definition:MmdCabinDefinition = MmdCabinInstancer.parse(
            ProjectSettings.globalize_path(MMD), MmdCabinInstancer.vehicle_parameters("cabs", "lua_screen", ""), 1, {})
    var root:Node3D = add_child_autofree(Node3D.new())
    var diagnostics:Array[Dictionary] = []
    watch_signals(ScenarioScriptServer)
    MmdCabinInstancer.build_into(root, definition, RID(), DATA_PATH, "", diagnostics)
    await wait_idle_frames(2)

    var screens:Array[Node] = root.find_children("*", "MaszynaScreenLuaNode", true, false)
    assert_eq(screens.size(), 1, "drawn by Lua: %s" % [diagnostics])
    assert_eq(root.find_children("*", "CabinPythonScreen", true, false).size(), 0, "not by Python")
    var mesh:MeshInstance3D = root.find_child(TARGET, true, false)
    assert_eq(MaszynaMaterialManager.material_get_texture(mesh.material_override), (screens[0] as MaszynaScreenLuaNode).texture,
            "the submodel shows the script's drawing")
    assert_eq((screens[0] as MaszynaScreenLuaNode).texture.texture_size, SCREEN_TEXTURE_SIZE,
            "drawn at the size of the texture it replaces")
    assert_signal_not_emitted(ScenarioScriptServer, "script_error", "the script got what it asserts")
    MaszynaGameDirectory.save_game_dir(previous_game_dir)
