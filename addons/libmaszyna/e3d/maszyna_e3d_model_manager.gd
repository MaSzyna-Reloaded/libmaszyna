@tool
extends Node

var _cache = ResourceCache.create("e3d")


## MaszynaE3DRenderingServer loads the models of registered scenery placements through this, on the
## streaming's worker threads
func _ready() -> void:
    MaszynaE3DRenderingServer.model_set_loader(load_model)


func clear_cache():
    _cache.clear()

func _set_owner_recursive(node, new_owner):
    if not node == new_owner:
        node.owner = new_owner
    if node.get_child_count():
        for kid in node.get_children():
            _set_owner_recursive(kid, new_owner)

func _make_cache_path(data_path:String, filename:String, source_abs_path:String):
    return data_path.path_join(filename+".e3d.res")


func _make_cache_hash(source_abs_path:String) -> String:
    return ("%s:%s:%s" % [
        FileAccess.get_modified_time(source_abs_path),
        MaszynaE3DModel.FORMAT_VERSION,
        source_abs_path
    ]).md5_text()

func load_model(data_path:String, filename: String) -> MaszynaE3DModel:
    var output:MaszynaE3DModel
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var relative_base_path:String = data_path.trim_prefix("/").path_join(filename)
    # the binary model first, the text one when there is none (MdlMngr.cpp:144 find_on_disk)
    var relative_path:String = MaszynaDataPath.resolve(game_dir, relative_base_path + ".e3d")
    var path:String = game_dir.path_join(relative_path)
    if not FileAccess.file_exists(path):
        relative_path = MaszynaDataPath.resolve(game_dir, relative_base_path + ".t3d")
        var t3d_path:String = game_dir.path_join(relative_path)
        if FileAccess.file_exists(t3d_path):
            path = t3d_path

    # check users cache

    var cached_path = _make_cache_path(data_path, filename, path)
    var cache_hash = _make_cache_hash(path)

    output = _cache.get(cached_path, cache_hash) as MaszynaE3DModel
    if output:
        return output

    if FileAccess.file_exists(path):
        output = load(path) as MaszynaE3DModel # load external e3d
        if output:
            # saved on the thread that loaded it: an MaszynaE3DMesh keeps its arrays, so saving reads
            # nothing back from the RenderingServer (it was a GPU stall on the main thread)
            _cache.set(cached_path, output, cache_hash)
            return output
        else:
            push_warning("File is not an MaszynaE3DModel: "+path)
    else:
        # a vehicle or a scenery model without its model file is drawn without it; the original
        # only logs it (Model3d.cpp:1657)
        push_warning("File does not exist: %s" % path)
        MaszynaFileIndexer.missing_file_add(path, MaszynaFileIndexer.MASZYNA_FILE_MODEL)
    return output
