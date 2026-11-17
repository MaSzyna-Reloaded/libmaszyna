extends MaszynaGutTest

## A vehicle's low-poly interior is drawn behind its body where they meet (#422) by a plain uniform
## of its own materials - an instance uniform in the material types made Godot reserve a block of
## its global shader buffer for every scenery mesh, and a large scenery ran out of it (#442).

const MATERIALS_GAME_DIR: String = "res://tests/materials"
const MATERIAL_NAME: String = "nontransparent_manager"
const DRAW_BEHIND_PARAMETER: StringName = &"draw_behind_depth"
const TYPES_DIR: String = "res://addons/libmaszyna/materials/types"
const SHADER_EXTENSIONS: Array[String] = ["gdshader", "gdshaderinc"]
## The free spotlights carry their light by instance uniforms, one instance per lamp
const INSTANCE_UNIFORM_SHADER_PREFIX: String = "free_spotlight"
const INSTANCE_UNIFORM: String = "instance uniform"

var _previous_game_dir: String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(MATERIALS_GAME_DIR)
    MaszynaMaterialManager.clear_cache()


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)
    MaszynaMaterialManager.clear_cache()


func _submodel_material(low_poly_interior: bool) -> ShaderMaterial:
    var submodel: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    submodel.material_name = MATERIAL_NAME
    return MaszynaMaterialManager.get_submodel_material(
        submodel, "test", PackedStringArray(), MaszynaE3DRenderingServer.TRANSLUCENCY_CUTOUT,
        MaszynaMaterialManager.MAX_TEXTURE_SIZE_PROJECT_DEFAULT, low_poly_interior
    ) as ShaderMaterial


func test_low_poly_interior_material_is_drawn_behind() -> void:
    var interior: ShaderMaterial = _submodel_material(true)

    assert_not_null(interior)
    assert_eq(
        interior.get_shader_parameter(DRAW_BEHIND_PARAMETER),
        MaszynaMaterialFactory.LOW_POLY_INTERIOR_DEPTH_BEHIND
    )


func test_other_material_is_not_drawn_behind() -> void:
    var body: ShaderMaterial = _submodel_material(false)

    assert_not_null(body)
    assert_eq(body.get_shader_parameter(DRAW_BEHIND_PARAMETER), 0.0)


func test_low_poly_interior_gets_a_material_of_its_own() -> void:
    var interior: ShaderMaterial = _submodel_material(true)
    var body: ShaderMaterial = _submodel_material(false)

    assert_ne(interior, body)


func test_material_types_declare_no_instance_uniform() -> void:
    for file_name: String in DirAccess.get_files_at(TYPES_DIR):
        if not file_name.get_extension() in SHADER_EXTENSIONS or file_name.begins_with(INSTANCE_UNIFORM_SHADER_PREFIX):
            continue
        var code: String = FileAccess.get_file_as_string(TYPES_DIR.path_join(file_name))
        assert_false(code.contains(INSTANCE_UNIFORM), file_name)
