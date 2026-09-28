extends MaszynaGutTest

## `pyscreen:` and `pyscreenupdatetime:` of an MMD (Train.cpp:93, :10662-10740): both forms of a
## screen, the update interval resolved against the vehicle's and the global one, and the
## instruments around them still read.

const FIXTURE_PATH:String = "res://tests/fixtures/test_cabin_pyscreen.mmd"
## A game directory without the scripts - a test reads no game data of its own
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

var _definition:MmdCabinDefinition = null
var _previous_game_dir:String


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(ProjectSettings.globalize_path(FIXTURES_GAME_DIR))
    _definition = MmdCabinInstancer.parse(ProjectSettings.globalize_path(FIXTURE_PATH), {}, 1, {})


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_block_form_screen_reads_script_target_and_parameters() -> void:
    var screen:MmdPythonScreenDescriptor = _definition.python_screens[0]
    # the spelling of the MMD - the file is found whatever its case when it is there (MaszynaDataPath)
    assert_eq(screen.script_path, UserSettings.get_maszyna_game_dir().path_join("scripts/Koliber"))
    assert_eq(screen.target, "ekran")
    var parameters:Dictionary = {"kod_e": "ic", "margin1": "20"}
    assert_eq(screen.parameters, parameters)


func test_legacy_form_screen_script_without_directory_lives_next_to_the_vehicle() -> void:
    var screen:MmdPythonScreenDescriptor = _definition.python_screens[1]
    assert_eq(screen.target, "wyswietlacz")
    assert_eq(screen.script_path, ProjectSettings.globalize_path(FIXTURE_PATH).get_base_dir().path_join("traxx_renderer"))


func test_update_interval_falls_back_to_the_vehicle_and_is_bounded_by_the_global_one() -> void:
    var intervals:Array[int] = []
    for screen:MmdPythonScreenDescriptor in _definition.python_screens:
        intervals.append(screen.update_time_msec)
    var expected:Array[int] = [500, 500, 250]
    assert_eq(intervals, expected)
    assert_eq(_definition.python_screens[2].target, "none")


func test_instruments_after_a_screen_are_still_read() -> void:
    var labels:Array[String] = []
    for descriptor:MmdInstrumentDescriptor in _definition.instruments:
        labels.append(descriptor.label)
    var expected:Array[String] = ["mainctrl", "dirkey"]
    assert_eq(labels, expected)
