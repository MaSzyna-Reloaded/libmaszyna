@tool
extends Node

static var UNKNOWN_MATERIAL = preload("res://addons/libmaszyna/legacy/materials/unknown.tres")
static var UNKNOWN_TEXTURE = preload("res://addons/libmaszyna/legacy/materials/missing_texture.png")
const DDSTextureLoader = preload("res://addons/libmaszyna/legacy/materials/dds_texture_loader.gd")
const COLORED_MATERIAL: Material = preload("res://addons/libmaszyna/legacy/e3d/colored.tres")
## A free spotlight drawn as a point of a constant size on the screen (types/free_spotlight.gdshader)
## and its glare (types/free_spotlight_glare.gdshader) as the next pass
const FREE_SPOTLIGHT_MATERIAL: ShaderMaterial = preload("types/free_spotlight.tres")
const FREE_SPOTLIGHT_GLARE_MATERIAL: ShaderMaterial = preload("types/free_spotlight_glare.tres")
## The original's glare texture (opengl33renderer.cpp:102)
const FREE_SPOTLIGHT_GLARE_TEXTURE: String = "fx/lightglare"
## Whether free spotlights - signals, lamps - get that point and glare, as the original draws them
const RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING: StringName = &"maszyna/scenery/railway_lights_visibility_improved"
## How many times the original's pointsize the point is drawn (the original: 4)
const RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_SETTING: StringName = &"maszyna/scenery/railway_lights_point_size_multiplier"
const RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_DEFAULT: float = 2.0
## How much of the original's glare alpha the glare is drawn with
const RAILWAY_LIGHTS_GLARE_INTENSITY_SETTING: StringName = &"maszyna/scenery/railway_lights_glare_intensity"
const RAILWAY_LIGHTS_GLARE_INTENSITY_DEFAULT: float = 0.5
## The share of a light's falloff angle past which its glare fades out
const RAILWAY_LIGHTS_GLARE_FADE_START_SETTING: StringName = &"maszyna/scenery/railway_lights_glare_fade_start"
const RAILWAY_LIGHTS_GLARE_FADE_START_DEFAULT: float = 0.5
## How many degrees the glare's rays turn per degree the camera goes off the light's axis
const RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_SETTING: StringName = &"maszyna/scenery/railway_lights_glare_rotation_ratio"
const RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_DEFAULT: float = 1.0
## The glare's size at the edge of the light's cone, as a share of its size on the axis
const RAILWAY_LIGHTS_GLARE_EDGE_SIZE_SETTING: StringName = &"maszyna/scenery/railway_lights_glare_edge_size"
const RAILWAY_LIGHTS_GLARE_EDGE_SIZE_DEFAULT: float = 0.4
## The least share of the screen's height a glare spans, however far the light
const RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_SETTING: StringName = &"maszyna/scenery/railway_lights_glare_min_screen_size"
const RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_DEFAULT: float = 0.03

## The point with its glare, shared by every free spotlight; drawn at no size while
## RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING is off
var _free_spotlight_material: ShaderMaterial = null
var _managed_materials: Dictionary = {}
## Weak, like _managed_materials: a texture goes, from RAM and VRAM, with the last material using it
var _dds_cache: Dictionary[String, WeakRef] = {}

enum Transparency { Disabled, Alpha, AlphaScissor }

## A max texture size that takes the project's maszyna/import/dds_max_texture_size
const MAX_TEXTURE_SIZE_PROJECT_DEFAULT: int = 0


class MaterialOptions:
    var diffuse_color: Color = Color.WHITE
    var selfillum_color: Color = Color.WHITE
    var selfillum_energy: float = 1.0
    var selfillum_enabled: bool = false
    # E3D translucent submodels are rendered in a separate alpha-blended pass.
    var force_transparent: bool = false
    # A submodel of the optimized E3D instancer, which never draws in the alpha pass: opaque
    # whatever its texture's alpha, not even cut out (E3DRenderingServer.TRANSLUCENCY_OPAQUE)
    var force_opaque: bool = false
    var alpha_scissor_threshold: float = 0.5
    # Tracks draw both faces - the original disables culling for them in the shadow pass, "roads-based
    # platforms tend to miss parts of shadows" (opengl33renderer.cpp:3609), and an open rail profile
    # with its front faces culled casts almost nothing
    var cull_disabled: bool = false
    # the largest texture size loaded, larger DDS drop their top mipmaps - the cab has its own
    # limit in the original (Train.cpp:660)
    var max_texture_size: int = MAX_TEXTURE_SIZE_PROJECT_DEFAULT


@export var season := MaszynaEnvironment.Season.SEASON_SUMMER:
    set(x):
        if not x == season:
            season = x
            _refresh_managed_materials()

@export var weather := MaszynaEnvironment.Weather.WEATHER_CLEAR:
    set(x):
        if not x == weather:
            weather = x
            _refresh_managed_materials()


func _ready() -> void:
    var glare: ShaderMaterial = FREE_SPOTLIGHT_GLARE_MATERIAL.duplicate()
    glare.set_shader_parameter("glare_texture", load_texture("", FREE_SPOTLIGHT_GLARE_TEXTURE))
    _free_spotlight_material = FREE_SPOTLIGHT_MATERIAL.duplicate()
    _free_spotlight_material.next_pass = glare
    _apply_railway_lights_settings()
    ProjectSettings.settings_changed.connect(_apply_railway_lights_settings)
    E3DRenderingServer.material_set_resolver(get_submodel_material)
    GameDataServer.cache_clear_requested.connect(clear_cache)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)


func _exit_tree() -> void:
    ProjectSettings.settings_changed.disconnect(_apply_railway_lights_settings)
    GameDataServer.cache_clear_requested.disconnect(clear_cache)
    GameDataServer.data_unload_requested.disconnect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)


## Whatever was built of the game's data is built again by its owner and asks for its materials
## anew - none of them is handed out again
func _on_data_unload_requested() -> void:
    _managed_materials.clear()
    _dds_cache.clear()


func _on_data_reload_requested() -> void:
    _free_spotlight_material.next_pass.set_shader_parameter(
        "glare_texture", load_texture("", FREE_SPOTLIGHT_GLARE_TEXTURE))


## The shared material follows its settings, so every free spotlight changes at once; off, the
## point and its glare have no size and no alpha
func _apply_railway_lights_settings() -> void:
    var improved: bool = ProjectSettings.get_setting(RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING, true)
    _free_spotlight_material.set_shader_parameter("point_size_multiplier", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_SETTING, RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_DEFAULT)) if improved else 0.0)
    var glare: ShaderMaterial = _free_spotlight_material.next_pass
    glare.set_shader_parameter("glare_intensity", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_INTENSITY_SETTING, RAILWAY_LIGHTS_GLARE_INTENSITY_DEFAULT)) if improved else 0.0)
    glare.set_shader_parameter("glare_fade_start", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_FADE_START_SETTING, RAILWAY_LIGHTS_GLARE_FADE_START_DEFAULT)))
    glare.set_shader_parameter("glare_rotation_ratio", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_SETTING, RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_DEFAULT)))
    glare.set_shader_parameter("glare_edge_size", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_EDGE_SIZE_SETTING, RAILWAY_LIGHTS_GLARE_EDGE_SIZE_DEFAULT)))
    glare.set_shader_parameter("glare_min_screen_size", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_SETTING, RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_DEFAULT)))


func clear_cache() -> void:
    _dds_cache.clear()
    _refresh_managed_materials()

func load_material(model_path:String, material_name:String) -> MaszynaMaterial:
    return MaszynaMaterialParser.parse(model_path, material_name)

func get_material(
    model_path:String,
    material_path:String,
    options: MaterialOptions = MaterialOptions.new(),
) -> Material:
    var material_key: String = _material_key(model_path, material_path, options)
    var managed_material: Dictionary = _managed_materials.get(material_key, {})
    if managed_material:
        var material_ref: WeakRef = managed_material.get("material_ref") as WeakRef
        var material: ShaderMaterial = material_ref.get_ref() as ShaderMaterial if material_ref else null
        if material:
            return material
        _managed_materials.erase(material_key)
    # built anew, never read from a disk cache: a cached material carried its textures embedded,
    # reading it cost up to 80 ms on the main thread and apply() loaded every texture again anyway
    # (docs/findings-archive.md, 2026-10-02 streaming hitches)
    var mmat: MaszynaMaterial = load_material(model_path, material_path)
    var output: ShaderMaterial = MaszynaMaterialFactory.create(mmat, model_path, season, weather, options)
    _managed_materials[material_key] = {
        "material_ref": weakref(output),
        # most materials declare no season or weather variant and never change with them
        "has_variants": mmat.variants.size() > 0,
        "model_path": model_path,
        "material_path": material_path,
        "options": options,
    }
    return output

## Material override of an E3D submodel - the material resolver of [E3DRenderingServer].
## The first segment of [param data_path] is dropped from the material search path
## (see maszyna_rail_vehicle_3d_instancer.gd's _build_structure()).
func get_submodel_material(
    submodel: E3DSubModel,
    data_path: String,
    skins: PackedStringArray,
    translucency: E3DRenderingServer.Translucency,
    max_texture_size: int,
) -> Material:
    # E3DOptimizedBackend draws a free spotlight only when it gets a material for it
    if submodel.submodel_type == E3DSubModel.SUBMODEL_FREE_SPOTLIGHT:
        return _free_spotlight_material

    var unprefixed_model_path: String = "/".join(data_path.split("/").slice(1))
    var options: MaterialOptions = MaterialOptions.new()

    # TODO: handle more material options here (selfillum, etc)
    options.force_transparent = translucency == E3DRenderingServer.TRANSLUCENCY_BLENDED
    options.force_opaque = translucency == E3DRenderingServer.TRANSLUCENCY_OPAQUE
    options.max_texture_size = max_texture_size
    options.diffuse_color = submodel.diffuse_color
    options.selfillum_color = (
        submodel.self_illumination
        if submodel.self_illumination and not submodel.self_illumination == Color.BLACK
        else Color.WHITE
    )
    options.selfillum_energy = options.selfillum_color.a  # legacy renderer
    options.selfillum_enabled = options.selfillum_energy > 0.0 and submodel.lights_on_threshold >= 1.0  # legacy renderer logic

    if submodel.dynamic_material:
        if skins.size() < submodel.dynamic_material_index + 1:
            push_warning(
                "Model %s has no skins set, but submodel requires material #%s"
                % [data_path, submodel.dynamic_material_index]
            )
            return null
        return get_material(unprefixed_model_path, skins[submodel.dynamic_material_index], options)

    if submodel.material_colored:
        return COLORED_MATERIAL

    if submodel.material_name:
        return get_material(unprefixed_model_path, submodel.material_name, options)

    return null


func get_texture(texture_path:String) -> Texture:
    return load_texture("", texture_path)

func load_texture(
    model_path:String,
    material_name:String,
    normal:bool = false,
    max_texture_size:int = MAX_TEXTURE_SIZE_PROJECT_DEFAULT,
) -> Texture:
    var project_data_dir:String = UserSettings.get_maszyna_game_dir()
    if (project_data_dir.ends_with("\\")):
        project_data_dir = project_data_dir.trim_suffix("\\")
    if (project_data_dir.ends_with("/")):
        project_data_dir = project_data_dir.trim_suffix("/")

    var possible_paths:Array[String] = [
        model_path.path_join(material_name+".dds"),
        "textures".path_join(model_path.path_join(material_name+".dds")),
        material_name+".dds",
        "textures".path_join(material_name+".dds"),
    ]

    var final_path:String = ""
    for p:String in possible_paths:
        var resolved_path:String = MaszynaDataPath.resolve(project_data_dir, p)
        if FileAccess.file_exists(project_data_dir.path_join(resolved_path)):
            final_path = resolved_path
            break

    if not final_path:
        return UNKNOWN_TEXTURE

    var full_path:String = project_data_dir.path_join(final_path)
    var max_size:int = max_texture_size
    if max_size == MAX_TEXTURE_SIZE_PROJECT_DEFAULT:
        max_size = int(ProjectSettings.get_setting("maszyna/import/dds_max_texture_size", 1024))
    var texture:Texture2D = _load_dds_clamped(full_path, max_size)
    if not texture:
        texture = load(full_path) as Texture2D
    if texture:
        return texture
    return UNKNOWN_TEXTURE


## Loads a .dds with its top mipmap levels discarded down to max_size - port of the original
## engine's iMaxTextureSize/maxtexturesize clamp (Texture.cpp), which this wrapper had no
## equivalent of. Cached by path+max_size since this bypasses Godot's own load() resource
## cache, and the same texture file is commonly referenced by several distinct materials
## (e.g. a shared normal map across dynamic skin slots).
func _load_dds_clamped(full_path:String, max_size:int) -> Texture2D:
    var cache_key:String = "%s:%d" % [full_path, max_size]
    var cached:WeakRef = _dds_cache.get(cache_key)
    var texture:Texture2D = cached.get_ref() as Texture2D if cached else null
    if texture:
        return texture
    texture = DDSTextureLoader.load_texture(full_path, max_size)
    if texture:
        _dds_cache[cache_key] = weakref(texture)
    return texture


## The material handed out for a path with these options, the same while it is alive
func _material_key(
    model_path: String,
    material_path: String,
    options: MaterialOptions,
) -> String:
    # the same material path is another material in another game directory
    var options_hash = ":".join([
        UserSettings.get_maszyna_game_dir(),
        options.force_transparent,
        options.force_opaque,
        options.diffuse_color.to_html(true),
        options.alpha_scissor_threshold,
        options.selfillum_enabled,
        options.selfillum_color.to_html(true),
        options.selfillum_energy,
        options.cull_disabled,
        options.max_texture_size,
    ].map(str)).md5_text()
    return model_path.path_join("%s_%s" % [material_path, options_hash])


func _refresh_managed_materials() -> void:
    var material_keys: Array = _managed_materials.keys()
    for material_key: String in material_keys:
        _refresh_managed_material(material_key)

func _refresh_managed_material(material_key: String) -> void:
    var managed_material: Dictionary = _managed_materials.get(material_key, {})
    if not managed_material:
        return
    var material_ref: WeakRef = managed_material.get("material_ref") as WeakRef
    var material: ShaderMaterial = material_ref.get_ref() as ShaderMaterial if material_ref else null
    if not material:
        _managed_materials.erase(material_key)
        return
    if not managed_material.get("has_variants", true):
        return
    var model_path: String = managed_material.get("model_path", "")
    var material_path: String = managed_material.get("material_path", "")
    var options:MaterialOptions = managed_material.get("options")
    var mmat: MaszynaMaterial = load_material(model_path, material_path)
    MaszynaMaterialFactory.apply(material, mmat, model_path, season, weather, options)
