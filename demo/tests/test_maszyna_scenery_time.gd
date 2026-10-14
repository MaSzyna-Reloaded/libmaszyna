extends MaszynaGutTest

## The scenario clock a scenery starts at: its "time" section, else the original's 10:30
## (scenario_time(), simulationtime.h:21)

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY_WITH_TIME:String = "evening_time.scn"
const SCENERY_WITHOUT_TIME:String = "no_time.scn"
## evening_time.scn: time 18:45
const EVENING_START_TIME:float = 18.75

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    _scenery.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_a_scenery_without_a_time_section_starts_at_half_past_ten() -> void:
    await _load(SCENERY_WITHOUT_TIME)

    assert_eq(_scenery.start_time, MaszynaSceneryNode.START_TIME_DEFAULT)


func test_a_scenery_starts_at_its_time_section() -> void:
    await _load(SCENERY_WITH_TIME)

    assert_eq(_scenery.start_time, EVENING_START_TIME)


func _load(scenery_file:String) -> void:
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = scenery_file
    add_child(_scenery)
    if not await wait_loaded(_scenery.scenery_loaded, _scenery.filename):
        return
