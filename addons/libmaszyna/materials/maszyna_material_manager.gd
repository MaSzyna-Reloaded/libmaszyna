@tool
extends Node

static var UNKNOWN_MATERIAL = preload("res://addons/libmaszyna/materials/unknown.tres")
static var UNKNOWN_TEXTURE = preload("res://addons/libmaszyna/materials/missing_texture.png")
const DDSTextureLoader = preload("res://addons/libmaszyna/materials/dds_texture_loader.gd")
## A texture file's default extension, looked for first in every place (Global.szDefaultExt)
const DEFAULT_TEXTURE_EXTENSION: String = ".dds"
## The extensions then looked for in each place, in the original's order (texture_manager::find_on_disk(),
## Texture.cpp:1503) - but .ktx and .tex, which Godot reads not and the game data has none of
const TEXTURE_EXTENSIONS: Array[String] = [".dds", ".tga", ".png", ".bmp", ".jpg"]
const COLORED_MATERIAL: Material = preload("res://addons/libmaszyna/e3d/colored.tres")
## A free spotlight drawn as a point of a constant size on the screen (types/free_spotlight.gdshader)
## and its glare (types/free_spotlight_glare.gdshader) as the next pass
const FREE_SPOTLIGHT_MATERIAL: ShaderMaterial = preload("types/free_spotlight.tres")
const FREE_SPOTLIGHT_GLARE_MATERIAL: ShaderMaterial = preload("types/free_spotlight_glare.tres")
## The original's glare texture (opengl33renderer.cpp:102)
const FREE_SPOTLIGHT_GLARE_TEXTURE: String = "fx/lightglare"
## Whether free spotlights - signals, lamps - get that point and glare, as the original draws them
const RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING: StringName = &"maszyna/scenery/railway_lights_visibility_improved"
const RAILWAY_LIGHTS_VISIBILITY_IMPROVED_DEFAULT: bool = true
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
## A key of the request asking for a set of data, not a value (dictionary.cpp:25): of them, the
## timetable of a vehicle's train
const DATASET_KEY_PREFIX: String = "$"
const TIMETABLE_KEY: String = "$timetable"
## The diffuse texture of the materials (MaszynaMaterialFactory)
const ALBEDO_TEXTURE_PARAMETER: StringName = &"texture_albedo"
## A texture of the game's own, not of the data (MaszynaMaterialPatch.textures)
const GAME_TEXTURE_SCHEME: String = "res://"

## The point with its glare, shared by every free spotlight; drawn at no size while
## RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING is off
var _free_spotlight_material: ShaderMaterial = null
var _managed_materials: Dictionary = {}
## The pictures of the cab screens that replace the textures of materials, by the materials' names
## (_texture_owner_key()) - material_replace_texture()
var _replaced_textures: Dictionary[String, Texture2D] = {}
## The PythonScreenServer screens drawing the generated textures, by their names - kept until the
## game's data is unloaded
var _generated_screens: Dictionary[String, RID] = {}
## The textures given names by whoever draws them (MaszynaTextureLuaNode), by their names in lower
## case - a skin, a material or a `make:` request names one as it names a file
var _named_textures: Dictionary[String, Texture2D] = {}
## The Lua scripts drawing the generated textures that have one (`<script>.lua`) - kept until the
## game's data is unloaded
var _generated_nodes: Array[MaszynaTextureLuaNode] = []
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

## A max texture size that takes the project's DDS_MAX_TEXTURE_SIZE_SETTING
const MAX_TEXTURE_SIZE_PROJECT_DEFAULT: int = 0
## The texture size limit of everything but the cab (the original's iMaxTextureSize, Globals.h:164,
## is 4096 - this one saves video memory)
const DDS_MAX_TEXTURE_SIZE_SETTING: StringName = &"maszyna/import/dds_max_texture_size"
const DDS_MAX_TEXTURE_SIZE_DEFAULT: int = 1024
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
    # the environment's snow, wet and standing water lie on it (types/weather.gdshaderinc): the
    # scenery's - its terrain, tracks and models - never a vehicle's, whose cab would fill with it
    var weather_exposed: bool = false
    # a glass inside the cab of its windows' texture: no rain on it (MaszynaMaterial.dry_submodels)
    var dry: bool = false


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
    material_options.weather_exposed = true
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
    _replaced_textures.clear()
    for screen:RID in _generated_screens.values():
        PythonScreenServer.screen_free(screen)
    _generated_screens.clear()
    for node:MaszynaTextureLuaNode in _generated_nodes:
        node.queue_free()
    _generated_nodes.clear()
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
    var improved: bool = ProjectSettings.get_setting(RAILWAY_LIGHTS_VISIBILITY_IMPROVED_SETTING)
    _free_spotlight_material.set_shader_parameter("point_size_multiplier", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_POINT_SIZE_MULTIPLIER_SETTING)) if improved else 0.0)
    var glare: ShaderMaterial = _free_spotlight_material.next_pass
    glare.set_shader_parameter("glare_intensity", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_INTENSITY_SETTING)) if improved else 0.0)
    glare.set_shader_parameter("glare_fade_start", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_FADE_START_SETTING)))
    glare.set_shader_parameter("glare_rotation_ratio", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_ROTATION_RATIO_SETTING)))
    glare.set_shader_parameter("glare_edge_size", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_EDGE_SIZE_SETTING)))
    glare.set_shader_parameter("glare_min_screen_size", float(ProjectSettings.get_setting(
        RAILWAY_LIGHTS_GLARE_MIN_SCREEN_SIZE_SETTING)))


func _apply_selfillum_energy() -> void:
    RenderingServer.global_shader_parameter_set(&"maszyna_selfillum_energy", float(ProjectSettings.get_setting(
        SELFILLUM_ENERGY_SETTING)))


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
            # named as the data names it: it is looked for in four places (_get_texture_file())
            MaszynaFileIndexer.missing_file_add(texture_name, MaszynaFileIndexer.MASZYNA_FILE_TEXTURE)
            continue
        # the game's own is a resource, loaded with the material - by its name: a game directory may
        # itself lie under res:// (the tests')
        if texture_name.begins_with(GAME_TEXTURE_SCHEME):
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
            image = _load_image(full_path, max_size)
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
        return int(ProjectSettings.get_setting(DDS_MAX_TEXTURE_SIZE_SETTING))
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
    var replaced: Texture2D = _replaced_textures.get(_texture_owner_key(_managed_materials[material_key]))
    if replaced:
        output.set_shader_parameter(ALBEDO_TEXTURE_PARAMETER, replaced)
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

    var path_segments: PackedStringArray = data_path.split("/")
    var unprefixed_model_path: String = "/".join(path_segments.slice(1))
    var options: MaterialOptions = MaterialOptions.new()
    # a vehicle's model is the game directory's dynamic/, its cab's too; the scenery's lie under the
    # weather. A vehicle's data path leads with a "/" (read_mmd_data()): its first segment is empty
    var root_dir: String = path_segments[1] if path_segments.size() > 1 and path_segments[0] == "" else path_segments[0]
    options.weather_exposed = not root_dir.to_lower() == MaszynaVehiclesBank.DYNAMIC_DIR

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
        options.dry = load_material(unprefixed_model_path, submodel.material_name).dry_submodels.has(
            submodel.resource_name.to_lower())
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
    var named: Texture2D = _named_textures.get(material_name.to_lower())
    if named:
        return named
    if material_name.begins_with(GENERATED_TEXTURE_SCHEME):
        return _load_generated_texture(material_name)
    var full_path:String = _get_texture_file(model_path, material_name)
    if not full_path:
        MaszynaFileIndexer.missing_file_add(material_name, MaszynaFileIndexer.MASZYNA_FILE_TEXTURE)
        return UNKNOWN_TEXTURE
    if material_name.begins_with(GAME_TEXTURE_SCHEME):
        return load(full_path) as Texture2D

    var texture:Texture2D = _load_clamped(full_path, _get_max_texture_size(max_texture_size))
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
    if not full_path:
        return false
    if _is_dds(full_path):
        return DDSTextureLoader.has_alpha(full_path)
    # any other file by its image's channels (data_components, Texture.cpp:358)
    var image:Image = Image.load_from_file(full_path)
    return image != null and not image.detect_alpha() == Image.ALPHA_NONE


## The size in pixels of a material's diffuse texture, read from a .dds's header or any other file's
## image; zero when there is no such file
func get_material_texture_size(material_name:String) -> Vector2i:
    var variant:MaszynaMaterial.MaszynaMaterialVariant = load_material("", material_name).default
    var texture_name:String = variant.get_texture_path("diffuse")
    if not texture_name:
        texture_name = variant.get_texture_path("tex1")
    var full_path:String = _get_texture_file("", texture_name if texture_name else material_name)
    if not full_path:
        return Vector2i.ZERO
    if _is_dds(full_path):
        return DDSTextureLoader.get_size(full_path)
    var image:Image = Image.load_from_file(full_path)
    return image.get_size() if image else Vector2i.ZERO


## The texture is the one of the name - before any file of it - in every material of the name, made
## already or later; a texture drawn for a skin, a material or a `make:` request
func named_texture_add(texture_name:String, texture:Texture2D) -> void:
    _named_textures[texture_name.to_lower()] = texture
    _apply_named_texture(texture_name)


## The materials of the name show their own texture again
func named_texture_remove(texture_name:String) -> void:
    _named_textures.erase(texture_name.to_lower())
    _apply_named_texture(texture_name)


## The managed materials of the name made again of their files - with the named texture, while it is
func _apply_named_texture(texture_name:String) -> void:
    for managed_material: Dictionary in _managed_materials.values():
        var managed: ShaderMaterial = (managed_material["material_ref"] as WeakRef).get_ref() as ShaderMaterial
        if managed and String(managed_material["material_path"]).to_lower() == texture_name.to_lower():
            var model_path: String = managed_material["model_path"]
            MaszynaMaterialFactory.apply(
                managed, load_material(model_path, managed_material["material_path"]), model_path, season,
                weather, managed_material["options"])


## The texture its script draws, once, with the query for its state: keys in lower case, values as
## written (make_request(), Texture.cpp:428; dictionary_source, dictionary.cpp:18). A script with a
## `.lua` file beside it is drawn by Lua - a MaszynaTextureLuaNode named after the request, given the
## vehicle `$timetable=` names and the query -, any other by Python, with the timetable of that vehicle's
## train (PythonScreenState.timetable_state()). Blank until drawn
func _load_generated_texture(texture_name:String) -> Texture2D:
    var screen:RID = _generated_screens.get(texture_name, RID())
    if screen.is_valid():
        return PythonScreenServer.screen_get_texture(screen)
    var request:PackedStringArray = texture_name.trim_prefix(GENERATED_TEXTURE_SCHEME).split("?", true, 1)
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var state:Dictionary = {}
    var vehicle:RID = RID()
    # a pair without "=" is its own key and value, as the original's substr() takes it; "$timetable="
    # names the vehicle whose train's timetable is drawn (dictionary.cpp:36-43)
    for pair:String in (request[1] if request.size() > 1 else "").split("&", false):
        var key_value:PackedStringArray = pair.split("=", true, 1)
        var key:String = key_value[0].to_lower()
        var value:String = key_value[key_value.size() - 1]
        if key == TIMETABLE_KEY:
            vehicle = VehicleServer.vehicle_get_rid_by_id(value.to_lower())
        elif not key.begins_with(DATASET_KEY_PREFIX):
            state[key] = value
    var lua_script:String = MaszynaDataPath.resolve(game_dir, request[0].to_lower() + MaszynaTextureLuaNode.EXTENSION)
    if FileAccess.file_exists(game_dir.path_join(lua_script)):
        var node: MaszynaTextureLuaNode = MaszynaTextureLuaNode.new()
        node.texture = DrawingTexture.new()
        # upside down, as the data's textures are (abstractscreenrenderer.py:69)
        node.texture.flip_y = true
        node.texture_name = texture_name
        node.set_vehicle(vehicle)
        node.parameters = state
        node.data_dir = game_dir
        node.script_path = game_dir.path_join(lua_script)
        _generated_nodes.append(node)
        add_child(node)
        return node.texture
    if vehicle.is_valid():
        state.merge(PythonScreenState.timetable_state(vehicle), true)
    var script:String = MaszynaDataPath.resolve(game_dir, request[0].to_lower() + ".py")
    screen = PythonScreenServer.screen_create(game_dir.path_join(script).trim_suffix(".py"), Callable())
    _generated_screens[texture_name] = screen
    PythonScreenServer.screen_request_render(screen, state)
    return PythonScreenServer.screen_get_texture(screen)


## The texture's file in the game directory, empty when there is none
func _get_texture_file(model_path:String, material_name:String) -> String:
    # a texture's traits follow its name after a colon and are no part of the file's name:
    # "peronowe/krawedz_zolty2:#" is krawedz_zolty2.dds (Texture.cpp:1310-1318)
    # the game's own texture (a patch's, MaszynaMaterialPatch): no file of the data
    if material_name.begins_with(GAME_TEXTURE_SCHEME):
        return material_name if ResourceLoader.exists(material_name) else ""
    var trait_position:int = material_name.rfind(":")
    if trait_position >= 0:
        material_name = material_name.left(trait_position)
    # nor the extension the data may give it: "loads/coal.bmp" is looked for as "loads/coal" in every
    # format (Texture.cpp:1321)
    material_name = MaszynaDataPath.erase_extension(material_name)
    var project_data_dir:String = MaszynaGameDirectory.get_game_dir()
    if (project_data_dir.ends_with("\\")):
        project_data_dir = project_data_dir.trim_suffix("\\")
    if (project_data_dir.ends_with("/")):
        project_data_dir = project_data_dir.trim_suffix("/")

    var possible_paths:Array[String] = [
        model_path.path_join(material_name),
        "textures".path_join(model_path.path_join(material_name)),
        material_name,
        "textures".path_join(material_name),
    ]
    # the default extension in every place first, then each place with every extension
    # (find_on_disk(), Texture.cpp:1482-1504; FileExists(), utilities.cpp:334-349)
    for p:String in possible_paths:
        var found:String = _find_data_file(project_data_dir, p + DEFAULT_TEXTURE_EXTENSION)
        if found:
            return found
    for p:String in possible_paths:
        for extension:String in TEXTURE_EXTENSIONS:
            var found:String = _find_data_file(project_data_dir, p + extension)
            if found:
                return found
    return ""


## The file of the game data's path, resolved as the data names it; empty when there is none
static func _find_data_file(data_dir:String, path:String) -> String:
    var resolved_path:String = data_dir.path_join(MaszynaDataPath.resolve(data_dir, path))
    return resolved_path if FileAccess.file_exists(resolved_path) else ""


## Loads a texture limited to max_size (_load_image()) - port of the original engine's
## iMaxTextureSize/maxtexturesize clamp (Texture.cpp), which this wrapper had no equivalent of. Cached by path+max_size since this bypasses Godot's own load() resource
## cache, and the same texture file is commonly referenced by several distinct materials
## (e.g. a shared normal map across dynamic skin slots).
## The image a preload read is taken while its build holds it; the file is read here only without one.
func _load_clamped(full_path:String, max_size:int) -> Texture2D:
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
        image = _load_image(full_path, max_size)
    if not image:
        return null
    texture = ImageTexture.create_from_image(image)
    _preload_mutex.lock()
    _dds_cache[cache_key] = weakref(texture)
    _preload_mutex.unlock()
    return texture


## The texture file's image, read on any thread: a .dds by DDSTextureLoader, its top mipmap levels
## dropped down to max_size; any other file by Godot's image loader - as the original reads them with
## stb_image (Texture.cpp:252-258) - halved down to max_size the same way, with its mipmaps made
static func _load_image(full_path:String, max_size:int) -> Image:
    if _is_dds(full_path):
        return DDSTextureLoader.load_image(full_path, max_size)
    var image:Image = Image.load_from_file(full_path)
    if not image:
        return null
    while (image.get_width() > max_size or image.get_height() > max_size) \
            and image.get_width() > 1 and image.get_height() > 1:
        image.resize(image.get_width() / 2, image.get_height() / 2)
    image.generate_mipmaps()
    return image


## Whether the texture file is a .dds, which DDSTextureLoader reads
static func _is_dds(full_path:String) -> bool:
    return full_path.get_extension().to_lower() == DEFAULT_TEXTURE_EXTENSION.trim_prefix(".")


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
        options.weather_exposed,
        options.dry,
    ].map(str)).md5_text()
    return model_path.path_join("%s_%s" % [material_path, options_hash])


## The diffuse texture of a material
func material_get_texture(material: ShaderMaterial) -> Texture2D:
    return material.get_shader_parameter(ALBEDO_TEXTURE_PARAMETER) as Texture2D


## A cab screen's picture replaces the diffuse texture of the material: in every material of its
## name, whatever its options - a self-illuminated variant too, so a timetable shows its picture
## with its backlight on (timetable_podsw) - as the original draws a screen into the texture itself
## (Train.cpp:10691, python_rt::shared_tex). Kept through the materials' refreshes, and given to the
## ones built later, until material_restore_texture()
func material_replace_texture(material: ShaderMaterial, texture: Texture2D) -> void:
    var material_key: String = _managed_material_key(material)
    if not material_key:
        push_error("MaszynaMaterialManager: a screen's texture on a material it does not manage")
        return
    var managed_material: Dictionary = _managed_materials[material_key]
    _replaced_textures[_texture_owner_key(managed_material)] = texture
    _apply_replaced_texture(managed_material)


## The materials of the material's name show their own texture again
func material_restore_texture(material: ShaderMaterial) -> void:
    var material_key: String = _managed_material_key(material)
    if not material_key:
        return
    var owner_key: String = _texture_owner_key(_managed_materials[material_key])
    _replaced_textures.erase(owner_key)
    for key: String in _managed_materials.keys():
        var managed_material: Dictionary = _managed_materials[key]
        var managed: ShaderMaterial = (managed_material["material_ref"] as WeakRef).get_ref() as ShaderMaterial
        if managed and _texture_owner_key(managed_material) == owner_key:
            var model_path: String = managed_material["model_path"]
            MaszynaMaterialFactory.apply(
                managed, load_material(model_path, managed_material["material_path"]), model_path, season,
                weather, managed_material["options"])


## The key a material is managed under; empty for one it does not manage
func _managed_material_key(material: ShaderMaterial) -> String:
    for material_key: String in _managed_materials:
        var material_ref: WeakRef = _managed_materials[material_key].get("material_ref") as WeakRef
        if material_ref and material_ref.get_ref() == material:
            return material_key
    return ""


## What a replaced texture belongs to: a material's name, whatever its options
func _texture_owner_key(managed_material: Dictionary) -> String:
    return String(managed_material["model_path"]).path_join(managed_material["material_path"])


## A screen's picture in every managed material of the name it replaces
func _apply_replaced_texture(replacing: Dictionary) -> void:
    var owner_key: String = _texture_owner_key(replacing)
    var texture: Texture2D = _replaced_textures[owner_key]
    for managed_material: Dictionary in _managed_materials.values():
        var managed: ShaderMaterial = (managed_material["material_ref"] as WeakRef).get_ref() as ShaderMaterial
        if managed and _texture_owner_key(managed_material) == owner_key:
            managed.set_shader_parameter(ALBEDO_TEXTURE_PARAMETER, texture)


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
    var replaced: Texture2D = _replaced_textures.get(_texture_owner_key(managed_material))
    if replaced:
        material.set_shader_parameter(ALBEDO_TEXTURE_PARAMETER, replaced)
