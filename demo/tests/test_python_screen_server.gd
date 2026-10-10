extends MaszynaGutTest

## PythonScreenServer runs a Python 2 cab screen script, in its Python host process, and turns
## what it draws into a texture. The pixels themselves cannot be read back headless (the dummy
## renderer keeps no texture data), so the fixture sends the state it received back as a command.
## A test that needs a CPython 2.7 runtime reads MASZYNA_PYTHON_HOME, its prefix (the directory
## holding lib/libpython2.7.so.1.0), and is skipped without it.
## A host that has ended is not restarted, so a test that ends one does it on a server of its own.

const SCRIPT:String = "res://tests/fixtures/python/fixture_screen"
const ABORT_SCRIPT:String = "res://tests/fixtures/python/fixture_abort_screen"
## The host starts in the game directory; the fixtures stand for one
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const HOME_SETTING:String = "maszyna/python/home"
const RENDER_TIMEOUT_SEC:float = 10.0
## Long enough for a render that should not happen to have happened
const QUIET_WAIT_SEC:float = 0.5

var _previous_home:String = ""
var _previous_game_dir:String = ""
var _screen:RID = RID()
var _commands:PackedStringArray = PackedStringArray()
var _rendered:Array[RID] = []
var _failures:PackedStringArray = PackedStringArray()


func before_each() -> void:
    _previous_home = ProjectSettings.get_setting(HOME_SETTING, "")
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(ProjectSettings.globalize_path(FIXTURES_GAME_DIR))
    _commands = PackedStringArray()
    _failures = PackedStringArray()
    PythonScreenServer.screen_rendered.connect(_on_screen_rendered)


func after_each() -> void:
    PythonScreenServer.screen_rendered.disconnect(_on_screen_rendered)
    PythonScreenServer.screen_free(_screen)
    ProjectSettings.set_setting(HOME_SETTING, _previous_home)
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_script_draws_the_state_into_the_texture_and_sends_commands() -> void:
    var home:String = OS.get_environment("MASZYNA_PYTHON_HOME")
    if not home:
        pending("MASZYNA_PYTHON_HOME is not set")
        return
    ProjectSettings.set_setting(HOME_SETTING, home)
    _screen = PythonScreenServer.screen_create(ProjectSettings.globalize_path(SCRIPT), _on_commands_received)

    var touches:Array = [Vector2(0.25, 0.5)]
    var state:Dictionary = {"level": 1.0, "step": 128, "enabled": true, "name": "ep07-424", "touches": touches}
    PythonScreenServer.screen_request_render(_screen, state)
    await wait_until(func() -> bool: return _commands.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_commands, PackedStringArray([
        "RGBA", ProjectSettings.globalize_path(SCRIPT).get_base_dir() + "/",
        "1.0;128;True;ep07-424;[(0.25, 0.5)]"]))
    var texture:Texture2D = PythonScreenServer.screen_get_texture(_screen)
    assert_eq(texture.get_size(), Vector2(2, 1))
    # the fixture draws a white and a 128 grey pixel - what the screen throws around is their average
    assert_eq(_rendered, [_screen])
    var average:Color = PythonScreenServer.screen_get_average_color(_screen)
    assert_almost_eq(average.r, (1.0 + 128.0 / 255.0) * 0.5, 0.01)
    assert_almost_eq(average.a, 1.0, 0.01)


func test_a_missing_runtime_is_reported_once_and_the_screens_stay_blank() -> void:
    ProjectSettings.set_setting(HOME_SETTING, "missing-python-home")
    var server:Object = _create_server()
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(SCRIPT), _on_commands_received)
    await wait_until(func() -> bool: return _failures.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_failures.size(), 1)
    assert_string_contains(_failures[0], "missing-python-home")
    assert_push_error("Python screens stay blank")
    var state:Dictionary = {"level": 1.0}
    server.screen_request_render(screen, state)
    await wait_seconds(QUIET_WAIT_SEC)
    assert_eq(_failures.size(), 1)
    assert_eq(_commands, PackedStringArray())
    server.screen_free(screen)
    server.free()


func test_a_script_that_aborts_the_interpreter_does_not_end_the_game() -> void:
    var home:String = OS.get_environment("MASZYNA_PYTHON_HOME")
    if not home:
        pending("MASZYNA_PYTHON_HOME is not set")
        return
    ProjectSettings.set_setting(HOME_SETTING, home)
    var server:Object = _create_server()
    var screen:RID = server.screen_create(ProjectSettings.globalize_path(ABORT_SCRIPT), _on_commands_received)
    var state:Dictionary = {"level": 1.0}
    server.screen_request_render(screen, state)
    await wait_until(func() -> bool: return _failures.size() > 0, RENDER_TIMEOUT_SEC)

    assert_eq(_failures.size(), 1)
    assert_string_contains(_failures[0], "exit code")
    assert_push_error("Python screens stay blank")
    server.screen_free(screen)
    server.free()


## A server of the test's own. The global name PythonScreenServer is the engine's singleton, so
## the class is reached through ClassDB.
func _create_server() -> Object:
    var server:Object = ClassDB.instantiate("PythonScreenServer")
    server.python_runtime_failed.connect(_on_python_runtime_failed)
    return server


func _on_python_runtime_failed(message:String) -> void:
    _failures.append(message)


func _on_screen_rendered(screen:RID) -> void:
    _rendered.append(screen)


func _on_commands_received(commands:PackedStringArray) -> void:
    _commands = commands
