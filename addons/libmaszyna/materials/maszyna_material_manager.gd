@tool
extends Node

static var UNKNOWN_MATERIAL = preload("res://addons/libmaszyna/materials/unknown.tres")
static var UNKNOWN_TEXTURE = preload("res://addons/libmaszyna/materials/missing_texture.png")
const DDSTextureLoader = preload("res://addons/libmaszyna/materials/dds_texture_loader.gd")
const COLORED_MATERIAL: Material = preload("res://addons/libmaszyna/e3d/colored.tres")
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
## A texture a script draws, "make:<script>?<key>=<value>&..." (Texture.cpp:1294-1305)
const GENERATED_TEXTURE_SCHEME: String = "make:"

## The point with its glare, shared by every free spotlight; drawn at no size while
## RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING is off
var _free_spotlight_material: ShaderMaterial = null
var _managed_materials: Dictionary = {}
## The PythonScreenServer screens drawing the generated textures, by their names - kept until the
## game's data is unloaded
var _generated_screens: Dictionary[String, RID] = {}
## Weak, like _managed_materials: a texture goes, from RAM and VRAM, with the last material using it
var _dds_cache: Dictionary[String, WeakRef] = {}
## What preloads read on workers (material_preload()), by the material's and the texture's keys
## (_get_material_file_key(), _get_texture_key()): [the MaszynaMaterial or the Image, how many
## MaterialPreloads hold it]. Taken by get_material() and load_texture() on the main thread instead
## of the files; an entry goes with the last MaterialPreload holding it. Counted, not weak: a worker
## never takes an object back from a WeakRef the main thread may be letting go (FINDINGS.md, an
## object taken back from its ObjectID on another thread)
static var _preloaded: Dictionary[String, Array] = {}
## Guards _preloaded and _dds_cache's keys: the preloads read them on the streaming's workers
static var _preload_mutex: Mutex = Mutex.new()

enum Transparency { Disabled, Alpha, AlphaScissor }

## The files of materials read ahead on a worker - their parsed .mat and their textures' images. Held
## by the build that makes the materials of them; let go, the read data goes with it.
class MaterialPreload:
    var materials: Array[MaszynaMaterial] = []
    var images: Array[Image] = []
    ## The keys it holds in MaszynaMaterialManager._preloaded
    var keys: Array[String] = []

    func _notification(what: int) -> void:
        if what == NOTIFICATION_PREDELETE:
            MaszynaMaterialManager._release_preloaded(keys)

## A max texture size that takes the project's maszyna/import/dds_max_texture_size
const MAX_TEXTURE_SIZE_PROJECT_DEFAULT: int = 0
## A self-illumination threshold no light level reaches: lit always ("selfillum: true",
## Model3d.cpp:346)
const SELFILLUM_ALWAYS: float = 2.0
## How bright self-illumination is drawn, the shaders' global maszyna_selfillum_energy
const SELFILLUM_ENERGY_SETTING: StringName = &"maszyna/rendering/selfillum_energy"
const SELFILLUM_ENERGY_DEFAULT: float = 0.2


class MaterialOptions:
    var diffuse_color: Color = Color.WHITE
    var selfillum_color: Color = Color.WHITE
    var selfillum_energy: float = 1.0
    var selfillum_enabled: bool = false
    # the light level below which it self-illuminates (Global.fLuminance < fLight,
    # opengl33renderer.cpp:3452) - always lit by default, as the shaders' own
    var lights_on_threshold: float = SELFILLUM_ALWAYS
    # E3D translucent submodels are rendered in a separate alpha-blended pass.
    var force_transparent: bool = false
    # A submodel of the optimized E3D instancer, which never draws in the alpha pass: opaque
    # whatever its texture's alpha, not even cut out (MaszynaE3DRenderingServer.TRANSLUCENCY_OPAQUE)
    var force_opaque: bool = false
    var alpha_scissor_threshold: float = 0.5
    # Tracks draw both faces - the original disables culling for them in the shadow pass, "roads-based
    # platforms tend to miss parts of shadows" (opengl33renderer.cpp:3609), and an open rail profile
    # with its front faces culled casts almost nothing
    var cull_disabled: bool = false
    # the largest texture size loaded, larger DDS drop their top mipmaps - the cab has its own
    # limit in the original (Train.cpp:660)
    var max_texture_size: int = MAX_TEXTURE_SIZE_PROJECT_DEFAULT


@export var season := SimulationServer.SEASON_SUMMER:
    set(x):
        if not x == season:
            season = x
            _refresh_managed_materials()

@export var weather := SimulationServer.WEATHER_CLEAR:
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
    _apply_selfillum_energy()
    ProjectSettings.settings_changed.connect(_apply_selfillum_energy)
    MaszynaE3DRenderingServer.material_set_resolver(get_submodel_material)
    ReloadedMaterialServer.material_set_provider(_provide_material, get_material_texture_size, _provide_texture_length)
    MaszynaE3DRenderingServer.material_set_preloader(preload_model_materials)
    GameDataServer.cache_clear_requested.connect(clear_cache)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)
    SimulationServer.season_changed.connect(_on_season_changed)
    SimulationServer.weather_changed.connect(_on_weather_changed)
    _on_season_changed()
    _on_weather_changed()


func _exit_tree() -> void:
    SimulationServer.season_changed.disconnect(_on_season_changed)
    SimulationServer.weather_changed.disconnect(_on_weather_changed)
    ProjectSettings.settings_changed.disconnect(_apply_railway_lights_settings)
    ProjectSettings.settings_changed.disconnect(_apply_selfillum_energy)
    GameDataServer.cache_clear_requested.disconnect(clear_cache)
    GameDataServer.data_unload_requested.disconnect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)


## A material the world names (ReloadedMaterialServer.material_get())
func _provide_material(material_name: String, options: Dictionary) -> Material:
    var material_options: MaterialOptions = MaterialOptions.new()
    material_options.cull_disabled = bool(options.get("cull_disabled", false))
    return get_material("", material_name, material_options)


## The length its texture covers ("size:" of its .mat), -1 if it declares none
func _provide_texture_length(material_name: String) -> float:
    return load_material("", material_name).size.y


func _on_season_changed() -> void:
    season = SimulationServer.season


## Any precipitation switches the materials to their "rain" variant. It was blocked for a while as
## bad looking: the wet texture ("rain: { texture2: ... }") is a reflection map and was bound as a
## normal map back then (FINDINGS.md, "texture2: is not always the normal map").
func _on_weather_changed() -> void:
    weather = SimulationServer.WEATHER_RAIN if SimulationServer.precipitation > 0.0 else SimulationServer.weather


## Whatever was built of the game's data is built again by its owner and asks for its materials
## anew - none of them is handed out again
func _on_data_unload_requested() -> void:
    _managed_materials.clear()
    for screen:RID in _generated_screens.values():
        PythonScreenServer.screen_free(screen)
    _generated_screens.clear()
    _preload_mutex.lock()
    _dds_cache.clear()
    _preloaded.clear()
    _preload_mutex.unlock()


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


func _apply_selfillum_energy() -> void:
    RenderingServer.global_shader_parameter_set(&"maszyna_selfillum_energy", float(ProjectSettings.get_setting(
        SELFILLUM_ENERGY_SETTING, SELFILLUM_ENERGY_DEFAULT)))


func clear_cache() -> void:
    _preload_mutex.lock()
    _dds_cache.clear()
    _preload_mutex.unlock()
    _refresh_managed_materials()

## The parsed .mat - the one a preload read, while its build holds it, else read now
func load_material(model_path:String, material_name:String) -> MaszynaMaterial:
    var material:MaszynaMaterial = _get_preloaded(_get_material_file_key(model_path, material_name)) as MaszynaMaterial
    return material if material else MaszynaMaterialParser.parse(model_path, material_name)


## On a worker: the files of the materials of the model's submodels - found as
## get_submodel_material() finds them - read into what is returned, which the build holds while it
## makes the materials, so the main thread reads no file (docs/findings-archive.md, 2026-10-02
## streaming hitches). MaszynaE3DRenderingServer's material preloader.
func preload_model_materials(
    model:MaszynaE3DModel, data_path:String, skins:PackedStringArray, max_texture_size:int,
) -> MaterialPreload:
    var files:MaterialPreload = MaterialPreload.new()
    var unprefixed_model_path: String = "/".join(data_path.split("/").slice(1))
    var names:Dictionary[String, bool] = {}
    var submodels:Array[MaszynaE3DSubModel] = []
    submodels.assign(model.submodels)
    while submodels:
        var submodel:MaszynaE3DSubModel = submodels.pop_back()
        submodels.append_array(submodel.submodels)
        if submodel.dynamic_material and submodel.dynamic_material_index < skins.size():
            names[skins[submodel.dynamic_material_index]] = true
        elif submodel.material_name and not submodel.material_colored:
            names[submodel.material_name] = true
    for material_name:String in names:
        material_preload(unprefixed_model_path, material_name, max_texture_size, files)
    return files


## On a worker: the material's .mat parsed and the textures of its variant for the season and the
## weather read as images, into `files`. A season or a weather changed meanwhile only leaves the
## main thread reading what the new variant names
func material_preload(
    model_path:String, material_name:String, max_texture_size:int, files:MaterialPreload,
) -> void:
    var material_key:String = _get_material_file_key(model_path, material_name)
    var material:MaszynaMaterial = _hold_preloaded(material_key, files) as MaszynaMaterial
    if not material:
        material = _put_preloaded(
            material_key, MaszynaMaterialParser.parse(model_path, material_name), files) as MaszynaMaterial
    files.materials.append(material)
    var max_size:int = _get_max_texture_size(max_texture_size)
    for texture_name:String in material.get_variant(season, weather).get_texture_paths():
        var full_path:String = _get_texture_file(model_path, texture_name)
        if not full_path:
            continue
        var texture_key:String = _get_texture_key(full_path, max_size)
        # a texture made already is not read again, nor one another preload holds
        _preload_mutex.lock()
        var resident:bool = _dds_cache.has(texture_key)
        _preload_mutex.unlock()
        if resident:
            continue
        var image:Image = _hold_preloaded(texture_key, files) as Image
        if not image:
            image = DDSTextureLoader.load_image(full_path, max_size)
            if not image:
                continue
            image = _put_preloaded(texture_key, image, files) as Image
        files.images.append(image)


## The preloaded value of the key, held by `files` as well; null when no preload holds it
static func _hold_preloaded(key:String, files:MaterialPreload) -> RefCounted:
    _preload_mutex.lock()
    var entry:Array = _preloaded.get(key, [])
    if entry:
        entry[1] += 1
        files.keys.append(key)
    _preload_mutex.unlock()
    return entry[0] if entry else null


## The value put under the key, held by `files` - or the one another preload put first
static func _put_preloaded(key:String, value:RefCounted, files:MaterialPreload) -> RefCounted:
    _preload_mutex.lock()
    var entry:Array = _preloaded.get(key, [])
    if entry:
        entry[1] += 1
    else:
        entry = [value, 1]
        _preloaded[key] = entry
    files.keys.append(key)
    _preload_mutex.unlock()
    return entry[0]


## A MaterialPreload let go: its keys' entries go with their last holder
static func _release_preloaded(keys:Array[String]) -> void:
    _preload_mutex.lock()
    for key:String in keys:
        var entry:Array = _preloaded.get(key, [])
        if not entry:
            continue
        entry[1] -= 1
        if entry[1] <= 0:
            _preloaded.erase(key)
    _preload_mutex.unlock()


static func _get_preloaded(key:String) -> RefCounted:
    _preload_mutex.lock()
    var entry:Array = _preloaded.get(key, [])
    _preload_mutex.unlock()
    return entry[0] if entry else null


func _get_material_file_key(model_path:String, material_name:String) -> String:
    return "%s|%s|%s" % [MaszynaGameDirectory.get_game_dir(), model_path, material_name]


func _get_texture_key(full_path:String, max_size:int) -> String:
    return "%s:%d" % [full_path, max_size]


## The texture size limit: MAX_TEXTURE_SIZE_PROJECT_DEFAULT takes the project's
func _get_max_texture_size(max_texture_size:int) -> int:
    if max_texture_size == MAX_TEXTURE_SIZE_PROJECT_DEFAULT:
        return int(ProjectSettings.get_setting("maszyna/import/dds_max_texture_size", 1024))
    return max_texture_size

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

## Material override of an E3D submodel - the material resolver of [MaszynaE3DRenderingServer].
## The first segment of [param data_path] is dropped from the material search path
## (see maszyna_rail_vehicle_3d_instancer.gd's _build_structure()).
func get_submodel_material(
    submodel: MaszynaE3DSubModel,
    data_path: String,
    skins: PackedStringArray,
    translucency: MaszynaE3DRenderingServer.Translucency,
    max_texture_size: int,
) -> Material:
    # MaszynaE3DOptimizedBackend draws a free spotlight only when it gets a material for it
    if submodel.submodel_type == MaszynaE3DSubModel.SUBMODEL_FREE_SPOTLIGHT:
        return _free_spotlight_material

    var unprefixed_model_path: String = "/".join(data_path.split("/").slice(1))
    var options: MaterialOptions = MaterialOptions.new()

    # TODO: handle more material options here (selfillum, etc)
    options.force_transparent = translucency == MaszynaE3DRenderingServer.TRANSLUCENCY_BLENDED
    options.force_opaque = translucency == MaszynaE3DRenderingServer.TRANSLUCENCY_OPAQUE
    options.max_texture_size = max_texture_size
    options.diffuse_color = submodel.diffuse_color
    options.selfillum_color = (
        submodel.self_illumination
        if submodel.self_illumination and not submodel.self_illumination == Color.BLACK
        else Color.WHITE
    )
    options.selfillum_energy = options.selfillum_color.a  # legacy renderer
    # self-illuminated below its threshold (the shaders' lights_on_threshold, opengl33renderer.cpp:3452);
    # none at 0 or below - never darker than that outside
    options.selfillum_enabled = options.selfillum_energy > 0.0 and submodel.lights_on_threshold > 0.0
    if options.selfillum_enabled:
        options.lights_on_threshold = submodel.lights_on_threshold

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
    if material_name.begins_with(GENERATED_TEXTURE_SCHEME):
        return _load_generated_texture(material_name)
    var full_path:String = _get_texture_file(model_path, material_name)
    if not full_path:
        return UNKNOWN_TEXTURE

    var texture:Texture2D = _load_dds_clamped(full_path, _get_max_texture_size(max_texture_size))
    if not texture:
        texture = load(full_path) as Texture2D
    if texture:
        return texture
    return UNKNOWN_TEXTURE


## Whether a scenery shape with this material is translucent - the original's legacy "problend":
## its diffuse texture has "@" in its name and an alpha channel (scenenode.cpp:221-227). Asked by
## MaszynaTrianglesSink to merge the shape as the original does (MASZYNA_ORIGINAL_QUIRKS.md,
## "Scenery shapes"); called on the parse's workers.
func is_material_translucent(material_name:String) -> bool:
    var variant:MaszynaMaterial.MaszynaMaterialVariant = load_material("", material_name).default
    var texture_name:String = variant.get_texture_path("diffuse")
    if not texture_name:
        texture_name = variant.get_texture_path("tex1")
    if not ("@" in texture_name):
        return false
    var full_path:String = _get_texture_file("", texture_name)
    return full_path and DDSTextureLoader.has_alpha(full_path)


## The size in pixels of a material's diffuse texture, read from its file's header; zero when
## there is no such file or it is not a .dds
func get_material_texture_size(material_name:String) -> Vector2i:
    var variant:MaszynaMaterial.MaszynaMaterialVariant = load_material("", material_name).default
    var texture_name:String = variant.get_texture_path("diffuse")
    if not texture_name:
        texture_name = variant.get_texture_path("tex1")
    var full_path:String = _get_texture_file("", texture_name if texture_name else material_name)
    return DDSTextureLoader.get_size(full_path) if full_path else Vector2i.ZERO


## The texture its script draws, once, with the query for its state: keys in lower case, values as
## written (make_request(), Texture.cpp:428; dictionary_source, dictionary.cpp:18). Blank until drawn
func _load_generated_texture(texture_name:String) -> Texture2D:
    var screen:RID = _generated_screens.get(texture_name, RID())
    if not screen.is_valid():
        var request:PackedStringArray = texture_name.trim_prefix(GENERATED_TEXTURE_SCHEME).split("?", true, 1)
        var game_dir:String = MaszynaGameDirectory.get_game_dir()
        var script:String = MaszynaDataPath.resolve(game_dir, request[0].to_lower() + ".py")
        screen = PythonScreenServer.screen_create(game_dir.path_join(script).trim_suffix(".py"), Callable())
        _generated_screens[texture_name] = screen
        var state:Dictionary = {}
        # a pair without "=" is its own key and value, as the original's substr() takes it
        for pair:String in (request[1] if request.size() > 1 else "").split("&", false):
            var key_value:PackedStringArray = pair.split("=", true, 1)
            state[key_value[0].to_lower()] = key_value[key_value.size() - 1]
        PythonScreenServer.screen_request_render(screen, state)
    return PythonScreenServer.screen_get_texture(screen)


## The texture's file in the game directory, empty when there is none
func _get_texture_file(model_path:String, material_name:String) -> String:
    # a texture's traits follow its name after a colon and are no part of the file's name:
    # "peronowe/krawedz_zolty2:#" is krawedz_zolty2.dds (Texture.cpp:1310-1318)
    var trait_position:int = material_name.rfind(":")
    if trait_position >= 0:
        material_name = material_name.left(trait_position)
    var project_data_dir:String = MaszynaGameDirectory.get_game_dir()
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

    for p:String in possible_paths:
        var resolved_path:String = MaszynaDataPath.resolve(project_data_dir, p)
        if FileAccess.file_exists(project_data_dir.path_join(resolved_path)):
            return project_data_dir.path_join(resolved_path)
    return ""


## Loads a .dds with its top mipmap levels discarded down to max_size - port of the original
## engine's iMaxTextureSize/maxtexturesize clamp (Texture.cpp), which this wrapper had no
## equivalent of. Cached by path+max_size since this bypasses Godot's own load() resource
## cache, and the same texture file is commonly referenced by several distinct materials
## (e.g. a shared normal map across dynamic skin slots).
## The image a preload read is taken while its build holds it; the file is read here only without one.
func _load_dds_clamped(full_path:String, max_size:int) -> Texture2D:
    var cache_key:String = _get_texture_key(full_path, max_size)
    _preload_mutex.lock()
    var cached:WeakRef = _dds_cache.get(cache_key)
    _preload_mutex.unlock()
    # on the main thread, which lets textures go: taken back from the WeakRef safely here
    var texture:Texture2D = cached.get_ref() as Texture2D if cached else null
    if texture:
        return texture
    if cached:
        # gone: a preload reads its file again (material_preload())
        _preload_mutex.lock()
        _dds_cache.erase(cache_key)
        _preload_mutex.unlock()
    var image:Image = _get_preloaded(cache_key) as Image
    if not image:
        image = DDSTextureLoader.load_image(full_path, max_size)
    if not image:
        return null
    texture = ImageTexture.create_from_image(image)
    _preload_mutex.lock()
    _dds_cache[cache_key] = weakref(texture)
    _preload_mutex.unlock()
    return texture


## The material handed out for a path with these options, the same while it is alive
func _material_key(
    model_path: String,
    material_path: String,
    options: MaterialOptions,
) -> String:
    # the same material path is another material in another game directory
    var options_hash = ":".join([
        MaszynaGameDirectory.get_game_dir(),
        options.force_transparent,
        options.force_opaque,
        options.diffuse_color.to_html(true),
        options.alpha_scissor_threshold,
        options.selfillum_enabled,
        options.selfillum_color.to_html(true),
        options.selfillum_energy,
        options.lights_on_threshold,
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
