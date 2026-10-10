@tool
extends Node

const EMPTY_NODEBANK = preload("./empty_nodebank.tscn")
const CACHE_FILE: String = "nodebank.tscn"
const CACHE_VERSION: String = "4"
const STOP_TOKENS: Array = [" ", ";", char(9), char(10), char(13)]

var _cache: ResourceCache = ResourceCache.create("nodebank")


func load_library(force_rebuild: bool = false) -> PackedScene:
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var nodebank_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, "nodebank.txt"))
    if not FileAccess.file_exists(nodebank_path):
        push_warning("Nodebank file does not exist: %s" % nodebank_path)
        return EMPTY_NODEBANK

    var cache_hash: String = _make_cache_hash(nodebank_path)
    if not force_rebuild:
        var cached_scene: PackedScene = _cache.get(CACHE_FILE, cache_hash) as PackedScene
        if cached_scene:
            return cached_scene

    var file: FileAccess = FileAccess.open(nodebank_path, FileAccess.READ)
    if not file:
        push_warning("Could not open nodebank file: %s" % nodebank_path)
        return EMPTY_NODEBANK

    var scene: PackedScene = _parse_nodebank_txt(file)
    _cache.set(CACHE_FILE, scene, cache_hash)

    var reloaded_scene: PackedScene = _cache.get(CACHE_FILE, cache_hash) as PackedScene
    return reloaded_scene if reloaded_scene else scene


func _parse_nodebank_txt(file:FileAccess) -> PackedScene:
    var root: Node = Node.new()

    root.name = "Nodebank"
    file.seek(0)
    var buffer = file.get_buffer(file.get_length())

    var parser: MaszynaParser = MaszynaParser.new()
    parser.initialize(buffer)

    var pending_label := []
    var current_group: Node = null

    while not parser.eof_reached():
        var token = parser.next_token(STOP_TOKENS).to_lower()

        match token:
            "[":
                if not pending_label:
                    current_group = _get_or_create_group(root, "Ungrouped")
                else:
                    pending_label[0] = pending_label[0].capitalize()
                    current_group = _get_or_create_group(root, " ".join(pending_label))
                    pending_label = []
            "]":
                current_group = null
            "node":
                var item:E3DModelInstance = _parse_nodebank_node(parser)
                if item:
                    current_group.add_child(item)
                    item.owner = root
            _:
                token = token.strip_edges()
                if not token in ["/", "?"]:
                    pending_label.append(token)

    var scene: PackedScene = PackedScene.new()
    var err: Error = scene.pack(root)
    root.free()
    if err != OK:
        push_error("Could not pack nodebank scene.")
        return EMPTY_NODEBANK

    return scene


func _get_or_create_group(root: Node, group_name: String) -> Node:
    var existing_group: Node = root.get_node_or_null(NodePath(group_name))
    if existing_group:
        return existing_group

    var group: Node = Node.new()
    group.name = group_name
    root.add_child(group)
    group.owner = root
    return group


func _parse_nodebank_node(parser: MaszynaParser) -> E3DModelInstance:
    var tokens = parser.get_tokens(10, STOP_TOKENS)
    var visibility_range := float(tokens[0])
    var model_path:String = tokens[8].replace("\\\\", "/").get_basename()
    var data_path = model_path.get_base_dir()
    model_path = model_path.get_file()
    var skin = null if tokens[9] == "none" else tokens[9]
    var skin_name = tokens[9].get_file()

    var item: E3DModelInstance = E3DModelInstance.new()
    item.name = "%s %s" % [model_path.capitalize(), skin_name.capitalize()] if skin else model_path.capitalize()
    item.data_path = "models".path_join(data_path)  # nodebank.txt does not include models dir
    item.model_filename = model_path
    item.skins = [skin] if skin else []

    # workaround form malformed nodebank.txt
    if tokens[9] == "endmodel":
        push_warning("[Nodebank] Malformed node definition: "+" ".join(tokens))
        return item

    match parser.next_token(STOP_TOKENS).to_lower():
        "endmode":
            # workaround for broken nodebank.txt: assume endmodel
            pass
        "endmodel":
            # end of model definition
            pass
        "lights":
            # lights section
            # TODO: add support for light section (lights are currently unsupported in E3D)
            parser.get_tokens_until("endmodel", STOP_TOKENS)
        _:
            push_warning("[Nodebank] Malformed node definition: "+" ".join(tokens))
            parser.get_tokens_until("endmodel", STOP_TOKENS)

    return item


func _make_cache_hash(nodebank_path: String) -> String:
    return "%s:%s:%s" % [
        CACHE_VERSION,
        nodebank_path,
        FileAccess.get_modified_time(nodebank_path),
    ]
