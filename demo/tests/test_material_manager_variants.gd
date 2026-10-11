extends MaszynaGutTest

const MATERIALS_GAME_DIR = "res://tests/materials"
const MATERIAL_NAME = "seasonal_manager"
const NONTRANSPARENT_MATERIAL_NAME = "nontransparent_manager"
const NORMALMAP_SHADER_PATH = "res://addons/libmaszyna/legacy/materials/types/normalmap.gdshader"
const SHADOWLESS_SHADER_PATH = "res://addons/libmaszyna/legacy/materials/types/shadowlessnormalmap.gdshader"
const WATER_SHADER_PATH = "res://addons/libmaszyna/legacy/materials/types/water.gdshader"

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
    assert_eq(variant.get_texture_path("tex1"), "spring_diffuse")


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

    assert_eq(variant.get_texture_path("tex1"), "cloudy_diffuse")


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
    var material: ShaderMaterial = MaszynaMaterialFactory.create(mmat) as ShaderMaterial

    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.AlphaScissor)
    assert_eq(material.get_shader_parameter("alpha_scissor_threshold"), 0.5)


func test_create_sets_alpha_blending_for_e3d_translucent_submodel() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new()
    options.force_transparent = true
    var material: ShaderMaterial = MaszynaMaterialFactory.create(
        mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR,
        options,
    ) as ShaderMaterial

    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.Alpha)
    assert_true(material.shader.code.contains("#define MASZYNA_ALPHA_BLEND"))


## A forced submodel of the optimized E3D instancer is drawn opaque, not even cut out - that
## instancer never uses the alpha pass (E3DRenderingServer.TRANSLUCENCY_OPAQUE)
func test_create_draws_a_transparent_material_opaque_for_the_optimized_instancer() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", MATERIAL_NAME)
    var options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new()
    options.force_opaque = true
    var material: ShaderMaterial = MaszynaMaterialFactory.create(
        mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR,
        options,
    ) as ShaderMaterial

    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.Disabled)


## The original's opaque pass keeps the texels at the material's "opacity:" and above
## (opengl33renderer.cpp:2247-2258) - EP09's skin gives 0.92 and has its glass at 0.79
func test_create_cuts_out_at_the_opacity_of_the_material() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", "opacity_manager")
    var material: ShaderMaterial = MaszynaMaterialFactory.create(mmat) as ShaderMaterial

    assert_false(mmat.transparent, "no texture of it asks for transparency")
    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.AlphaScissor)
    assert_almost_eq(float(material.get_shader_parameter("alpha_scissor_threshold")), 0.92, 0.0001)


func test_create_uses_diffuse_color_for_default_shader_without_texture() -> void:
    var mmat: MaszynaMaterial = MaszynaMaterial.new()
    var diffuse_color: Color = Color(0.25, 0.5, 0.75, 1.0)
    var options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new()
    options.diffuse_color = diffuse_color
    var material: ShaderMaterial = MaszynaMaterialFactory.create(
        mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR,
        options
    ) as ShaderMaterial

    assert_eq(material.get_shader_parameter("albedo"), diffuse_color)


func test_create_uses_diffuse_color_for_default_shader_with_texture() -> void:
    # Regression: "albedo" used to only be set from options.diffuse_color for untextured
    # submodels - every textured submodel (nontransparent_manager.mat has texture1: base_diffuse)
    # stayed stuck at the shader's default white, even when the E3D file's own diffuse color was
    # something else entirely (confirmed real: EP07's "wylszybki_on"/"_off" indicator lamp
    # submodels, diffuse (0, 0.749, 0) over a neutral texture, rendered white instead of green).
    var mmat: MaszynaMaterial = MaterialManager.load_material("", NONTRANSPARENT_MATERIAL_NAME)
    var diffuse_color: Color = Color(0.0, 0.749, 0.0, 1.0)
    var options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new()
    options.diffuse_color = diffuse_color
    var material: ShaderMaterial = MaszynaMaterialFactory.create(
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
    var material: ShaderMaterial = MaszynaMaterialFactory.create(
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
    var material: ShaderMaterial = MaszynaMaterialFactory.create(transparent_mmat) as ShaderMaterial

    MaszynaMaterialFactory.apply(
        material,
        nontransparent_mmat,
        "",
        MaszynaEnvironment.Season.SEASON_SUMMER,
        MaszynaEnvironment.Weather.WEATHER_CLEAR
    )

    assert_eq(material.get_shader_parameter("transparency"), MaterialManager.Transparency.Disabled)
    assert_eq(material.get_shader_parameter("alpha_scissor_threshold"), 0.5)


func test_default_shader_name_uses_default_material() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "default_shader_manager") as ShaderMaterial

    assert_eq(material.shader.resource_path, "res://addons/libmaszyna/legacy/materials/types/default.gdshader")


## mat_detail_normalmap.frag - normalmap plus a tiled detail normal map.
func test_detail_normalmap_shader_applies_detail_parameters() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", "detail_normalmap_manager")
    var material: ShaderMaterial = MaterialManager.get_material("", "detail_normalmap_manager") as ShaderMaterial

    assert_eq(mmat.default.get_texture_path("detailnormalmap"), "fx/t_detail_normal_3")
    assert_eq(material.shader.resource_path, "res://addons/libmaszyna/legacy/materials/types/detail_normalmap.gdshader")
    assert_almost_eq(float(material.get_shader_parameter("detail_scale")), 0.00125, 0.00001)
    assert_almost_eq(float(material.get_shader_parameter("detail_height_scale")), 0.45, 0.00001)
    assert_not_null(material.get_shader_parameter("texture_detail_normal"))


## The original resolves "mat_<name>.frag" on a case insensitive file system (dynamic/road/clio/kolo.mat)
func test_shader_name_is_case_insensitive() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", "mixed_case_shader_manager")
    var material: ShaderMaterial = MaterialManager.get_material("", "mixed_case_shader_manager") as ShaderMaterial

    assert_eq(mmat.default.shader, "default_1")
    assert_eq(material.shader.resource_path, "res://addons/libmaszyna/legacy/materials/types/default.gdshader")
    assert_eq(material.get_shader_parameter("metallic_texture_channel"), Vector4(1.0, 0.0, 0.0, 0.0))


## mat_reflmap.frag - "texture2:" is the reflection map there, its alpha scales param_reflection (one by default)
func test_reflmap_shader_binds_second_texture_as_reflection_map() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "reflmap_manager") as ShaderMaterial

    assert_not_null(material.get_shader_parameter("texture_metallic"))
    assert_eq(material.get_shader_parameter("metallic_texture_channel"), Vector4(0.0, 0.0, 0.0, 1.0))
    assert_almost_eq(float(material.get_shader_parameter("metallic")), 1.0, 0.00001)
    assert_null(material.get_shader_parameter("texture_normal"))


func test_reflmap_shader_applies_reflection_parameter() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "reflmap_reflection_manager") as ShaderMaterial

    assert_not_null(material.get_shader_parameter("texture_metallic"))
    assert_almost_eq(float(material.get_shader_parameter("metallic")), 0.25, 0.00001)


## material.cpp:117-134 - without "shader:" the bound textures pick default_0/1/2 (colored, default, reflmap)
func test_shaderless_material_with_second_texture_uses_reflmap() -> void:
    var mmat: MaszynaMaterial = MaterialManager.load_material("", "shaderless_two_textures_manager")
    var clear_material: ShaderMaterial = MaszynaMaterialFactory.create(
        mmat, "", MaszynaEnvironment.Season.SEASON_SUMMER, MaszynaEnvironment.Weather.WEATHER_CLEAR
    ) as ShaderMaterial
    var rain_material: ShaderMaterial = MaszynaMaterialFactory.create(
        mmat, "", MaszynaEnvironment.Season.SEASON_SUMMER, MaszynaEnvironment.Weather.WEATHER_RAIN
    ) as ShaderMaterial

    assert_null(clear_material.get_shader_parameter("texture_metallic"))
    assert_not_null(rain_material.get_shader_parameter("texture_metallic"))
    assert_eq(rain_material.get_shader_parameter("metallic_texture_channel"), Vector4(0.0, 0.0, 0.0, 1.0))
    assert_null(rain_material.get_shader_parameter("texture_normal"))


## mat_colored.frag
func test_colored_shader_uses_color_parameter() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "colored_manager") as ShaderMaterial

    assert_eq(material.get_shader_parameter("albedo"), Color(0.1, 0.2, 0.3, 1.0))
    assert_null(material.get_shader_parameter("texture_albedo"))


## mat_default_detail.frag - the detail normal map is its second texture, there is no base normal map
func test_default_detail_shader_binds_second_texture_as_detail_normal_map() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "default_detail_manager") as ShaderMaterial

    assert_eq(material.shader.resource_path, "res://addons/libmaszyna/legacy/materials/types/detail_normalmap.gdshader")
    assert_not_null(material.get_shader_parameter("texture_detail_normal"))
    assert_null(material.get_shader_parameter("texture_normal"))
    assert_almost_eq(float(material.get_shader_parameter("detail_scale")), 0.00125, 0.00001)


func test_detail_parallax_specgloss_shader_binds_detail_and_specgloss_textures() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "detail_parallax_specgloss_manager") as ShaderMaterial

    assert_eq(material.shader.resource_path, "res://addons/libmaszyna/legacy/materials/types/parallax_specgloss.gdshader")
    assert_true(material.get_shader_parameter("use_detail_normal"))
    assert_not_null(material.get_shader_parameter("specgloss_texture"))


func test_rain_windscreen_shader_binds_textures_and_grid_size() -> void:
    var material: ShaderMaterial = MaterialManager.get_material("", "rain_windscreen_manager") as ShaderMaterial

    assert_eq(material.shader.resource_path, "res://addons/libmaszyna/legacy/materials/types/rain_windscreen.gdshader")
    assert_not_null(material.get_shader_parameter("diffuse_texture"))
    assert_not_null(material.get_shader_parameter("raindrops_atlas"))
    assert_not_null(material.get_shader_parameter("wiper_mask"))
    assert_almost_eq(float(material.get_shader_parameter("raindrop_grid_size")), 150.0, 0.00001)
