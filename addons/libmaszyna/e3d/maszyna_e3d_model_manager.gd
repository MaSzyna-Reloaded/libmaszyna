@tool
extends Node

## The models of the game's data, read apart as their description and their geometry
## (MaszynaE3DModelReader), and kept on disk apart. What a vehicle's data needs of its model is its
## description alone (load_model_description()): nothing of it goes to the GPU. A model to draw
## (load_model()) gets its geometry applied - its meshes in the GPU - and the geometry is let go: an
## ArrayMesh keeps nothing in memory, so neither does the model. The geometry is written from the
## parse's arrays, so writing it reads nothing back from the GPU.

var _cache = ResourceCache.create("e3d")
var _geometry_cache = ResourceCache.create("e3d_geometry")


## MaszynaE3DRenderingServer loads the models of registered scenery placements through this, on the
## streaming's worker threads
func _ready() -> void:
    MaszynaE3DRenderingServer.model_set_loader(load_model)


func clear_cache():
    _cache.clear()
    _geometry_cache.clear()

func _set_owner_recursive(node, new_owner):
    if not node == new_owner:
        node.owner = new_owner
    if node.get_child_count():
        for kid in node.get_children():
            _set_owner_recursive(kid, new_owner)


## The model file of a name: the binary model first, the text one when there is none
## (MdlMngr.cpp:144 find_on_disk); the binary one's path when neither is there
func get_model_path(data_path:String, filename:String) -> String:
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var relative_base_path:String = data_path.trim_prefix("/").path_join(filename)
    var path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_base_path + ".e3d"))
    if not FileAccess.file_exists(path):
        var t3d_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_base_path + ".t3d"))
        if FileAccess.file_exists(t3d_path):
            path = t3d_path
    return path


func _make_cache_hash(source_abs_path:String, format_version:int) -> String:
    return ("%s:%s:%s" % [
        FileAccess.get_modified_time(source_abs_path),
        format_version,
        source_abs_path
    ]).md5_text()


## The model's description - its submodels, their names, transforms and skins, its lights - without
## its geometry; null without its file
func load_model_description(data_path:String, filename:String) -> MaszynaE3DModel:
    var path:String = get_model_path(data_path, filename)
    if not FileAccess.file_exists(path):
        # a vehicle or a scenery model without its model file is drawn without it; the original
        # only logs it (Model3d.cpp:1657)
        push_warning("File does not exist: %s" % path)
        MaszynaFileIndexer.missing_file_add(path, MaszynaFileIndexer.MASZYNA_FILE_MODEL)
        return null
    var cached_path:String = data_path.path_join(filename + ".e3d.res")
    var cache_hash:String = _make_cache_hash(path, MaszynaE3DModel.FORMAT_VERSION)
    var description:MaszynaE3DModel = _cache.get(cached_path, cache_hash) as MaszynaE3DModel
    if description:
        return description
    description = MaszynaE3DModelReader.read_description(path)
    if not description:
        push_warning("File is not an MaszynaE3DModel: " + path)
        return null
    _cache.set(cached_path, description, cache_hash)
    return description


## The model to draw: its description with its geometry applied, once - one model a file, shared by
## everyone who holds it; null without its file
func load_model(data_path:String, filename:String) -> MaszynaE3DModel:
    var model:MaszynaE3DModel = load_model_description(data_path, filename)
    if not model or model.has_geometry():
        return model
    var path:String = get_model_path(data_path, filename)
    var cached_path:String = data_path.path_join(filename + ".e3d_geometry.res")
    var cache_hash:String = _make_cache_hash(path, MaszynaE3DModelGeometry.FORMAT_VERSION)
    var geometry:MaszynaE3DModelGeometry = _geometry_cache.get(cached_path, cache_hash) as MaszynaE3DModelGeometry
    if not geometry:
        geometry = MaszynaE3DModelReader.read_geometry(path)
        if not geometry:
            push_warning("File is not an MaszynaE3DModel: " + path)
            return model
        _geometry_cache.set(cached_path, geometry, cache_hash)
    model.apply_geometry(geometry)
    return model
