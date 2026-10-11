@tool
extends Node

var _cache = ResourceCache.create("e3d")


## E3DRenderingServer loads the models of registered scenery placements through this, on the
## streaming's worker threads
func _ready() -> void:
    E3DRenderingServer.model_set_loader(load_model)


## Saving a model reads its meshes back from the RenderingServer, which off the main thread waits
## for the main thread to flush its commands - a worker waiting there deadlocked the load
## (docs/findings-archive.md, 2026-10-02). So a worker hands the save to the main thread - by a
## deferred call: a signal of a node in the tree cannot even be emitted off the main thread.
func _save_model(cached_path:String, model:E3DModel, cache_hash:String) -> void:
    _cache.set(cached_path, model, cache_hash)


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
        E3DModel.FORMAT_VERSION,
        source_abs_path
    ]).md5_text()

func load_model(data_path:String, filename: String) -> E3DModel:
    var output:E3DModel
    var game_dir:String = UserSettings.get_maszyna_game_dir()
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

    output = _cache.get(cached_path, cache_hash) as E3DModel
    if output:
        return output

    if FileAccess.file_exists(path):
        output = load(path) as E3DModel # load external e3d
        if output:
            if not OS.get_thread_caller_id() == OS.get_main_thread_id():
                _save_model.call_deferred(cached_path, output, cache_hash)
                return output
            _cache.set(cached_path, output, cache_hash)
            return _cache.get(cached_path)  # force use proper resource ref
        else:
            push_warning("File is not an E3DModel: "+path)
    else:
        # a vehicle or a scenery model without its model file is drawn without it; the original
        # only logs it (Model3d.cpp:1657)
        push_warning("File does not exist: %s" % path)
    return output
