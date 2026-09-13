extends MaszynaGutTest

const MATERIALS_GAME_DIR = "res://tests/materials"
const MATERIAL_NAME = "seasonal_manager"
const NONTRANSPARENT_MATERIAL_NAME = "nontransparent_manager"
const NORMALMAP_SHADER_PATH = "res://addons/libmaszyna/materials/types/normalmap.gdshader"
const SHADOWLESS_SHADER_PATH = "res://addons/libmaszyna/materials/types/shadowlessnormalmap.gdshader"
const WATER_SHADER_PATH = "res://addons/libmaszyna/materials/types/water.gdshader"

var _previous_game_dir: String = ""
var _previous_season: MaszynaEnvironment.Season
var _previous_weather: MaszynaEnvironment.Weather


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    _previous_season = MaterialManager.season
    _previous_weather = MaterialManager.weather
    UserSettings.save_maszyna_game_dir(MATERIALS_GAME_DIR)
    MaterialManager.season = MaszynaEnvironment.Season.SEASON_SUMMER
    MaterialManager.weather = MaszynaEnvironment.Weather.WEATHER_CLEAR
    MaterialManager.clear_cache()


func after_each() -> void:
    MaterialManager.season = _previous_season
    MaterialManager.weather = _previous_weather
    UserSettings.save_maszyna_game_dir(_previous_game_dir)
    MaterialManager.clear_cache()


func test_parse_stores_default_shader_on_default_variant() -> void:
    var material: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)

    assert_eq(material.default.shader, "normalmap")


func test_variant_inherits_default_shader_when_season_has_no_shader() -> void:
    var material: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var variant: MaszynaMaterial.MaszynaMaterialVariant = material.get_variant(
        MaszynaEnvironment.Season.SEASON_SPRING,
        MaszynaEnvironment.Weather.WEATHER_CLEAR
    )

    assert_eq(variant.shader, "normalmap")
    assert_eq(variant.get_texture_path("diffuse"), "spring_diffuse")


func test_weather_shader_overrides_season_shader() -> void:
    var material: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var variant: MaszynaMaterial.MaszynaMaterialVariant = material.get_variant(
        MaszynaEnvironment.Season.SEASON_WINTER,
        MaszynaEnvironment.Weather.WEATHER_RAIN
    )

    assert_eq(variant.shader, "water")


func test_weather_variant_merges_after_season_variant() -> void:
    var material: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var variant: MaszynaMaterial.MaszynaMaterialVariant = material.get_variant(
        MaszynaEnvironment.Season.SEASON_SPRING,
        MaszynaEnvironment.Weather.WEATHER_CLOUDY
    )

    assert_eq(variant.get_texture_path("diffuse"), "cloudy_diffuse")


func test_get_material_updates_existing_material_in_place_when_season_changes() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", MATERIAL_NAME) as ShaderMaterial
    var original_rid: RID = material.get_rid()

    MaterialManager.season = MaszynaEnvironment.Season.SEASON_WINTER

    assert_same(material, MaterialManager.get_material("", MATERIAL_NAME))
    assert_eq(material.get_rid(), original_rid)
    assert_eq(material.shader.resource_path, SHADOWLESS_SHADER_PATH)


func test_get_material_updates_existing_material_in_place_when_weather_changes() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", MATERIAL_NAME) as ShaderMaterial
    var original_rid: RID = material.get_rid()

    MaterialManager.season = MaszynaEnvironment.Season.SEASON_WINTER
    MaterialManager.weather = MaszynaEnvironment.Weather.WEATHER_RAIN

    assert_same(material, MaterialManager.get_material("", MATERIAL_NAME))
    assert_eq(material.get_rid(), original_rid)
    assert_eq(material.shader.resource_path, WATER_SHADER_PATH)


func test_refresh_prunes_dead_managed_material_entries() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", MATERIAL_NAME) as ShaderMaterial
    var material_ref: WeakRef = weakref(material)

    material = null
    MaterialManager.season = MaszynaEnvironment.Season.SEASON_WINTER

    assert_null(material_ref.get_ref())
    var refreshed_material: ShaderMaterial = MaterialManager.get_material("", MATERIAL_NAME) as ShaderMaterial
    assert_not_null(refreshed_material)
    assert_eq(refreshed_material.shader.resource_path, SHADOWLESS_SHADER_PATH)


func test_get_material_uses_default_shader_before_variant_override() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", MATERIAL_NAME) as ShaderMaterial

    assert_eq(material.shader.resource_path, NORMALMAP_SHADER_PATH)


func test_texture_transparency_suffix_sets_material_transparent() -> void:
    var material: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)

    assert_true(material.transparent)


func test_create_sets_alpha_scissor_for_transparent_material() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var material: ShaderMaterial = MaterialFactory.create(mmat) as ShaderMaterial

    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.AlphaScissor)
    assert_eq(material.get_shader_parameter("alpha_scissor_threshold"), 0.5)


func test_create_uses_diffuse_color_for_default_shader_without_texture() -> void:
    var mmat: MaszynaMaterial = MaszynaMaterial.new()
    var diffuse_color: Color = Color(0.25, 0.5, 0.75, 1.0)
    var options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new()
    options.diffuse_color = diffuse_color
    var material: ShaderMaterial = MaterialFactory.create(
        mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR,
        options
    ) as ShaderMaterial

    assert_eq(material.get_shader_parameter("albedo"), diffuse_color)


func test_create_uses_diffuse_color_for_parallax_shader_without_texture() -> void:
    var mmat: MaszynaMaterial = MaszynaMaterial.new()
    mmat.default.shader = "parallax"
    var diffuse_color: Color = Color(0.3, 0.4, 0.5, 1.0)
    var options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new()
    options.diffuse_color = diffuse_color
    var material: ShaderMaterial = MaterialFactory.create(
        mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR,
        options
    ) as ShaderMaterial

    assert_eq(material.get_shader_parameter("albedo_multiplier"), diffuse_color)


func test_apply_updates_existing_material_transparency_state() -> void:
    var transparent_mmat: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var nontransparent_mmat: MaszynaMaterial = MaterialManager.load_material("", NONTRANSPARENT_MATERIAL_NAME)
    var material: ShaderMaterial = MaterialFactory.create(transparent_mmat) as ShaderMaterial

    MaterialFactory.apply(
        material,
        nontransparent_mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR
    )

    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.Disabled)
    assert_eq(material.get_shader_parameter("alpha_scissor_threshold"), 0.5)
