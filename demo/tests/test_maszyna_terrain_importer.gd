extends MaszynaGutTest

## `terrain ... endterrain` (state_serializer::deserialize_terrain(), include_terrain_file()): every
## file it names is taken in binary (.btf) - the one beside a text terrain file (.txtf) where it was
## made from that text or is alone, else one made of the text (MaszynaBTFTerrainWriter) - and the
## text read as any scenery file only where no binary file can hold it; a name of another extension
## (.scm) as named while that file is there, else as the terrain file of its name. The fixtures'
## terrain files place a model each, named after the file read - what a binary file cannot hold.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const TERRAIN_DIR:String = "res://tests/fixtures/scenery/terrain"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _parse(scenery:String) -> MaszynaImporterContext:
    var context := MaszynaImporterContext.new()
    MaszynaSceneryProvider.parse_file(scenery, {}, context)
    return context


func _models(context:MaszynaImporterContext) -> Array:
    return context.models.map(func(model:ModelData) -> String: return model.name)


func _binary(file_name:String) -> Array[String]:
    var path:String = MaszynaGameDirectory.get_game_dir().path_join("scenery").path_join("terrain").path_join(file_name)
    return [path] as Array[String]


func test_a_text_terrain_file_of_triangles_is_made_binary() -> void:
    var context:MaszynaImporterContext = _parse("terrain_triangles.scn")
    assert_eq(_models(context), [])
    assert_eq(context.terrain_binary_files.size(), 1)
    var text_length:int = FileAccess.get_file_as_bytes(TERRAIN_DIR.path_join("triangles.txtf")).size()
    assert_eq(MaszynaBTFTerrainProvider.get_source_length(context.terrain_binary_files[0]), text_length)


func test_a_text_terrain_file_no_binary_file_can_hold_is_read_as_any_scenery_file() -> void:
    var context:MaszynaImporterContext = _parse("terrain_text.scn")
    assert_eq(_models(context), ["ground"])
    assert_eq(context.terrain_binary_files, [] as Array[String])
    assert_push_error("can't be a part of binary terrain")


func test_every_file_the_directive_names_is_read_in_its_order() -> void:
    assert_eq(_models(_parse("terrain_several.scn")), ["ground", "named"])
    assert_push_error_count(2)


func test_a_scm_name_without_its_file_is_read_as_its_text_terrain_file() -> void:
    assert_eq(_models(_parse("terrain_scm.scn")), ["named"])
    assert_push_error("can't be a part of binary terrain")


func test_a_scm_name_with_its_file_is_read_as_named() -> void:
    assert_eq(_models(_parse("terrain_scm_present.scn")), ["scm"])


func test_the_binary_file_made_from_the_text_stands_in_for_it() -> void:
    var context:MaszynaImporterContext = _parse("terrain_binary.scn")
    assert_eq(_models(context), [], "the text is not read")
    assert_eq(context.terrain_binary_files, _binary("binary.btf"))


func test_a_binary_file_made_from_another_text_is_passed_over() -> void:
    var context:MaszynaImporterContext = _parse("terrain_stale.scn")
    assert_eq(_models(context), ["stale"])
    assert_eq(context.terrain_binary_files, [] as Array[String])
    assert_push_error("can't be a part of binary terrain")


func test_a_binary_file_alone_is_read() -> void:
    var context:MaszynaImporterContext = _parse("terrain_alone.scn")
    assert_eq(context.terrain_binary_files, _binary("alone.btf"))


## The binary file holds the geometry where the text alone puts it (include_terrain_file())
func test_under_an_origin_only_the_text_is_read() -> void:
    var context:MaszynaImporterContext = _parse("terrain_relocated.scn")
    assert_eq(_models(context), ["text"])
    assert_eq(context.terrain_binary_files, [] as Array[String])
