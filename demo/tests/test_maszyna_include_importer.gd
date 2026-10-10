extends MaszynaGutTest

## Include paths in the original assets are authored on Windows: backslash separators and
## arbitrary case (maszyna_include_importer.gd).

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_backslash_include_paths_resolve() -> void:
    var context := MaszynaImporterContext.new()
    SceneryInstancer.parse_file("threaded/backslash.scn", {}, context)

    # both includes name threaded/b.inc, the second one in a case the disk does not use;
    # each one passes its own parameter, so a model per include is expected
    var filenames:Array = context.models.map(
        func(model:MaszynaModelData) -> String: return model.model_filename
    )
    assert_eq(filenames, ["bs1", "bs2"])
    # an unresolved include would have made the scenery uncacheable
    assert_true(context.cacheable)


## An origin inside an origin adds to it, as in the original (simulationstateserializer.cpp:651);
## replacing it put every switch's ballast and sleepers at the scenery's (0, 0, 0)
func test_nested_origin_adds_to_the_enclosing_one() -> void:
    var context := MaszynaImporterContext.new()
    SceneryInstancer.parse_file("nested_origin.scn", {}, context)

    var positions:Dictionary[String, Vector3] = {}
    for model:MaszynaModelData in context.models:
        positions[model.model_filename] = model.position
    assert_almost_eq(positions["inner"], Vector3(100.0, -0.2, 51.0), Vector3.ONE * 0.001)
    assert_almost_eq(positions["outer"], Vector3(100.0, 0.0, 50.0), Vector3.ONE * 0.001)
    assert_almost_eq(positions["after"], Vector3(1.0, 0.0, 0.0), Vector3.ONE * 0.001)
