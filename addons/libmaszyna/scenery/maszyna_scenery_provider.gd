@tool
extends SceneryProvider

## MaSzyna's sceneries: the .scn files of the game directory's scenery/, parsed - or read from their
## cache - into a MaszynaCompiledScenery, and what only they have registered with the servers: the
## E3D models and their signal heads, the events, memory cells and launchers, the terrain of the
## triangles and the region files, the trainsets of MaSzyna's vehicles. Registered with SceneryServer
## as MaszynaSceneryProvider.NAME.

const NAME:StringName = &"maszyna"
## Where the sceneries are, under the game directory, and what they are
const SCENERY_DIRECTORY:String = "scenery"
const SCENERY_EXTENSION:String = "scn"
const NO_SCENARIO_MARK:String = "$"
## The label of its sceneries on a list (SceneryProvider.title)
const TITLE:String = "MaSzyna"
## The share of the loading progress the terrain starts at (instantiate_content())
const TERRAIN_PROGRESS:float = 0.6
## The parse's own throttle of its progress reports (_parse_file_with_progress())
static var _last_report_msec:int = 0

static var sky_importer = preload("res://addons/libmaszyna/scenery/maszyna_sky_importer.gd").new()
static var atmo_importer = preload("res://addons/libmaszyna/scenery/maszyna_atmo_importer.gd").new()
static var time_importer = preload("res://addons/libmaszyna/scenery/maszyna_time_importer.gd").new()
static var config_importer = preload("res://addons/libmaszyna/scenery/maszyna_config_importer.gd").new()
static var node_importer = preload("res://addons/libmaszyna/scenery/maszyna_node_importer.gd").new()
static var event_importer = preload("res://addons/libmaszyna/scenery/maszyna_event_importer.gd").new()
## A lit model no `lights` event is aimed at - it has lights but no aspects
## What a scenery's drivers think with, by name (DriverServer.implementation_register()): the
## original's driver (MaszynaAIDriver), registered by the provider before any scenery is loaded
const DRIVER_IMPLEMENTATION:StringName = &"maszyna"
## The order that gives a driver its timetable (simulationstateserializer.cpp:845, endtrainset)
const TIMETABLE_ORDER:String = "Timetable:"
const GENERIC_SIGNAL_HEAD_KIND:SignalHeadKind = preload("res://addons/libreloaded/signalling/generic_signal_head_kind.tres")
static var origin_importer = preload("res://addons/libmaszyna/scenery/maszyna_origin_importer.gd").new()
static var endorigin_importer = preload("res://addons/libmaszyna/scenery/maszyna_endorigin_importer.gd").new()
static var rotate_importer = preload("res://addons/libmaszyna/scenery/maszyna_rotate_importer.gd").new()
static var terrain_importer = preload("res://addons/libmaszyna/scenery/maszyna_terrain_importer.gd").new()
static var include_importer = preload("res://addons/libmaszyna/scenery/maszyna_include_importer.gd").new()
static var trainset_importer = preload("res://addons/libmaszyna/scenery/maszyna_trainset_importer.gd").new()
static var endtrainset_importer = preload("res://addons/libmaszyna/scenery/maszyna_endtrainset_importer.gd").new()
static var firstinit_importer = preload("res://addons/libmaszyna/scenery/maszyna_firstinit_importer.gd").new()
static var isolated_importer = preload("res://addons/libmaszyna/scenery/maszyna_isolated_importer.gd").new()
static var area_importer = preload("res://addons/libmaszyna/scenery/maszyna_area_importer.gd").new()
static var lua_importer = preload("res://addons/libmaszyna/scenery/maszyna_lua_importer.gd").new()
## 55: ModelData.model_provider renamed rendering_backend
const CACHE_FORMAT_VERSION:int = 55
## The root the scenery's nodes are packed under (_pack_objects())
const PACKED_ROOT_NAME:String = "Scenery"
const CACHE_DIRECTORY:String = "scenery_compiled"
## Parameterless includes at least this large are parsed as cached subscenes (parse_subscene_task())
const SUBSCENE_MIN_SIZE:int = 65536
## Cached subscenes nested deeper are parsed as part of their parent's cache entry
const SUBSCENE_MAX_DEPTH:int = 2
## Triangles a subscene keeps in memory before its largest chunks go to disk - subscenes are parsed
## on every worker at once (MaszynaTrianglesSink.BUDGET_BYTES is the scenery's own)
const SUBSCENE_TRIANGLES_BUDGET_BYTES:int = 32 * 1024 * 1024
## Parameterless includes smaller than this are parsed in place of the include, not as a task of the
## queue - as every include with parameters is (maszyna_include_importer.gd)
const INLINE_INCLUDE_MAX_SIZE:int = 16384
## The memory lines of the steps of a .scn converted (SceneryLoadMeasurement.print_process())
const CONVERT_TAG:String = "SceneryConvert"
static var _cache:ResourceCache = ResourceCache.create(CACHE_DIRECTORY)
## The file a queue worker opened last (open_parser()), for the loading screen to show the parse
## going on while its progress stands - written by the workers, read by the main thread
static var _file_in_parse:String = ""


static var _file_in_parse_mutex:Mutex = Mutex.new()
## The files read so far in this parse that are parsed again - included with parameters, or small;
## grass.inc is included tens of thousands of times. Emptied as the parse ends
static var _include_buffers:Dictionary[String, PackedByteArray] = {}


static var _include_buffers_mutex:Mutex = Mutex.new()
## Cache paths of subscenes being saved by queue workers - one writer per file
static var _saving_subscenes:Dictionary = {}


static var _saving_subscenes_mutex:Mutex = Mutex.new()
## The sounds of each MaSzyna scenery loaded, by its node's ObjectID (scenery_get_sounds())
var _sounds:Dictionary[int, MaszynaScenerySounds] = {}
## The scenario of each MaSzyna scenery running, by its node's ObjectID (scenario_start())
var _scenarios:Dictionary[int, MaszynaScenario] = {}


func _init() -> void:
    title = TITLE


func _enter_tree() -> void:
    DriverServer.implementation_register(DRIVER_IMPLEMENTATION, MaszynaAIDriver.new())
    TractionRenderingServer.backend_register(
        MaszynaTractionData.RENDERING_BACKEND, MaszynaTractionRenderingBackend.new()
    )


func _ready() -> void:
    SceneryServer.provider_register(NAME, get_instance_id())


func _exit_tree() -> void:
    SceneryServer.provider_unregister(NAME)
    TractionRenderingServer.backend_unregister(MaszynaTractionData.RENDERING_BACKEND)
    DriverServer.implementation_unregister(DRIVER_IMPLEMENTATION)


## The scenery's sounds, made audible by its scenario (MaszynaScenario); null for none loaded
func scenery_get_sounds(root:SceneryContentNode) -> MaszynaScenerySounds:
    return _sounds.get(root.get_instance_id())


## The sounds of the scenery [param root] loads, kept until its content goes
func scenery_set_sounds(root:SceneryContentNode, sounds:MaszynaScenerySounds) -> void:
    if not _sounds.has(root.get_instance_id()):
        root.unloading.connect(_on_scenery_unloading.bind(root.get_instance_id()), CONNECT_ONE_SHOT)
    _sounds[root.get_instance_id()] = sounds


func _on_scenery_unloading(root_id:int) -> void:
    _sounds.erase(root_id)


## The scenery's scenario (MaszynaScenario): its sounds audible, its scripts run, its keys taken
func scenario_start(root:SceneryContentNode, progressed:Callable) -> void:
    var scenario:MaszynaScenario = MaszynaScenario.new()
    _scenarios[root.get_instance_id()] = scenario
    await scenario.start(root, progressed)


func scenario_get_script_context(root:SceneryContentNode) -> RID:
    var scenario:MaszynaScenario = _scenarios.get(root.get_instance_id())
    return scenario.get_script_context() if scenario else RID()


func scenario_stop(root:SceneryContentNode) -> void:
    var scenario:MaszynaScenario = _scenarios.get(root.get_instance_id())
    if not scenario:
        return
    scenario.stop()
    _scenarios.erase(root.get_instance_id())


## Every .scn of the game directory's scenery/ but the "$" ones, by the title its header gives it
## (MaszynaSceneryInfo.read_display_name()) - a "$" file is no scenario to start
## ($stary_jawor_eszelon.scn)
func list() -> Array[SceneryInfo]:
    var sceneries:Array[SceneryInfo] = []
    var scenery_dir:String = MaszynaGameDirectory.get_game_dir().path_join(SCENERY_DIRECTORY)
    for file:String in DirAccess.get_files_at(scenery_dir):
        if file.get_extension().to_lower() == SCENERY_EXTENSION and not file.begins_with(NO_SCENARIO_MARK):
            var info:SceneryInfo = SceneryInfo.new()
            info.provider = NAME
            info.id = file
            info.title = MaszynaSceneryInfo.read_display_name(file)
            sceneries.append(info)
    return sceneries


## The .scn's header and trainsets (MaszynaSceneryInfo.read())
func describe(info:SceneryInfo) -> SceneryInfo:
    return MaszynaSceneryInfo.read(info.id)


## A MaszynaSceneryNode set to the .scn (SceneryInfo.id)
func create_scenery(info:SceneryInfo) -> SceneryContentNode:
    var scenery:MaszynaSceneryNode = MaszynaSceneryNode.new()
    scenery.autoload = false
    scenery.filename = info.id
    return scenery


## The parse stops between tokens, or joining its worker means waiting out a whole file
func set_parsing_cancelled(cancelled:bool) -> void:
    MaszynaParser.set_cancelled(cancelled)


## Wired into the "Clear caches" button (user_settings_dock.gd) alongside
## MaszynaE3DModelManager.clear_cache()/MaszynaMaterialManager.clear_cache() - nothing previously cleared this
## one, so a stale/corrupt compiled-scenery cache entry (e.g. from a killed process mid-write)
## had no way to be cleared from the settings UI.
static func clear_cache() -> void:
    _cache.clear()


## The .scn [param id] (a file name under the game directory's scenery/) with [param parameters], read
## from its cache, else parsed on workers - its terrain to disk chunk by chunk, its nodes packed -
## and cached; what SceneryServer registers with the servers. baltyk_skm1.scn alone places 8 487
## models, 90 228 "triangles" nodes (41% of them with no range at all), 2 340 tracks and 1 163
## traction spans.
func compile(root:SceneryContentNode, id:String, parameters:Dictionary) -> CompiledScenery:
    var include:MaszynaIncludeNode = root as MaszynaIncludeNode
    var filename:String = id
    var source_path:String = _get_source_path(filename)
    var parameters_hash:String = _get_parameters_hash(parameters)
    var cache_path:String = _get_cache_path(source_path, parameters_hash)
    print("[SceneryLoad] SCENERY %s" % source_path)
    var compiled:MaszynaCompiledScenery
    if include.use_cache:
        # compiled sceneries are hundreds of MB - read on a worker, so the main thread keeps drawing
        # frames (loading screen, the selector dissolving into it) instead of freezing
        compiled = await SceneryServer.run_on_worker(
            root, _load_cached.bind(cache_path, source_path, parameters_hash), 0.0,
            SceneryContentNode.LoadStage.FILES, "Reading cache"
        ) as MaszynaCompiledScenery
        if SceneryServer.is_load_given_up(root):
            return null

    if not compiled:
        # The .scn is converted into what a cached scenery is read from, and the parse let go of:
        # its terrain goes to disk chunk by chunk as it is parsed (MaszynaTrianglesSink), its nodes
        # are packed, and the scenery is built below from that alone, as from the cache - a large
        # scenery kept whole in memory as it was parsed filled it
        var triangles_sink:MaszynaTrianglesSink = MaszynaTrianglesSink.create(
            _cache.get_file_path(cache_path.get_basename()), MaszynaMaterialManager.is_material_translucent
        )
        var context:MaszynaImporterContext = await _parse_file_with_progress(
                include, filename, parameters, triangles_sink)
        if not context:
            return null
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "parsed")
        await SceneryServer.run_on_worker(
            root, assign_signal_head_kinds.bind(context.models, context.events), SceneryServer.PARSE_PROGRESS,
            SceneryContentNode.LoadStage.FILES, tr("Parsing %s") % filename
        )
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "signal heads")
        var chunk_descriptors:Variant = await SceneryServer.run_on_worker(
            root, triangles_sink.finish, SceneryServer.PARSE_PROGRESS, SceneryContentNode.LoadStage.FILES, "Building terrain"
        )
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "terrain written")
        compiled = _compile_scenery(source_path, parameters_hash, context, context.objects)
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "packed")
        for object:Variant in context.objects:
            if object is Node:
                (object as Node).free()
        var cacheable:bool = context.cacheable
        context = null
        # the parse ran on a dozen workers, and the allocator keeps what each one freed
        ProcessMemory.release_unused()
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "parse let go")
        if not compiled:
            return null
        for descriptor:Dictionary in (chunk_descriptors if chunk_descriptors else []):
            var chunk := MaszynaTrianglesChunkData.new()
            chunk.geometry_path = descriptor["path"]
            chunk.chunk_index = descriptor["index"]
            chunk.position = descriptor["position"]
            chunk.material_name = descriptor["texture"]
            chunk.range_min = descriptor["range_min"]
            chunk.range_max = maxf(descriptor["range_max"], 0.0)
            compiled.triangle_chunks.append(chunk)
        if include.use_cache and cacheable:
            await SceneryServer.run_on_worker(
                root, _cache.set.bind(cache_path, compiled), SceneryServer.PARSE_PROGRESS, SceneryContentNode.LoadStage.FILES,
                "Saving cache"
            )
            SceneryLoadMeasurement.print_process(CONVERT_TAG, "saved")

    return compiled


## The .scn's models and their signal heads, its events, memory cells and launchers, its terrain of
## triangles and of region files - what of it the core does not register
func instantiate_content(root:SceneryContentNode, compiled:CompiledScenery, track_rids:Array[RID]) -> void:
    var scn:MaszynaCompiledScenery = compiled as MaszynaCompiledScenery
    var world_3d:World3D = root.get_world_3d()
    var models:Array[ModelData] = compiled.models
    var model_rids:Array[RID] = []
    var built_count:int = 0
    # the original's signal heads: every lit model, driven by the scenery's own `lights` events
    var signalling_system:RID = SignallingServer.system_create()
    SignallingServer.system_attach_implementation(signalling_system, MaszynaSignallingImplementation.new())
    SignallingServer.system_set_name(signalling_system, root.get_scenery_name())
    root._signalling_system_rids.append(signalling_system)
    for model_data:ModelData in models:
        var e3d_rid:RID = _build_model(model_data, world_3d, signalling_system)
        model_rids.append(e3d_rid)
        if e3d_rid.is_valid():
            root._model_rids.append(e3d_rid)
        built_count += 1
        await SceneryServer.report_progress_throttled(
            root, lerpf(SceneryServer.CONTENT_PROGRESS, TERRAIN_PROGRESS, float(built_count) / models.size()), SceneryContentNode.LoadStage.INFRASTRUCTURE,
            TranslationServer.translate("Registering %s") % model_data.model
        )
        if SceneryServer.is_load_given_up(root):
            return
    # the original's firstinit: everything the events are aimed at exists by now
    await MaszynaEventFactory.build(
        root, scn.events, scn.memcells, scn.launchers, compiled.sounds, scn.isolated_sections, compiled.tracks,
        track_rids, models, model_rids, compiled.power_sources
    )
    if SceneryServer.is_load_given_up(root):
        return
    await SceneryServer.report_progress(root, TERRAIN_PROGRESS, SceneryContentNode.LoadStage.TERRAIN, "Registering terrain")
    await _build_triangle_chunks(root, scn.triangle_chunks, world_3d)
    if SceneryServer.is_load_given_up(root):
        return
    # the terrain of a region file is supplied as the camera comes near; its sections are listed on
    # a worker - a file lists twenty thousand of them
    for region_file:String in scn.region_files:
        var provider:MaszynaSBTTerrainProvider = MaszynaSBTTerrainProvider.new()
        var opened:bool = await SceneryServer.run_on_worker(
            root, provider.open.bind(region_file), TERRAIN_PROGRESS, SceneryContentNode.LoadStage.TERRAIN, "Registering terrain"
        )
        if SceneryServer.is_load_given_up(root):
            return
        if opened:
            root._provider_rids.append(SceneryStreamingServer.provider_register(provider, world_3d.scenario))
    print("[SceneryMemory] events %d, terrain chunks %d" % [scn.events.size(), scn.triangle_chunks.size()])


## The scenery's trainsets, their drivers thinking as the original's (DRIVER_IMPLEMENTATION)
func instantiate_vehicles(root:SceneryContentNode, compiled:CompiledScenery) -> void:
    _describe_trainsets(root, compiled.trainsets)
    await SceneryServer.instantiate_trainsets(root, compiled.trainsets, DRIVER_IMPLEMENTATION, _give_timetable)


## A trainset's mission and whether the scenario offers it to the player, which only the .scn's
## comments say - read off its header (MaszynaSceneryInfo), the trainsets matched by name in their order
static func _describe_trainsets(root:SceneryContentNode, trainsets:Array[TrainsetData]) -> void:
    var include:MaszynaIncludeNode = root as MaszynaIncludeNode
    if not include or not include.filename:
        return
    var described:Array[TrainsetData] = MaszynaSceneryInfo.read(include.filename).trainsets.duplicate()
    for data:TrainsetData in trainsets:
        for index:int in described.size():
            if described[index].name == data.name.to_lower():
                data.description = described[index].description
                data.player_drivable = described[index].player_drivable
                described.remove_at(index)
                break


## endtrainset (simulationstateserializer.cpp:839-848): the trainset's driver gets its timetable and
## the velocity it starts with
static func _give_timetable(trainset_data:TrainsetData, driver:RID) -> void:
    if trainset_data.timetable:
        DriverServer.driver_send_command(driver, TIMETABLE_ORDER + trainset_data.timetable, trainset_data.velocity, 0.0)


## Registers the terrain chunks with MaszynaSceneryChunkRenderingServer; they are rendered, and their
## geometry read from its file, only while the camera is within range of their chunk (see the
## server's doc comment for why)
static func _build_triangle_chunks(
    root:SceneryContentNode, chunks:Array[MaszynaTrianglesChunkData], world_3d:World3D
) -> void:
    # the scenery's node frees them with its content, and draws them into its world
    var chunk_rids:Array[RID] = []
    root.content_own(chunk_rids, MaszynaSceneryChunkRenderingServer.free_chunk,
            MaszynaSceneryChunkRenderingServer.chunk_set_scenario)
    for chunk:MaszynaTrianglesChunkData in chunks:
        chunk_rids.append(MaszynaSceneryChunkRenderingServer.create_chunk(
            chunk, world_3d.scenario, ResourceLoader.load.bind(chunk.geometry_path)
        ))
        await SceneryServer.frame_budget_wait()


static func _compile_scenery(
    source_path:String,
    parameters_hash:String,
    context:MaszynaImporterContext,
    objects:Array,
    compiled:MaszynaCompiledScenery = MaszynaCompiledScenery.new(),
) -> MaszynaCompiledScenery:
    var packed_scene:PackedScene = _pack_objects(objects)
    if not packed_scene:
        return null

    compiled.format_version = CACHE_FORMAT_VERSION
    compiled.source_path = source_path
    compiled.parameters_hash = parameters_hash
    compiled.dependencies = context.dependencies.duplicate(true)
    compiled.nodes = packed_scene
    compiled.tracks = context.tracks
    compiled.traction = context.traction
    compiled.power_sources = context.power_sources
    compiled.models = context.models
    compiled.events = context.events
    compiled.memcells = context.memcells
    compiled.launchers = context.launchers
    compiled.sounds = context.sounds
    compiled.isolated_sections = context.isolated_sections
    compiled.lines = context.lines
    compiled.trainsets = context.trainsets
    compiled.scripts = context.scripts
    compiled.region_files = context.region_files
    return compiled


static func _pack_objects(objects:Array) -> PackedScene:
    var scene_root := Node3D.new()
    # a node that was never in a tree has no name, and an empty one cannot be set when the packed
    # scene is instantiated (scene/main/node.cpp set_name)
    scene_root.name = PACKED_ROOT_NAME
    for object:Variant in objects:
        if object is Node:
            scene_root.add_child(object)
            object.owner = scene_root

    var packed_scene:PackedScene = ResourceCache.scene_pack(scene_root)

    for child:Node in scene_root.get_children():
        child.owner = null
        scene_root.remove_child(child)
    scene_root.free()

    if not packed_scene:
        push_error("Cannot compile scenery cache")
        return null
    return packed_scene


static func _load_cached(
    cache_path:String,
    source_path:String,
    parameters_hash:String,
) -> MaszynaCompiledScenery:
    var compiled:MaszynaCompiledScenery = _cache.get(cache_path) as MaszynaCompiledScenery
    if not compiled:
        return null
    if not _is_cache_valid(compiled, source_path, parameters_hash):
        _cache.remove(cache_path)
        return null
    return compiled


static func _is_cache_valid(
    compiled:MaszynaCompiledScenery,
    source_path:String,
    parameters_hash:String,
) -> bool:
    if not compiled.format_version == CACHE_FORMAT_VERSION:
        return false
    if not compiled.source_path == source_path:
        return false
    if not compiled.parameters_hash == parameters_hash:
        return false
    if not compiled.nodes:
        return false
    if not compiled.dependencies.has(source_path):
        return false
    for dependency_value:Variant in compiled.dependencies.keys():
        var dependency_path:String = dependency_value
        if not FileAccess.file_exists(dependency_path):
            return false
        var state:Dictionary = compiled.dependencies[dependency_path]
        if not FileAccess.get_modified_time(dependency_path) == int(state["modified_time"]):
            return false
        var file:FileAccess = FileAccess.open(dependency_path, FileAccess.READ)
        if not file:
            return false
        if not file.get_length() == int(state["size"]):
            return false
    return true


static func _get_parameters_hash(parameters:Dictionary) -> String:
    var keys:Array = parameters.keys()
    keys.sort()
    var parts:Array[String] = []
    for key_value:Variant in keys:
        var key:String = str(key_value)
        var value:String = JSON.stringify(parameters[key_value])
        parts.append("%d:%s=%d:%s" % [key.length(), key, value.length(), value])
    return "|".join(parts).md5_text()


static func _get_cache_path(source_path:String, parameters_hash:String) -> String:
    return (
        "%s:%s:%s" % [CACHE_FORMAT_VERSION, source_path, parameters_hash]
    ).md5_text() + ".res"


static func _get_source_path(filename:String) -> String:
    var scenery_dir:String = MaszynaGameDirectory.get_game_dir().path_join(SCENERY_DIRECTORY)
    return scenery_dir.path_join(ReloadedPluginServer.data_path_resolve(scenery_dir, filename)).simplify_path()


func scenery_exists(filename: String):
    var abs_file:String = _get_source_path(filename)
    return FileAccess.file_exists(abs_file)


func parse_file(filename: String, parameters: Dictionary, context: MaszynaImporterContext) -> Array:
    var parser:MaszynaParser = open_parser(filename, parameters, context)
    if not parser:
        return []
    var objects:Array = parser.parse()
    _close_parser(parser, filename, context)
    return objects


## Parses a file in its own context restored from state (MaszynaImporterContext.get_state()) -
## a WorkerTaskQueue task. Includes become further tasks of the queue, merged at the end.
func parse_file_task(
    filename:String, parameters:Dictionary, state:Dictionary, queue:WorkerTaskQueue
) -> MaszynaImporterContext:
    var context:MaszynaImporterContext = MaszynaImporterContext.from_state(state)
    context.queue = queue
    context.objects = parse_file(filename, parameters, context)
    context.merge_pending_includes()
    context.queue = null
    return context


## parse_file_task() of a subscene (see maszyna_include_importer.gd), read from / saved to the
## cache. The cache key includes the inherited origin/rotate - parsed objects are already placed
## in world coordinates - whether a region file holds the terrain, which leaves its shapes out, and
## the scenery's joinduplicatedevents, which its events are read with; the value the subscene
## leaves it at is cached with it.
## The subscene's terrain is chunked in a sink of its own, cached with it,
## and added to the scenery's sink.
func parse_subscene_task(
    filename:String, parameters:Dictionary, state:Dictionary, queue:WorkerTaskQueue
) -> MaszynaImporterContext:
    var source_path:String = _get_source_path(filename)
    var state_hash:String = var_to_str(
        [
            state["origin"], state["rotate"], state["binary_terrain"], state["binary_terrain_state"],
            state["join_duplicated_events"],
        ]
    ).md5_text()
    var cache_path:String = _get_cache_path(source_path, state_hash)
    var scenery_sink:MaszynaTrianglesSink = state["triangles_sink"]
    var compiled:MaszynaCompiledSubscene = _load_cached(cache_path, source_path, state_hash) as MaszynaCompiledSubscene
    if compiled:
        var cached := MaszynaImporterContext.new()
        cached.tracks.assign(compiled.tracks)
        cached.traction.assign(compiled.traction)
        cached.power_sources.assign(compiled.power_sources)
        cached.models.assign(compiled.models)
        cached.events.assign(compiled.events)
        cached.memcells.assign(compiled.memcells)
        cached.launchers.assign(compiled.launchers)
        cached.sounds.assign(compiled.sounds)
        cached.isolated_sections.assign(compiled.isolated_sections)
        cached.lines.assign(compiled.lines)
        cached.trainsets.assign(compiled.trainsets)
        cached.region_files.assign(compiled.region_files)
        cached.join_duplicated_events = compiled.join_duplicated_events
        for chunk_path:String in compiled.triangle_chunk_paths:
            scenery_sink.add_geometry_file(chunk_path)
        cached.dependencies = compiled.dependencies.duplicate(true)
        cached.objects = SceneryServer.instantiate_cached_nodes(compiled.nodes)
        return cached

    state["subscene_depth"] = int(state["subscene_depth"]) + 1
    # one writer per cache entry and its chunk directory; the same subscene parsed twice at once
    # (included twice at the same origin and rotation) keeps its second copy in memory, unsaved
    _saving_subscenes_mutex.lock()
    var is_saving:bool = _saving_subscenes.has(cache_path)
    if not is_saving:
        _saving_subscenes[cache_path] = true
    _saving_subscenes_mutex.unlock()
    # the subscene's triangles go to disk beside its cache entry as they are parsed, like the
    # scenery's - kept whole in memory, the vegetation of a large scenery took gigabytes
    var subscene_sink:MaszynaTrianglesSink = MaszynaTrianglesSink.create(
        "" if is_saving else _cache.get_file_path(cache_path.get_basename()),
        MaszynaMaterialManager.is_material_translucent, SUBSCENE_TRIANGLES_BUDGET_BYTES
    )
    state["triangles_sink"] = subscene_sink
    var context:MaszynaImporterContext = parse_file_task(filename, parameters, state, queue)
    if is_saving:
        for geometry:MaszynaTrianglesChunkGeometry in subscene_sink.get_geometries():
            scenery_sink.add_geometry(geometry)
        return context

    var chunk_paths:PackedStringArray = []
    # a section's file holds all its chunks, each with a descriptor of its own
    for descriptor:Dictionary in subscene_sink.finish():
        if not descriptor["path"] in chunk_paths:
            chunk_paths.append(descriptor["path"])
            scenery_sink.add_geometry_file(descriptor["path"])
    if context.cacheable:
        var subscene := MaszynaCompiledSubscene.new()
        subscene.triangle_chunk_paths = chunk_paths
        subscene.join_duplicated_events = context.join_duplicated_events
        if _compile_scenery(source_path, state_hash, context, context.objects, subscene):
            _cache.set(cache_path, subscene)
    _saving_subscenes_mutex.lock()
    _saving_subscenes.erase(cache_path)
    _saving_subscenes_mutex.unlock()
    return context


## Parses root's scenery on WorkerTaskQueue workers (every include is a task), reporting
## progress as they finish, at most once per SceneryServer.FRAME_BUDGET_MSEC: finished tasks / tasks submitted
## so far.
func _parse_file_with_progress(
    root:MaszynaIncludeNode, filename:String, parameters:Dictionary, triangles_sink:MaszynaTrianglesSink
) -> MaszynaImporterContext:
    var root_context := MaszynaImporterContext.new()
    root_context.rotate = root.context_rotate
    root_context.origin = root.context_origin
    root_context.triangles_sink = triangles_sink
    var queue := WorkerTaskQueue.new()
    SceneryServer.queue_add(queue)
    # the scenario's region file first: what is parsed after it depends on whether it is there
    root_context.load_scenario_binary_terrain(filename)
    var task_id:int = queue.submit(parse_file_task.bind(filename, parameters, root_context.get_state(), queue))
    # every include is a task submitted while parsing, so the total grows with the parse; the bar
    # does not go back when it does
    var parsed:float = 0.0
    # the root task finishes last: it waits for the includes it submitted
    while await queue.task_finished != task_id:
        if Time.get_ticks_msec() - _last_report_msec < SceneryServer.FRAME_BUDGET_MSEC:
            continue
        _last_report_msec = Time.get_ticks_msec()
        parsed = maxf(parsed, float(queue.get_completed_count()) / float(queue.get_submitted_count()))
        _file_in_parse_mutex.lock()
        var file_in_parse:String = _file_in_parse
        _file_in_parse_mutex.unlock()
        root.load_files_parsed.emit(queue.get_completed_count(), file_in_parse)
        root.load_progress.emit(
            SceneryServer.PARSE_PROGRESS * parsed, SceneryContentNode.LoadStage.FILES, tr("Parsing %s") % filename
        )
    var context:MaszynaImporterContext = queue.wait(task_id) as MaszynaImporterContext
    SceneryServer.queue_remove(queue)
    _include_buffers_mutex.lock()
    _include_buffers.clear()
    _include_buffers_mutex.unlock()
    if not context:
        # a drained queue (cancel_loading()) is a load given up, not one that failed
        if not SceneryServer.is_load_given_up(root):
            push_error("Cannot parse scenery: " + filename)
        return null
    # the scenario's region file is found before the parse, in the root's own context
    context.dependencies.merge(root_context.dependencies)
    context.region_files.assign(root_context.region_files + context.region_files)
    return context


func open_parser(filename: String, parameters: Dictionary, context: MaszynaImporterContext) -> MaszynaParser:
    var abs_file:String = _get_source_path(filename)
    _file_in_parse_mutex.lock()
    _file_in_parse = filename
    _file_in_parse_mutex.unlock()
    if not context.begin_file(abs_file):
        push_error("Recursive scenery include: " + abs_file)
        return null
    _include_buffers_mutex.lock()
    var buffer:PackedByteArray = _include_buffers.get(abs_file, PackedByteArray())
    _include_buffers_mutex.unlock()
    if not buffer:
        var file := FileAccess.open(abs_file, FileAccess.READ)
        if not file:
            context.end_file(abs_file)
            push_error("Cannot load scenery: " + abs_file)
            return null
        buffer = file.get_buffer(file.get_length())
        if parameters or buffer.size() < INLINE_INCLUDE_MAX_SIZE:
            _include_buffers_mutex.lock()
            _include_buffers[abs_file] = buffer
            _include_buffers_mutex.unlock()
    context.register_dependency(abs_file, buffer.size())

    var parser := MaszynaParser.new()
    parser.set_parameters(parameters)
    parser.initialize(buffer)
    parser.register_handler("sky", _make_importer_callback(sky_importer, context))
    parser.register_handler("atmo", _make_importer_callback(atmo_importer, context))
    parser.register_handler("time", _make_importer_callback(time_importer, context))
    parser.register_handler("config", _make_importer_callback(config_importer, context))
    parser.register_handler("node", _make_importer_callback(node_importer, context))
    parser.register_handler("event", _make_importer_callback(event_importer, context))
    parser.register_handler("origin", _make_importer_callback(origin_importer, context))
    parser.register_handler("endorigin", _make_importer_callback(endorigin_importer, context))
    parser.register_handler("rotate", _make_importer_callback(rotate_importer, context))
    parser.register_handler("terrain", _make_importer_callback(terrain_importer, context))
    parser.register_handler("include", _make_importer_callback(include_importer, context))
    parser.register_handler("trainset", _make_importer_callback(trainset_importer, context))
    parser.register_handler("endtrainset", _make_importer_callback(endtrainset_importer, context))
    parser.register_handler("firstinit", _make_importer_callback(firstinit_importer, context))
    parser.register_handler("isolated", _make_importer_callback(isolated_importer, context))
    parser.register_handler("area", _make_importer_callback(area_importer, context))
    parser.register_handler("lua", _make_importer_callback(lua_importer, context))
    return parser


func _close_parser(parser:MaszynaParser, filename:String, context:MaszynaImporterContext) -> void:
    for token in ["sky", "atmo", "config", "node", "event", "origin", "endorigin", "rotate", "terrain", "include", "trainset", "endtrainset", "firstinit", "isolated", "area", "lua"]:
        parser.unregister_handler(token)
    context.end_file(_get_source_path(filename))


static func _make_importer_callback(importer, context) -> Callable:
    var callback = func(p): return importer.import(p, context)
    return callback


## Registers the placement with MaszynaE3DRenderingServer instead of instancing it (a real scenery places
## hundreds of thousands of submodels - instancing them all at load costs both the loading time and
## the frame rate). The server loads the model and builds the instance once the streaming camera
## comes within the node's range of the chunk it falls into, and clears it when the camera leaves.
## The original's signal heads have no kind: a model shows whatever `lights` events the scenery aims
## at it (Event.cpp:1741-1803). Every lit model, and every model such an event is aimed at, is
## given a kind made of those events (MaszynaSignalHeadKindFactory); the copies of one include
## end up with equal kinds, which are then one resource. A lit model no event reaches gets the
## generic kind. Target names are matched in lower case, as the original reads them (Event.cpp:327).
static func assign_signal_head_kinds(
    models:Array[ModelData], scenery_events:Array[MaszynaEventData]
) -> void:
    var events_by_target:Dictionary[String, Array] = {}
    for event:MaszynaEventData in scenery_events:
        if not event.type == "lights":
            continue
        for target:String in event.targets:
            if not events_by_target.has(target):
                events_by_target[target] = []
            events_by_target[target].append(event)
    var kinds:Dictionary[String, SignalHeadKind] = {}
    for model_data:ModelData in models:
        var events:Array = events_by_target.get(model_data.name.to_lower(), []) if model_data.name else []
        if not events:
            var scn_model:MaszynaModelData = model_data as MaszynaModelData
            model_data.signal_head_kind = GENERIC_SIGNAL_HEAD_KIND if scn_model and scn_model.lights else null
            continue
        var aspects:Dictionary = {}
        for event:MaszynaEventData in events:
            # one TAnimModel::LightSet() value per light, light 0 first
            var values:PackedFloat32Array = []
            for value:String in event.parameters:
                values.append(float(value))
            aspects[MaszynaSignalHeadKindFactory.get_aspect_name(event.name, model_data.name)] = values
        var key:String = var_to_str(aspects)
        if not kinds.has(key):
            kinds[key] = MaszynaSignalHeadKindFactory.create_kind(aspects)
        model_data.signal_head_kind = kinds[key]


static func _build_model(model_data:ModelData, world_3d:World3D, signalling_system:RID) -> RID:
    # a .scn's model is found in its own directory; any other by its name alone, by the backend of its
    # provider
    var scn_model:MaszynaModelData = model_data as MaszynaModelData
    var model_rid:RID = SceneryServer.build_model(
        model_data, scn_model.data_path.path_join(model_data.model) if scn_model else model_data.model, world_3d
    )
    # the declared modes outlive the streaming, so they are set once here and not on every build
    if scn_model and scn_model.lights:
        MaszynaE3DRenderingServer.instance_set_lights_modes(model_rid, scn_model.lights)
    var signal_head:RID = SceneryServer.build_signal_head(model_rid, model_data)
    if signal_head.is_valid():
        SignallingServer.system_add_signal_head(signalling_system, signal_head)
    return model_rid


