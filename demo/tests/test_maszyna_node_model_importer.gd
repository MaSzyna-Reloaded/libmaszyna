extends MaszynaGutTest

## Scenery "node model" becomes MaszynaModelData (built as an E3DRenderingServer RID by
## SceneryInstancer), not an E3DModelInstance node.

const NodeImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_importer.gd")


func test_model_node_is_imported_as_data_with_context_transform() -> void:
    var parser: MaszynaParser = MaszynaParser.new()
    parser.initialize("500 10 house model 1 2 3 90 budynki/dom.t3d skin1|skin2 endmodel".to_utf8_buffer())
    var context: MaszynaImporterContext = MaszynaImporterContext.new()
    context.origin = Vector3(100, 0, 0)

    var objects: Array = NodeImporter.new().import(parser, context)

    assert_false(objects.any(func(object: Variant) -> bool: return object is Node), "no nodes for models")
    assert_eq(context.models.size(), 1)
    var model: MaszynaModelData = context.models[0]
    assert_eq(model.data_path, "models/budynki")
    assert_eq(model.model_filename, "dom")
    assert_eq(model.skins, PackedStringArray(["skin1", "skin2"]))
    assert_eq(model.position, Vector3(101, 2, 3))
    assert_almost_eq(model.rotation.y, deg_to_rad(90.0), 0.0001)
    assert_eq(model.range_min, 10.0)
    assert_eq(model.range_max, 500.0)


## A model is looked for as its path is given, from the game directory, then under models/
## (MdlMngr.cpp:146-150): Sandomierz's platform is "models\linia053\peron_sandomierz.t3d"
func test_model_path_is_looked_for_as_given_then_under_models() -> void:
    var previous_game_dir:String = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir("res://tests/fixtures")
    var parser: MaszynaParser = MaszynaParser.new()
    parser.initialize((
        "-1 0 given model 0 0 0 0 models\\t3d\\legacy.t3d none endmodel "
        + "-1 0 under_models model 0 0 0 0 t3d\\legacy.t3d none endmodel"
    ).to_utf8_buffer())
    var context: MaszynaImporterContext = MaszynaImporterContext.new()

    NodeImporter.new().import(parser, context)
    NodeImporter.new().import(parser, context)
    UserSettings.save_maszyna_game_dir(previous_game_dir)

    assert_eq(context.models.size(), 2)
    assert_eq(context.models[0].data_path, "models/t3d", "a path given from the game directory")
    assert_eq(context.models[1].data_path, "models/t3d", "a path under models/")


## A `lights` list ends at the next keyword (AnimModel.cpp:268-277): l107's Agromet factory is
## `lights 4.5 angles 0 80 0 endmodel` - a light, and the factory turned by 80 degrees
func test_lights_end_at_the_next_keyword() -> void:
    var parser: MaszynaParser = MaszynaParser.new()
    parser.initialize((
        "5000 0 factory model 0 0 0 0 przemysl/fabryka.t3d none lights 4.5 angles 0 80 0 endmodel "
        + "1000 0 lamp model 0 0 0 0 lampa.t3d none lights 3 1 lightcolors ff0000 -1 notransition endmodel"
    ).to_utf8_buffer())
    var context: MaszynaImporterContext = MaszynaImporterContext.new()

    NodeImporter.new().import(parser, context)
    NodeImporter.new().import(parser, context)

    assert_eq(context.models.size(), 2)
    var factory: MaszynaModelData = context.models[0]
    assert_eq(factory.lights, PackedFloat32Array([4.5]), "the angles read as lights")
    assert_almost_eq(factory.rotation.y, deg_to_rad(80.0), 0.0001, "the angles lost")
    var lamp: MaszynaModelData = context.models[1]
    assert_eq(lamp.lights, PackedFloat32Array([3.0, 1.0]))
    assert_eq(lamp.light_colors, PackedColorArray([Color8(0xff, 0, 0), Color(-1.0, -1.0, -1.0)]))
