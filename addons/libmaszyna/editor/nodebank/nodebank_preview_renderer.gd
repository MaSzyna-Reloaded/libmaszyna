@tool
extends Node
class_name NodebankPreviewRenderer

const CACHE_VERSION: String = "10"
const PREVIEW_SIZE: int = 192

var _cache: ResourceCache = ResourceCache.create("nodebank")
var _model_manager: Node
var _preview_material: StandardMaterial3D


func get_preview(item_data: NodebankGridItem) -> Texture2D:
    var source_path: String = _get_source_path(item_data)
    if not source_path or not FileAccess.file_exists(source_path):
        return null

    var cache_path: String = _make_cache_path(item_data)
    var cache_hash: String = _make_cache_hash(item_data, source_path)
    var cached_texture: Texture2D = _cache.get(cache_path, cache_hash) as Texture2D
    if cached_texture:
        return cached_texture

    var rendered_texture: Texture2D = await _render_preview(item_data)
    if not rendered_texture:
        return null

    _cache.set(cache_path, rendered_texture, cache_hash)
    cached_texture = _cache.get(cache_path, cache_hash) as Texture2D
    return cached_texture if cached_texture else rendered_texture


func _render_preview(item_data: NodebankGridItem) -> Texture2D:
    var instance = item_data.model.duplicate()
    var model = E3DModelManager.load_model(instance.data_path, instance.model_filename)
    instance.model = model

    var viewport: SubViewport = SubViewport.new()
    viewport.size = Vector2i(PREVIEW_SIZE, PREVIEW_SIZE)
    viewport.transparent_bg = true
    viewport.render_target_update_mode = SubViewport.UPDATE_ONCE
    viewport.own_world_3d = true
    add_child(viewport)

    var root: Node3D = Node3D.new()
    viewport.add_child(root)
    viewport.add_child(instance)

    var aabb: AABB = E3DModelTool.get_aabb(instance.model)

    if aabb.size.length_squared() <= 0.0:
        viewport.queue_free()
        return null

    var light: DirectionalLight3D = DirectionalLight3D.new()
    light.light_energy = 1.25
    light.rotation_degrees = Vector3(-45.0, 35.0, 0.0)
    viewport.add_child(light)

    var camera: Camera3D = Camera3D.new()
    viewport.add_child(camera)
    _fit_camera(camera, aabb)
    camera.current = true

    await RenderingServer.frame_pre_draw
    await RenderingServer.frame_post_draw
    await viewport.get_tree().process_frame


    var image: Image = viewport.get_texture().get_image()
    viewport.queue_free()


    if not image or image.get_width() <= 0 or image.get_height() <= 0:
        return null

    var texture: PortableCompressedTexture2D = PortableCompressedTexture2D.new()
    texture.create_from_image(image, PortableCompressedTexture2D.COMPRESSION_MODE_LOSSLESS)
    return texture


func _fit_camera(camera: Camera3D, aabb: AABB) -> void:
    var center: Vector3 = aabb.position + aabb.size * 0.5
    var radius: float = max(aabb.size.length() * 0.55, 0.5)
    var fov_radians: float = deg_to_rad(camera.fov)
    var distance: float = radius / tan(fov_radians * 0.5)
    var direction: Vector3 = Vector3(1.0, 0.65, 1.0).normalized()

    camera.position = center + direction * distance * 1.25
    camera.near = 0.01
    camera.far = max(distance * 4.0, 100.0)
    camera.look_at(center, Vector3.UP)


func _get_source_path(item_data: NodebankGridItem) -> String:
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_base_path:String = item_data.model.data_path.path_join(item_data.model.model_filename)
    var relative_path:String = MaszynaDataPath.resolve(game_dir, relative_base_path + ".e3d")
    var source_path:String = game_dir.path_join(relative_path)
    if FileAccess.file_exists(source_path):
        return source_path
    relative_path = MaszynaDataPath.resolve(game_dir, relative_base_path + ".t3d")
    return game_dir.path_join(relative_path)


func _make_cache_path(item_data: NodebankGridItem) -> String:
    return "previews/%s.res" % _make_cache_key(item_data)


func _make_cache_hash(item_data: NodebankGridItem, source_path: String) -> String:
    return "%s:%s:%s:%s:%s" % [
        CACHE_VERSION,
        E3DModel.FORMAT_VERSION,
        source_path,
        FileAccess.get_modified_time(source_path),
        JSON.stringify(item_data.model.skins),
    ]


func _make_cache_key(item_data: NodebankGridItem) -> String:
    var key_source: String = "%s:%s:%s" % [
        item_data.model.data_path,
        item_data.model.model_filename,
        JSON.stringify(item_data.model.skins),
    ]
    return key_source.md5_text()
