extends MaszynaGutTest

## PythonScreenServer runs a cab screen script, in a Python host process, and turns what it draws
## into a texture. The pixels themselves cannot be read back headless (the dummy renderer keeps no
## texture data), so the fixtures send the state they received and the major version of the Python
## they run on back as commands.
## The tests of one runtime run on each, 2.7 and 3.x (RUNTIMES). A CPython 2.7 runtime is read from
## MASZYNA_PYTHON_HOME, its prefix (the directory holding lib/libpython2.7.so.1.0), a 3.x one from
## MASZYNA_PYTHON3_HOME (holding lib/libpython3.NN.so.1.0) - make python-runtime-linux builds both; a test
## whose runtime is not set is skipped. Each test has a server and a game directory of its own: a
## 3.x runtime is found in the game directory, which links it in as the game data has it, and a game
## directory without one runs every script on 2.7.

const SCRIPT:String = "res://tests/fixtures/python/fixture_screen"
const ABORT_SCRIPT:String = "res://tests/fixtures/python/fixture_abort_screen"
const COUNTER_SCRIPT:String = "res://tests/fixtures/python/fixture_counter_screen"
const PYTHON2_SCRIPT:String = "res://tests/fixtures/python/fixture_python2_screen"
const PYTHON2_IMPORT_SCRIPT:String = "res://tests/fixtures/python/fixture_python2_import_screen"
const PYTHON2_LOAD_SCRIPT:String = "res://tests/fixtures/python/fixture_python2_load_screen"
const PYTHON2_DRAW_SCRIPT:String = "res://tests/fixtures/python/fixture_python2_draw_screen"
const BASE_SCRIPT:String = "res://tests/fixtures/python/fixture_base_screen"
## The modules the fixtures import from their game directory
const FIXTURE_MODULES:String = "res://tests/fixtures/fixture_modules"
## The game directories the tests make, one a runtime
const GAME_DIRS:String = "user://python_screen_server"
## Where maszyna-python-host stands in this project, beside the library: the game's own runtimes are
## found there (Linux)
const HOST_DIR:String = "res://bin/libreloaded/linux"
const HOME_SETTING:String = "maszyna/python/home"
## The runtimes the tests of one runtime run on
const RUNTIMES:Array[String] = ["2.7", "3"]
## By runtime: the major version a fixture sends back
const MAJOR_VERSIONS:Dictionary[String, String] = {"2.7": "2", "3": "3"}
const RENDER_TIMEOUT_SEC:float = 10.0
## Long enough for a render that should not happen to have happened
const QUIET_WAIT_SEC:float = 0.5

var _previous_home:String = ""
var _previous_game_dir:String = ""
var _servers:Array[Object] = []
var _commands:PackedStringArray = PackedStringArray()
var _rendered:Array[RID] = []
var _failures:PackedStringArray = PackedStringArray()
## A runtime linked in beside the host by a test, removed after it
var _host_runtime:String = ""


func before_each() -> void:
    _previous_home = ProjectSettings.get_setting(HOME_SETTING, "")
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    _commands = PackedStringArray()
    _rendered = []
    _failures = PackedStringArray()


func after_each() -> void:
    for server:Object in _servers:
        server.free()
    _servers = []
    if _host_runtime:
        DirAccess.remove_absolute(_host_runtime)
        _host_runtime = ""
    ProjectSettings.set_setting(HOME_SETTING, _previous_home)
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_script_draws_the_state_into_the_texture_and_sends_commands(runtime:String = use_parameters(RUNTIMES)) -> void:
    var server:Object = _create_server_on(runtime)
    if not server:
        return
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(SCRIPT), _on_commands_received)

    var touches:Array = [Vector2(0.25, 0.5)]
    var state:Dictionary = {"level": 1.0, "step": 128, "enabled": true, "name": "ep07-424", "touches": touches}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _commands.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_commands, PackedStringArray([
        "RGBA", ProjectSettings.globalize_path(SCRIPT).get_base_dir() + "/",
        "1.0;128;True;ep07-424;[(0.25, 0.5)]", MAJOR_VERSIONS[runtime]]))
    var texture:Texture2D = server.screen_get_texture(screen)
    assert_eq(texture.get_size(), Vector2(2, 1))
    # the fixture draws a white and a 128 grey pixel - what the screen throws around is their average
    assert_eq(_rendered, [screen])
    var average:Color = server.screen_get_average_color(screen)
    assert_almost_eq(average.r, (1.0 + 128.0 / 255.0) * 0.5, 0.01)
    assert_almost_eq(average.a, 1.0, 0.01)
    assert_eq(_failures, PackedStringArray())


func test_a_script_that_aborts_the_interpreter_does_not_end_the_game(runtime:String = use_parameters(RUNTIMES)) -> void:
    var server:Object = _create_server_on(runtime)
    if not server:
        return
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(ABORT_SCRIPT), _on_commands_received)
    var state:Dictionary = {"level": 1.0}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _failures.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_failures.size(), 1)
    assert_string_contains(_failures[0], "exit code")
    assert_push_error("Python screens stay blank")


func test_each_screen_has_an_instance_of_its_script_of_its_own(runtime:String = use_parameters(RUNTIMES)) -> void:
    # the original shares one a script (python_taskqueue::fetch_renderer()) for as long as it runs,
    # one scenery: ksiazka_wozu.py kept the vehicle of the scenery before (#346)
    var server:Object = _create_server_on(runtime)
    if not server:
        return
    var counts:Dictionary[String, String] = {}
    var first:RID = server.screen_create(ProjectSettings.globalize_path(COUNTER_SCRIPT),
            func(commands:PackedStringArray) -> void: counts["first"] = commands[0])
    var state:Dictionary = {}
    server.screen_request_render(first, state)
    await wait_until(func() -> bool: return counts.get("first") == "1", RENDER_TIMEOUT_SEC)
    server.screen_request_render(first, state)
    await wait_until(func() -> bool: return counts.get("first") == "2", RENDER_TIMEOUT_SEC)
    server.screen_free(first)
    var second:RID = server.screen_create(ProjectSettings.globalize_path(COUNTER_SCRIPT),
            func(commands:PackedStringArray) -> void: counts["second"] = commands[0])
    server.screen_request_render(second, state)
    await wait_until(func() -> bool: return counts.has("second"), RENDER_TIMEOUT_SEC)

    assert_eq(counts["first"], "2")
    assert_eq(counts["second"], "1", "the second screen draws with an instance of its own")


func test_the_base_class_is_the_servers_own_for_the_runtime(runtime:String = use_parameters(RUNTIMES)) -> void:
    var server:Object = _create_server_on(runtime)
    if not server:
        return
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(BASE_SCRIPT), _on_commands_received)
    var state:Dictionary = {}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _commands.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_commands, PackedStringArray([MAJOR_VERSIONS[runtime], "True"]))


func test_a_missing_runtime_is_reported_once_and_the_screens_stay_blank() -> void:
    ProjectSettings.set_setting(HOME_SETTING, "missing-python-home")
    MaszynaGameDirectory.save_game_dir(_make_game_dir("2.7", ""))
    var server:Object = _create_server()
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(SCRIPT), _on_commands_received)
    var state:Dictionary = {"level": 1.0}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _failures.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_failures.size(), 1)
    assert_string_contains(_failures[0], "missing-python-home")
    assert_push_error("Python screens stay blank")
    server.screen_request_render(screen, state)
    await wait_seconds(QUIET_WAIT_SEC)
    assert_eq(_failures.size(), 1)
    assert_eq(_commands, PackedStringArray())


func test_a_script_runs_on_python_3_when_it_compiles_and_loads_there_and_on_2_7_beside_it_otherwise() -> void:
    var home:String = OS.get_environment("MASZYNA_PYTHON_HOME")
    var home3:String = OS.get_environment("MASZYNA_PYTHON3_HOME")
    if not home or not home3:
        pending("MASZYNA_PYTHON_HOME or MASZYNA_PYTHON3_HOME is not set")
        return
    ProjectSettings.set_setting(HOME_SETTING, home)
    MaszynaGameDirectory.save_game_dir(_make_game_dir("3", home3))
    var server:Object = _create_server()
    var versions:Dictionary[String, PackedStringArray] = {}
    var screens:Array[RID] = []
    for script:String in [SCRIPT, PYTHON2_SCRIPT, PYTHON2_IMPORT_SCRIPT, PYTHON2_LOAD_SCRIPT, BASE_SCRIPT]:
        screens.append(server.screen_create(ProjectSettings.globalize_path(script),
                func(commands:PackedStringArray) -> void: versions[script] = commands))
    var state:Dictionary = {"level": 1.0, "step": 128, "enabled": true, "name": "ep07-424", "touches": []}
    for screen:RID in screens:
        server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return versions.size() == screens.size(), RENDER_TIMEOUT_SEC)

    assert_eq(versions[SCRIPT][-1], "3", "it compiles on 3.x")
    assert_eq(versions[PYTHON2_SCRIPT], PackedStringArray(["2"]), "written for Python 2")
    assert_eq(versions[PYTHON2_IMPORT_SCRIPT], PackedStringArray(["2"]), "it imports a module written for Python 2")
    assert_eq(versions[PYTHON2_LOAD_SCRIPT], PackedStringArray(["2"]), "it compiles on 3.x, but does not load there")
    assert_eq(versions[BASE_SCRIPT], PackedStringArray(["3", "True"]), "the base class of 3.x")
    assert_eq(_failures, PackedStringArray())


func test_a_script_that_fails_on_python_3_while_it_draws_runs_on_2_7_from_then_on() -> void:
    var home:String = OS.get_environment("MASZYNA_PYTHON_HOME")
    var home3:String = OS.get_environment("MASZYNA_PYTHON3_HOME")
    if not home or not home3:
        pending("MASZYNA_PYTHON_HOME or MASZYNA_PYTHON3_HOME is not set")
        return
    ProjectSettings.set_setting(HOME_SETTING, home)
    MaszynaGameDirectory.save_game_dir(_make_game_dir("3", home3))
    var server:Object = _create_server()
    var versions:Array[String] = []
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(PYTHON2_DRAW_SCRIPT),
            func(commands:PackedStringArray) -> void: versions.append(commands[0]))
    var state:Dictionary = {}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return versions.size() == 1, RENDER_TIMEOUT_SEC)
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return versions.size() == 2, RENDER_TIMEOUT_SEC)

    assert_eq(versions, ["2", "2"] as Array[String], "the first frame and the next are drawn on 2.7")
    assert_eq(_rendered, [screen, screen])
    assert_eq(_failures, PackedStringArray())


func test_a_python_2_script_without_a_2_7_runtime_is_reported_once_and_the_rest_draws() -> void:
    var home3:String = OS.get_environment("MASZYNA_PYTHON3_HOME")
    if not home3:
        pending("MASZYNA_PYTHON3_HOME is not set")
        return
    ProjectSettings.set_setting(HOME_SETTING, "missing-python-home")
    MaszynaGameDirectory.save_game_dir(_make_game_dir("3", home3))
    var server:Object = _create_server()
    var python2_screen:RID = server.screen_create(ProjectSettings.globalize_path(PYTHON2_SCRIPT), _on_commands_received)
    var python3_commands:Array[PackedStringArray] = []
    var python3_screen:RID = server.screen_create(ProjectSettings.globalize_path(SCRIPT),
            func(commands:PackedStringArray) -> void: python3_commands.append(commands))
    var state:Dictionary = {"level": 1.0, "step": 128, "enabled": true, "name": "ep07-424", "touches": []}
    server.screen_request_render(python2_screen, state)
    server.screen_request_render(python3_screen, state)
    await wait_until(func() -> bool: return _failures.size() > 0 and python3_commands.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_failures.size(), 1)
    assert_string_contains(_failures[0], "missing-python-home")
    assert_push_error("Python screens stay blank")
    assert_eq(_commands, PackedStringArray())
    assert_eq(python3_commands[0][-1], "3")


func test_a_data_reload_finds_the_runtimes_of_the_game_directory_set_by_then() -> void:
    var home:String = OS.get_environment("MASZYNA_PYTHON_HOME")
    var home3:String = OS.get_environment("MASZYNA_PYTHON3_HOME")
    if not home or not home3:
        pending("MASZYNA_PYTHON_HOME or MASZYNA_PYTHON3_HOME is not set")
        return
    ProjectSettings.set_setting(HOME_SETTING, home)
    MaszynaGameDirectory.save_game_dir(_make_game_dir("2.7", ""))
    var server:Object = _create_server()
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(SCRIPT), _on_commands_received)
    var state:Dictionary = {"level": 1.0, "step": 128, "enabled": true, "name": "ep07-424", "touches": []}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _commands.size() > 0, RENDER_TIMEOUT_SEC)
    assert_eq(_commands[-1], "2", "no 3.x runtime in the first game directory")

    _commands = PackedStringArray()
    MaszynaGameDirectory.save_game_dir(_make_game_dir("3", home3))
    GameDataServer.data_reload()
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _commands.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_commands[-1], "3", "the second game directory has a 3.x runtime")
    assert_eq(_failures, PackedStringArray())


func test_the_runtime_beside_the_host_comes_before_the_game_directorys() -> void:
    var home3:String = OS.get_environment("MASZYNA_PYTHON3_HOME")
    if not home3 or not OS.get_name() == "Linux":
        pending("MASZYNA_PYTHON3_HOME is not set, or not on Linux")
        return
    # the game directory has no 3.x runtime and no 2.7 one: only the one beside the host can draw
    ProjectSettings.set_setting(HOME_SETTING, "missing-python-home")
    MaszynaGameDirectory.save_game_dir(_make_game_dir("2.7", ""))
    _host_runtime = ProjectSettings.globalize_path(HOST_DIR).path_join(home3.simplify_path().get_file())
    assert_false(DirAccess.dir_exists_absolute(_host_runtime), "no runtime of a build beside the host")
    DirAccess.open(ProjectSettings.globalize_path(HOST_DIR)).create_link(home3, _host_runtime)
    var server:Object = _create_server()
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(SCRIPT), _on_commands_received)
    var state:Dictionary = {"level": 1.0, "step": 128, "enabled": true, "name": "ep07-424", "touches": []}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _commands.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_commands[-1], "3")
    assert_eq(_failures, PackedStringArray())


## A server whose scripts run on `runtime` - "2.7": a game directory without a 3.x runtime, "3": one
## with it and no 2.7 runtime; null, and the test pending, when the runtime is not set
func _create_server_on(runtime:String) -> Object:
    var home:String = OS.get_environment("MASZYNA_PYTHON_HOME" if runtime == "2.7" else "MASZYNA_PYTHON3_HOME")
    if not home:
        pending("no runtime for Python %s is set" % runtime)
        return null
    ProjectSettings.set_setting(HOME_SETTING, home if runtime == "2.7" else "missing-python-home")
    MaszynaGameDirectory.save_game_dir(_make_game_dir(runtime, home if runtime == "3" else ""))
    return _create_server()


## A game directory of the test's own: the fixtures' modules and, when `home3` is set, that 3.x
## runtime linked in, as the game data has them; its path, absolute
func _make_game_dir(name:String, home3:String) -> String:
    var game_dir:String = ProjectSettings.globalize_path(GAME_DIRS.path_join(name))
    DirAccess.make_dir_recursive_absolute(game_dir)
    var links:Dictionary[String, String] = {
        game_dir.path_join(FIXTURE_MODULES.get_file()): ProjectSettings.globalize_path(FIXTURE_MODULES),
    }
    if home3:
        links[game_dir.path_join(home3.simplify_path().get_file())] = home3
    for link:String in links:
        DirAccess.remove_absolute(link)
        DirAccess.open(game_dir).create_link(links[link], link)
    return game_dir


## A server of the test's own, freed after it. The global name PythonScreenServer is the engine's
## singleton, so the class is reached through ClassDB.
func _create_server() -> Object:
    var server:Object = ClassDB.instantiate("PythonScreenServer")
    server.python_runtime_failed.connect(_on_python_runtime_failed)
    server.screen_rendered.connect(_on_screen_rendered)
    _servers.append(server)
    return server


func _on_python_runtime_failed(message:String) -> void:
    _failures.append(message)


func _on_screen_rendered(screen:RID) -> void:
    _rendered.append(screen)


func _on_commands_received(commands:PackedStringArray) -> void:
    _commands = commands
