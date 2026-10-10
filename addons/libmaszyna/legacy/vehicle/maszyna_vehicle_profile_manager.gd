@tool
extends Node

## Side views ("profiles") of vehicles and their skins, rendered from the vehicle models. The
## original starter has its own images in textures/mini, but they cover only a fraction of the
## skins (e.g. dynamic/pkp/4e_v1 has three for over forty) and look nothing like the game.
##
## Rendered profiles go into a ResourceCache, so a skin is only ever rendered once. Reading and
## writing that cache and loading the vehicle model run on WorkerTaskQueue workers; the
## render itself has to stay on the main thread, a SubViewport is only drawn with the frame.

## Every profile has the same scale and the same band above the rail, so profiles shown at one
## height stand on one rail and in one scale; its centre is the vehicle's centre, so the vehicles
## of a trainset are laid get_profile_coupling_width() apart, as they stand on the track
const PROFILE_PIXELS_PER_METRE:float = 16.0
## The band a profile shows, from below the rail (the wheels) to above the highest roof and a
## lowered pantograph [m]; a vehicle's origin lies on the rail level
const PROFILE_BOTTOM:float = -0.3
const PROFILE_TOP:float = 5.0
## Bump to re-render the cached profiles after changing how they are rendered
const PROFILE_VERSION:int = 9
const CACHE_DIRECTORY:String = "vehicle_profiles"
## Profiles kept in memory; past it the least recently used go - read back from the disk cache
## when asked again. A game directory holds thousands of vehicles and skins; a profile is at most
## 85 px high and a 27 m car ~430 px long in RGBA8 (~150 KB), so this is ~130 MB at most.
const PROFILE_MEMORY_LIMIT:int = 900

var _cache:ResourceCache = ResourceCache.create(CACHE_DIRECTORY)
## The profiles in memory, least recently used first (a Dictionary keeps its insertion order)
var _profiles:Dictionary[String, Texture2D] = {}
var _viewport:SubViewport
var _environment:Environment
var _camera:Camera3D
var _model_root:Node3D
var _model:E3DModelInstance
## The attachments of the vehicle in the render viewport, children of _model
var _attachments:Array[E3DModelInstance] = []
## data_path and models of the vehicle currently in the render viewport
var _model_key:String = ""
var _rendering:bool = false
var _queue:WorkerTaskQueue = WorkerTaskQueue.new()


func _ready() -> void:
    GameDataServer.cache_clear_requested.connect(clear_cache)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)


func _exit_tree() -> void:
    GameDataServer.cache_clear_requested.disconnect(clear_cache)
    GameDataServer.data_unload_requested.disconnect(_on_data_unload_requested)


## A profile is rendered again, from the model read again, when it is next asked for
func _on_data_unload_requested() -> void:
    _profiles.clear()
    # at once: queued, it would still read its model again with everything else
    if _model:
        _model.free()
        _model = null
        _attachments.clear()


func clear_cache() -> void:
    _cache.clear()
    _profiles.clear()


## Side view of the vehicle with the given skin, null when its model cannot be read.
## Await it: a profile missing from the caches is rendered, which takes a frame.
func get_profile(data_path:String, file_name:String, skin:String, vehicle_name:String) -> Texture2D:
    # callers spell the path with or without the leading slash, it is the same vehicle
    var key:String = "%s:%s" % [data_path.to_lower().trim_prefix("/"), skin.to_lower()]
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_path:String = data_path.trim_prefix("/").path_join(file_name + ".mmd")
    var abs_mmd_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_path))
    if MmdCabinInstancer.names_vehicle(abs_mmd_path):
        key += ":" + vehicle_name
    if _profiles.has(key):
        var kept:Texture2D = _profiles[key]
        _remember_profile(key, kept)
        return kept

    var cache_path:String = _get_cache_path(key)
    var cached:Texture2D = await _run_in_queue(_cache.get.bind(cache_path)) as Texture2D
    if cached:
        _remember_profile(key, cached)
        return cached

    var rendered:Texture2D = await _render_profile(data_path, abs_mmd_path, file_name, skin, vehicle_name)
    if rendered:
        _remember_profile(key, rendered)
        _queue.submit(_cache.set.bind(cache_path, rendered, ""))
    return rendered


## Keeps the profile in memory as the most recently used, letting go of the least recently used
## one past PROFILE_MEMORY_LIMIT
func _remember_profile(key:String, texture:Texture2D) -> void:
    _profiles.erase(key)
    _profiles[key] = texture
    if _profiles.size() > PROFILE_MEMORY_LIMIT:
        _profiles.erase(_profiles.keys()[0])


## The pixels of a profile the vehicle takes in a trainset: its length over the buffers, the one
## its FIZ gives and the vehicles are coupled at (DynObj.cpp). The side view may be longer - the
## shared bogies of an articulated unit overhang its neighbours. 0 when there is no length.
func get_profile_coupling_width(data_path:String, file_name:String) -> float:
    var description:VehicleController = FizVehicleBuilder.build_description(data_path.trim_prefix("/"), file_name)
    return description.dimensions_length * PROFILE_PIXELS_PER_METRE if description else 0.0


## Runs task on a queue worker while the main thread keeps drawing
func _run_in_queue(task:Callable) -> Variant:
    var task_id:int = _queue.submit(task)
    while not _queue.is_done(task_id):
        await get_tree().process_frame
    return _queue.wait(task_id)


## One render at a time - the viewport holds a single model, only its skin changes between shots
func _render_profile(
        data_path:String, abs_mmd_path:String, file_name:String, skin:String, vehicle_name:String) -> Texture2D:
    while _rendering:
        await get_tree().process_frame
    _rendering = true
    var texture:Texture2D = null
    if await _build_model(data_path, abs_mmd_path, file_name, skin, vehicle_name):
        # the vehicle over black and over white gives its real transparency, whatever the
        # viewport does with its own background
        var over_black:Image = await _capture(Color.BLACK)
        var over_white:Image = await _capture(Color.WHITE)
        var image:Image = _extract_alpha(over_black, over_white)
        # trim the empty space before and after the vehicle by as much on both sides, so the
        # vehicle's centre stays the profile's; the height stays the band, so does the rail
        var used:Rect2i = image.get_used_rect()
        var trim:int = mini(used.position.x, image.get_width() - used.end.x)
        if used.size.x > 0:
            image = image.get_region(Rect2i(trim, 0, image.get_width() - 2 * trim, image.get_height()))
        texture = ImageTexture.create_from_image(image)
    _rendering = false
    return texture


func _capture(background:Color) -> Image:
    _environment.background_color = background
    _viewport.render_target_update_mode = SubViewport.UPDATE_ONCE
    await RenderingServer.frame_post_draw
    return _viewport.get_texture().get_image()


## What is opaque looks the same over both backgrounds, what is not lets the background through -
## the difference is the transparency (classic two-pass alpha, the render itself has none)
static func _extract_alpha(over_black:Image, over_white:Image) -> Image:
    var size:Vector2i = over_black.get_size()
    var image:Image = Image.create_empty(size.x, size.y, false, Image.FORMAT_RGBA8)
    for y:int in size.y:
        for x:int in size.x:
            var black:Color = over_black.get_pixel(x, y)
            var white:Color = over_white.get_pixel(x, y)
            var alpha:float = 1.0 - clampf(
                ((white.r - black.r) + (white.g - black.g) + (white.b - black.b)) / 3.0, 0.0, 1.0
            )
            if alpha <= 0.004:
                continue
            # over black the colour is already multiplied by its own alpha
            image.set_pixel(x, y, Color(black.r / alpha, black.g / alpha, black.b / alpha, alpha))
    return image


## The vehicle's exterior model and its attachments with the given skin, seen from the side by an
## orthogonal camera. Only the skin changes between the profiles of one vehicle.
func _build_model(
        data_path:String, abs_mmd_path:String, file_name:String, skin:String, vehicle_name:String) -> bool:
    # every vehicle path is used with a leading slash (see maszyna_rail_vehicle_3d_instancer.gd)
    var normalized_data_path:String = data_path if data_path.begins_with("/") else "/" + data_path
    var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(vehicle_name, file_name, skin)
    var body_model:String = MmdCabinInstancer.parse_body_model(abs_mmd_path, parameters)
    if not body_model:
        body_model = file_name
    var attachments:PackedStringArray = MmdCabinInstancer.parse_attachments(abs_mmd_path, parameters)

    var skins:Array = MmdCabinInstancer.resolve_skins(normalized_data_path, skin)
    var model_key:String = "%s:%s:%s" % [normalized_data_path, body_model, "|".join(attachments)]
    if _model and _model_key == model_key:
        _model.skins = skins
        _model.reload()
        for attachment:E3DModelInstance in _attachments:
            attachment.skins = skins
            attachment.reload()
        return true

    # the E3Ds are read on a worker, only building the instances needs the main thread
    var e3d_model:E3DModel = (
        await _run_in_queue(E3DModelManager.load_model.bind(normalized_data_path, body_model)) as E3DModel
    )
    if not e3d_model:
        return false
    var attachment_models:Array[E3DModel] = []
    for attachment:String in attachments:
        var attachment_model:E3DModel = (
            await _run_in_queue(E3DModelManager.load_model.bind(normalized_data_path, attachment)) as E3DModel
        )
        if attachment_model:
            attachment_models.append(attachment_model)

    _ensure_viewport()
    if _model:
        _model.queue_free()
    _model = _profile_instance(normalized_data_path, e3d_model, skins)
    _attachments.clear()
    # the attachments are drawn in the exterior's frame (DynObj.cpp:5384), so they move with it
    for attachment_model:E3DModel in attachment_models:
        var attachment:E3DModelInstance = _profile_instance(normalized_data_path, attachment_model, skins)
        _model.add_child(attachment)
        _attachments.append(attachment)
    _model_root.add_child(_model)
    _model_key = model_key

    var bounds:AABB = _model.submodels_aabb
    for attachment:E3DModelInstance in _attachments:
        bounds = bounds.merge(attachment.submodels_aabb)
    if not bounds.size.length() > 0.0:
        return false
    # centred across the track, but left on its rail and on its own centre along it
    _model.position = Vector3(-bounds.get_center().x, 0.0, 0.0)
    # the vehicle's longer half on both sides at the common scale; an orthogonal camera sizes its
    # view by height, the width follows the viewport's
    var half_length:float = maxf(-bounds.position.z, bounds.end.z)
    _viewport.size = Vector2i(
        ceili(maxf(2.0 * half_length, 1.0) * PROFILE_PIXELS_PER_METRE),
        ceili((PROFILE_TOP - PROFILE_BOTTOM) * PROFILE_PIXELS_PER_METRE)
    )
    _camera.size = float(_viewport.size.y) / PROFILE_PIXELS_PER_METRE
    # the vehicle runs along Z in the model frame, the camera looks at it down -X
    var band_centre:Vector3 = Vector3(0.0, PROFILE_BOTTOM + _camera.size * 0.5, 0.0)
    _camera.position = band_centre + Vector3(maxf(bounds.size.z, 1.0), 0.0, 0.0)
    _camera.look_at(band_centre)
    return true


func _profile_instance(data_path:String, e3d_model:E3DModel, skins:Array) -> E3DModelInstance:
    var instance := E3DModelInstance.new()
    instance.instancer = E3DModelInstance.Instancer.OPTIMIZED
    instance.instance_kind = E3DRenderingServer.INSTANCE_KIND_DYNAMIC
    instance.data_path = data_path
    instance.model = e3d_model
    # before entering the tree: the instance builds its materials there, an empty skin list
    # makes every submodel with a dynamic material complain
    instance.skins = skins
    return instance


func _ensure_viewport() -> void:
    if _viewport:
        return
    _viewport = SubViewport.new()
    _viewport.own_world_3d = true
    _viewport.transparent_bg = true
    _viewport.msaa_3d = Viewport.MSAA_4X
    _viewport.render_target_update_mode = SubViewport.UPDATE_DISABLED
    add_child(_viewport)

    _environment = Environment.new()
    # a flat background of a known colour: the profile is cut out of two of them
    _environment.background_mode = Environment.BG_COLOR
    _environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    _environment.ambient_light_color = Color(0.45, 0.55, 0.72)
    _environment.ambient_light_energy = 0.7
    # the two passes are compared channel by channel, a tonemap would bend that comparison
    _environment.tonemap_mode = Environment.TONE_MAPPER_LINEAR
    var world_environment := WorldEnvironment.new()
    world_environment.environment = _environment
    _viewport.add_child(world_environment)

    var key_light := DirectionalLight3D.new()
    key_light.rotation_degrees = Vector3(-35.0, 40.0, 0.0)
    key_light.light_energy = 1.6
    _viewport.add_child(key_light)
    var fill_light := DirectionalLight3D.new()
    fill_light.rotation_degrees = Vector3(-20.0, -140.0, 0.0)
    fill_light.light_color = Color(0.72, 0.8, 1.0)
    fill_light.light_energy = 0.6
    _viewport.add_child(fill_light)

    _model_root = Node3D.new()
    _viewport.add_child(_model_root)
    _camera = Camera3D.new()
    _camera.projection = Camera3D.PROJECTION_ORTHOGONAL
    _viewport.add_child(_camera)


## The same vehicle is another vehicle in another game directory
static func _get_cache_path(key:String) -> String:
    return ("%s:%s:%d:%d" % [
        UserSettings.get_maszyna_game_dir(), key, PROFILE_VERSION, E3DModel.FORMAT_VERSION
    ]).md5_text() + ".res"
