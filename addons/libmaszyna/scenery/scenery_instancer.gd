@tool
extends Node

static var sky_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_sky_importer.gd").new()
static var atmo_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_atmo_importer.gd").new()
static var time_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_time_importer.gd").new()
static var config_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_config_importer.gd").new()
static var node_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_importer.gd").new()
static var event_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_event_importer.gd").new()
## A lit model no `lights` event is aimed at - it has lights but no aspects
## What a scenery's drivers think with, by name (DriverServer.implementation_register()): the
## original's driver, which the game registers - a scenery knows its drivers, not their class
const DRIVER_IMPLEMENTATION:StringName = &"maszyna_legacy"
## The order that gives a driver its timetable (simulationstateserializer.cpp:845, endtrainset)
const TIMETABLE_ORDER:String = "Timetable:"
const GENERIC_SIGNAL_HEAD_KIND:SignalHeadKind = preload("../signalling/generic_signal_head_kind.tres")
static var origin_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_origin_importer.gd").new()
static var endorigin_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_endorigin_importer.gd").new()
static var rotate_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_rotate_importer.gd").new()
static var terrain_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_terrain_importer.gd").new()
static var include_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_include_importer.gd").new()
static var trainset_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_trainset_importer.gd").new()
static var endtrainset_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_endtrainset_importer.gd").new()
static var firstinit_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_firstinit_importer.gd").new()
static var isolated_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_isolated_importer.gd").new()
static var area_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_area_importer.gd").new()
static var lua_importer = preload("res://addons/libmaszyna/legacy/scenery/maszyna_lua_importer.gd").new()
const CACHE_FORMAT_VERSION:int = 41
## The root the scenery's nodes are packed under (_pack_objects())
const PACKED_ROOT_NAME:String = "Scenery"
const CACHE_DIRECTORY:String = "scenery_compiled"
## Parameterless includes at least this large are parsed as cached subscenes (parse_subscene_task())
const SUBSCENE_MIN_SIZE:int = 65536
## Cached subscenes nested deeper are parsed as part of their parent's cache entry
const SUBSCENE_MAX_DEPTH:int = 2
## Triangles a subscene keeps in memory before its largest chunks go to disk - subscenes are parsed
## on every worker at once (SceneryTrianglesSink.BUDGET_BYTES is the scenery's own)
const SUBSCENE_TRIANGLES_BUDGET_BYTES:int = 32 * 1024 * 1024
## Parameterless includes smaller than this are parsed in place of the include, not as a task of the
## queue - as every include with parameters is (maszyna_include_importer.gd)
const INLINE_INCLUDE_MAX_SIZE:int = 16384
## Share of the loading progress taken by reading the scenery - parsing the .scn, or the cache
const PARSE_PROGRESS:float = 0.5
## The progress the vehicles' stage starts at; it reaches the end as their trainsets stand
const VEHICLES_PROGRESS:float = 0.9
## Time the main thread may work through a load before it lets a frame be drawn - the loading
## screen's animations move only between frames (it was 100 ms: 10 frames a second at best)
const FRAME_BUDGET_MSEC:int = 12
## The memory lines of the steps of a .scn converted (SceneryLoadMeasurement.print_process())
const CONVERT_TAG:String = "SceneryConvert"

static var _cache:ResourceCache = ResourceCache.create(CACHE_DIRECTORY)
## Queues currently parsing, so a scenery leaving the tree can stop them - their tasks are
## GDScript and call GDScript handlers, which must not be reached once the scripts are going away
static var _active_queues:Array[WorkerTaskQueue] = []
static var _last_report_msec:int = 0
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


## Stops every parse in flight and joins its workers. Called by a scenery leaving the tree: the
## tasks are GDScript and the parser calls GDScript handlers through a Callable, so a worker still
## inside one when the scripts are freed jumps into code that is gone (see FINDINGS.md,
## 2026-09-24). Waiting here is safe, because here the scripts are still alive.
static func cancel_loading() -> void:
    # the parse has to stop between tokens, or joining its worker means waiting out a whole file
    MaszynaParser.set_cancelled(true)
    for queue:WorkerTaskQueue in _active_queues.duplicate():
        queue.drain()
    _active_queues.clear()
    # the workers are joined, so nothing is parsing and the next scenery may start clean
    MaszynaParser.set_cancelled(false)


## Wired into the "Clear caches" button (user_settings_dock.gd) alongside
## E3DModelManager.clear_cache()/MaterialManager.clear_cache() - nothing previously cleared this
## one, so a stale/corrupt compiled-scenery cache entry (e.g. from a killed process mid-write)
## had no way to be cleared from the settings UI.
static func clear_cache() -> void:
    _cache.clear()


## Parses root.filename and (re-)populates root with everything the scenery declares.
##
## Tracks, traction, models and merged terrain meshes are built directly against TrackServer/
## TrackRenderingServer/TractionRenderingServer/E3DRenderingServer/MaszynaSceneryChunkRenderingServer's
## RID-based API (_build_track()/_build_traction()/_build_model()/_build_triangle_chunks() below)
## instead of instantiating TrackNormal3D/TrackSwitch3D/Traction3D/E3DModelInstance nodes -
## a real scenery can have thousands of these, and a Node per segment (each with its own @tool
## script and per-frame _process()) is overhead that only actually earns its keep for a handful of
## hand-authored pieces edited directly in a scene like demo_3d.tscn. root keeps the resulting RIDs
## (_track_rids/_track_render_rids/_traction_rids/_wire_power_rids/_power_source_rids/_e3d_rids/
## _triangle_chunk_rids) so they can be freed on the next reload or when root itself leaves the
## tree - see maszyna_include.gd.
##
## What the rendering servers get here is a registration, not geometry: SceneryStreamingServer
## builds each piece only while the camera is within its visibility range of the 1 km chunk
## holding it. baltyk_skm1.scn alone places 8 487 models, 90 228 "triangles" nodes (41% of them
## with no range at all), 2 340 tracks and 1 163 traction spans.
func instantiate(root: MaszynaIncludeNode, parameters: Dictionary = {}) -> void:
    # what is loaded is what the scenery was set to when the load began
    var filename:String = root.filename
    var source_path:String = _get_source_path(filename)
    var parameters_hash:String = _get_parameters_hash(parameters)
    var cache_path:String = _get_cache_path(source_path, parameters_hash)
    var world_3d:World3D = root.get_world_3d()
    var measurement := SceneryLoadMeasurement.new(root)
    var compiled:MaszynaCompiledScenery
    if root.use_cache:
        # compiled sceneries are hundreds of MB - read on a worker, so the main thread keeps drawing
        # frames (loading screen, the selector dissolving into it) instead of freezing
        compiled = await _run_on_worker(
            root, _load_cached.bind(cache_path, source_path, parameters_hash), 0.0,
            MaszynaIncludeNode.LoadStage.FILES, "Reading cache"
        ) as MaszynaCompiledScenery
        if _is_load_given_up(root):
            return

    if not compiled:
        # The .scn is converted into what a cached scenery is read from, and the parse let go of:
        # its terrain goes to disk chunk by chunk as it is parsed (SceneryTrianglesSink), its nodes
        # are packed, and the scenery is built below from that alone, as from the cache - a large
        # scenery kept whole in memory as it was parsed filled it
        var triangles_sink:SceneryTrianglesSink = SceneryTrianglesSink.create(
            _cache.get_file_path(cache_path.get_basename())
        )
        var context:MaszynaImporterContext = await _parse_file_with_progress(
                root, filename, parameters, triangles_sink)
        if not context:
            return
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "parsed")
        await _run_on_worker(
            root, assign_signal_head_kinds.bind(context.models, context.events), PARSE_PROGRESS,
            MaszynaIncludeNode.LoadStage.FILES, tr("Parsing %s") % filename
        )
        SceneryLoadMeasurement.print_process(CONVERT_TAG, "signal heads")
        var chunk_descriptors:Variant = await _run_on_worker(
            root, triangles_sink.finish, PARSE_PROGRESS, MaszynaIncludeNode.LoadStage.FILES, "Building terrain"
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
            measurement.finish()
            return
        for descriptor:Dictionary in (chunk_descriptors if chunk_descriptors else []):
            var chunk := MaszynaTrianglesChunkData.new()
            chunk.geometry_path = descriptor["path"]
            chunk.position = descriptor["position"]
            chunk.material_name = descriptor["texture"]
            chunk.range_min = descriptor["range_min"]
            chunk.range_max = maxf(descriptor["range_max"], 0.0)
            compiled.triangle_chunks.append(chunk)
        if root.use_cache and cacheable:
            await _run_on_worker(
                root, _cache.set.bind(cache_path, compiled), PARSE_PROGRESS, MaszynaIncludeNode.LoadStage.FILES,
                "Saving cache"
            )
            SceneryLoadMeasurement.print_process(CONVERT_TAG, "saved")

    if _is_load_given_up(root):
        return
    await _report_progress(root, PARSE_PROGRESS, MaszynaIncludeNode.LoadStage.INFRASTRUCTURE, "Registering tracks and traction")
    await _instantiate_server_data(
        root,
        world_3d,
        compiled.tracks,
        compiled.traction,
        compiled.power_sources,
        compiled.models,
        compiled.events,
        compiled.memcells,
        compiled.launchers,
        compiled.sounds,
        compiled.isolated_sections,
        PARSE_PROGRESS,
        0.6,
    )
    if _is_load_given_up(root):
        return
    await _report_progress(root, 0.6, MaszynaIncludeNode.LoadStage.TERRAIN, "Registering terrain")
    await _build_triangle_chunks(root, compiled.triangle_chunks, world_3d)
    if _is_load_given_up(root):
        return
    # the terrain of a region file is supplied as the camera comes near; its sections are listed on
    # a worker - a file lists twenty thousand of them
    for region_file:String in compiled.region_files:
        var provider:MaszynaLegacySBTTerrainProvider = MaszynaLegacySBTTerrainProvider.new()
        var opened:bool = await _run_on_worker(
            root, provider.open.bind(region_file), 0.6, MaszynaIncludeNode.LoadStage.TERRAIN, "Registering terrain"
        )
        if _is_load_given_up(root):
            return
        if opened:
            root._provider_rids.append(SceneryStreamingServer.provider_register(provider, world_3d.scenario))
    await _report_progress(root, 0.7, MaszynaIncludeNode.LoadStage.OBJECTS, "Instancing objects")
    await _attach_objects(root, _instantiate_cached_nodes(compiled.nodes), 0.7, 0.9)
    if _is_load_given_up(root):
        return
    await _build_trainsets(root, compiled.trainsets)
    if _is_load_given_up(root):
        return
    if Engine.is_editor_hint():
        root.SceneryEditor.update_owners(root)
    root._scenario_scripts.assign(compiled.scripts)
    measurement.finish()
    SceneryLoadMeasurement.print_scenery(compiled)
    # what the load read and built from is let go of with the compiled scenery, and given back
    compiled = null
    ProcessMemory.release_unused()
    SceneryLoadMeasurement.print_process("SceneryMemory", "loaded")


## The load is not to go on: its scenery is gone - the editor frees a scene it reopens, mid-load -
## or was told to stop (MaszynaIncludeNode.stop_loading()). Asked after every wait of a load.
## `root` is untyped: a parameter typed MaszynaIncludeNode refuses a freed scenery at the call,
## before is_instance_valid() could say so.
static func _is_load_given_up(root:Variant) -> bool:
    return not is_instance_valid(root) or root.is_load_given_up()


## Reports the next loading stage and lets a frame be drawn (e.g. a loading screen) before it runs.
## `message` is a msgid the loading screen's Label translates; one composed with a value is
## translated before the value goes in.
static func _report_progress(
    root:MaszynaIncludeNode, progress:float, stage:MaszynaIncludeNode.LoadStage, message:String
) -> void:
    root.load_progress.emit(progress, stage, message)
    await root.get_tree().process_frame
    _last_report_msec = Time.get_ticks_msec()


## _report_progress() for loops over many objects - reports only after FRAME_BUDGET_MSEC
static func _report_progress_throttled(
    root:MaszynaIncludeNode, progress:float, stage:MaszynaIncludeNode.LoadStage, message:String
) -> void:
    if Time.get_ticks_msec() - _last_report_msec < FRAME_BUDGET_MSEC:
        return
    await _report_progress(root, progress, stage, message)


## Lets a frame be drawn once the main thread has worked FRAME_BUDGET_MSEC since the last one, for
## the loops of a load that report nothing of their own (MaszynaLegacyEventFactory.build())
static func frame_budget_wait() -> void:
    if Time.get_ticks_msec() - _last_report_msec < FRAME_BUDGET_MSEC:
        return
    await (Engine.get_main_loop() as SceneTree).process_frame
    _last_report_msec = Time.get_ticks_msec()


## The scenery's trainsets and their vehicles, through the servers - no node for either
## (MaszynaLegacyVehicleSystem, RailVehicleServer): every vehicle is built, then each trainset
## stands its vehicles on its track and couples them (trainset_place(), "endtrainset"), and only
## then the drivers are given their orders - what a driver does first is sent along the couplers
## (simulationstateserializer.cpp:818-848). Every track is registered by now.
static func _build_trainsets(root:MaszynaIncludeNode, trainsets:Array[MaszynaTrainsetData]) -> void:
    await _report_progress(root, VEHICLES_PROGRESS, MaszynaIncludeNode.LoadStage.VEHICLES, "Instancing vehicles")
    # A trainset on a track that is not built - a road car, roads are not built yet
    # (maszyna_node_track_importer.gd) - could never be placed, and still cost whole vehicles:
    # their physics stepped, their drivers and cab logic run (docs/findings-archive.md, 2026-10-03
    # hundreds of vehicles)
    var placed:Array[MaszynaTrainsetData] = []
    var tracks:Array[RID] = []
    var vehicles:Array[Array] = []
    # the vehicles of each placed trainset, as this load arranges them (trainset_override)
    var dynamics:Array[Array] = []
    for trainset_data:MaszynaTrainsetData in trainsets:
        var track:RID = TrackServer.track_get_rid_by_name(trainset_data.track_name)
        if not track.is_valid():
            continue
        var trainset_vehicles:Array[RID] = []
        var trainset_dynamics:Array[MaszynaDynamicData] = trainset_data.get_arranged_dynamics(root.trainset_override)
        for dynamic:MaszynaDynamicData in trainset_dynamics:
            var vehicle:RID = MaszynaLegacyVehicleSystem.vehicle_create(dynamic, root.get_instance_id())
            root._vehicle_rids.append(vehicle)
            trainset_vehicles.append(vehicle)
        placed.append(trainset_data)
        tracks.append(track)
        vehicles.append(trainset_vehicles)
        dynamics.append(trainset_dynamics)
    if placed.size() < trainsets.size():
        print("[SceneryLoad] %d trainsets on tracks that are not built (roads) left out" % (trainsets.size() - placed.size()))

    var vehicle_count:int = 0
    for trainset_vehicles:Array[RID] in vehicles:
        vehicle_count += trainset_vehicles.size()
    var built_count:int = 0
    for index:int in placed.size():
        var trainset_data:MaszynaTrainsetData = placed[index]
        var trainset_vehicles:Array[RID] = vehicles[index]
        for vehicle:RID in trainset_vehicles:
            # reported before the wait: the trainset stands, and its driver is given its orders, in
            # the frame its last vehicle is built
            await _report_progress_throttled(
                    root, lerpf(VEHICLES_PROGRESS, 1.0, float(built_count) / vehicle_count),
                    MaszynaIncludeNode.LoadStage.VEHICLES, "Instancing vehicles")
            if _is_load_given_up(root):
                return
            while not MaszynaLegacyVehicleSystem.vehicle_is_built(vehicle):
                await MaszynaLegacyVehicleSystem.vehicle_built
                if _is_load_given_up(root):
                    return
            built_count += 1
        var trainset:RID = RailVehicleServer.trainset_create()
        root._trainset_rids.append(trainset)
        RailVehicleServer.trainset_set_name(trainset, trainset_data.name)
        RailVehicleServer.trainset_set_track(trainset, tracks[index], trainset_data.offset)
        # a vehicle that failed to load has no simulation, and the trainset stands without it
        var members:Array[int] = []
        for member:int in trainset_vehicles.size():
            if not VehicleServer.vehicle_is_simulation_ready(trainset_vehicles[member]):
                continue
            var dynamic:MaszynaDynamicData = dynamics[index][member]
            RailVehicleServer.trainset_add_vehicle(
                    trainset, trainset_vehicles[member], dynamic.direction, dynamic.gap, dynamic.coupling)
            members.append(member)
        RailVehicleServer.trainset_place(trainset)
        # the person the scenery seats at the controls of a vehicle (MaszynaLegacyVehicleSystem)
        # thinks as the original's driver - that person, whatever role it has by now: a player who
        # took the vehicle over first sits at the controls and stays the player. It drives through
        # the vehicle's cab logic, like the player, without the 3D cab
        var trainset_driver:RID = RID()
        for member:int in members:
            var driver:RID = MaszynaLegacyVehicleSystem.vehicle_get_driver(trainset_vehicles[member])
            if driver.is_valid():
                trainset_driver = driver
                DriverServer.driver_attach_implementation(trainset_driver, DRIVER_IMPLEMENTATION)
        # endtrainset (simulationstateserializer.cpp:839-848): the trainset's driver gets its
        # timetable and the velocity it starts with; of several drivers, the one furthest along
        if trainset_driver.is_valid() and trainset_data.timetable:
            DriverServer.driver_send_command(
                    trainset_driver, TIMETABLE_ORDER + trainset_data.timetable,
                    trainset_data.velocity, 0.0)
    root.load_progress.emit(1.0, MaszynaIncludeNode.LoadStage.VEHICLES, "")


static func _instantiate_server_data(
    root:MaszynaIncludeNode,
    world_3d:World3D,
    tracks:Array[MaszynaTrackData],
    traction:Array[MaszynaTractionData],
    power_sources:Array[MaszynaPowerSourceData],
    models:Array[MaszynaModelData],
    events:Array[MaszynaEventData],
    memcells:Array[MaszynaMemcellData],
    launchers:Array[MaszynaEventLauncherData],
    sounds:Array[MaszynaSoundData],
    isolated_sections:Array[MaszynaIsolatedData],
    progress_from:float,
    progress_to:float,
) -> void:
    var total:float = float(maxi(tracks.size() + power_sources.size() + traction.size() + models.size(), 1))
    var built_count:int = 0
    # in the order of the data, which is how the events find what they are aimed at
    var track_rids:Array[RID] = []
    var model_rids:Array[RID] = []
    for track_data:MaszynaTrackData in tracks:
        var built:Dictionary = _build_track(track_data, world_3d)
        root._track_rids.append(built["track_rid"])
        track_rids.append(built["track_rid"])
        root._track_render_rids.append(built["track_render_rid"])
        built_count += 1
        await _report_progress_throttled(root, lerpf(progress_from, progress_to, built_count / total), MaszynaIncludeNode.LoadStage.INFRASTRUCTURE, "Registering tracks")
        if _is_load_given_up(root):
            return

    for power_source_data:MaszynaPowerSourceData in power_sources:
        root._power_source_rids.append(_build_power_source(power_source_data))
        built_count += 1
    for traction_data:MaszynaTractionData in traction:
        var traction_rid:RID = _build_traction(traction_data, world_3d)
        root._traction_rids.append(traction_rid)
        root._wire_power_rids.append(_build_wire_power(traction_data))
        built_count += 1
        await _report_progress_throttled(root, lerpf(progress_from, progress_to, built_count / total), MaszynaIncludeNode.LoadStage.INFRASTRUCTURE, "Registering traction")
        if _is_load_given_up(root):
            return
    if root._wire_power_rids.size() > 0:
        TractionServer.network_build()

    if root._track_rids.size() > 0:
        TrackServer.topology_rebuild()

    # the original's signal heads: every lit model, driven by the scenery's own `lights` events
    var signalling_system:RID = SignallingServer.system_create()
    SignallingServer.system_attach_implementation(signalling_system, MaszynaLegacySignallingImplementation.new())
    SignallingServer.system_set_name(signalling_system, root.filename)
    root._signalling_system_rids.append(signalling_system)
    for model_data:MaszynaModelData in models:
        var e3d_rid:RID = _build_model(model_data, world_3d, signalling_system)
        model_rids.append(e3d_rid)
        if e3d_rid.is_valid():
            root._e3d_rids.append(e3d_rid)
        built_count += 1
        await _report_progress_throttled(
            root, lerpf(progress_from, progress_to, built_count / total), MaszynaIncludeNode.LoadStage.INFRASTRUCTURE,
            TranslationServer.translate("Registering %s") % model_data.model_filename
        )
        if _is_load_given_up(root):
            return
    # the original's firstinit: everything the events are aimed at exists by now
    await MaszynaLegacyEventFactory.build(
        root, events, memcells, launchers, sounds, isolated_sections, tracks, track_rids, models, model_rids,
        power_sources
    )


## Registers the terrain chunks with MaszynaSceneryChunkRenderingServer; they are rendered, and their
## geometry read from its file, only while the camera is within range of their chunk (see the
## server's doc comment for why)
static func _build_triangle_chunks(
    root:MaszynaIncludeNode, chunks:Array[MaszynaTrianglesChunkData], world_3d:World3D
) -> void:
    for chunk:MaszynaTrianglesChunkData in chunks:
        root._triangle_chunk_rids.append(MaszynaSceneryChunkRenderingServer.create_chunk(
            chunk, world_3d.scenario, ResourceLoader.load.bind(chunk.geometry_path)
        ))
        await frame_budget_wait()


## Adds objects to root, reporting the object being attached (see _report_progress_throttled())
## with progress going from progress_from to progress_to.
static func _attach_objects(
    root:MaszynaIncludeNode, objects:Array, progress_from:float, progress_to:float
) -> void:
    for i:int in objects.size():
        var node:Node = objects[i] as Node
        if not node:
            continue
        root.add_child(node)
        var progress:float = lerpf(progress_from, progress_to, float(i) / float(objects.size()))
        await _report_progress_throttled(root, progress, MaszynaIncludeNode.LoadStage.OBJECTS, TranslationServer.translate("Instancing %s") % node.name)
        if _is_load_given_up(root):
            return


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

    var packed_scene := PackedScene.new()
    var result:Error = packed_scene.pack(scene_root)

    for child:Node in scene_root.get_children():
        child.owner = null
        scene_root.remove_child(child)
    scene_root.free()

    if not result == OK:
        push_error("Cannot compile scenery cache")
        return null
    return packed_scene


static func _instantiate_cached_nodes(packed_scene:PackedScene) -> Array:
    var objects:Array = []
    if not packed_scene:
        return objects
    var scene_root:Node = packed_scene.instantiate()
    for child:Node in scene_root.get_children():
        scene_root.remove_child(child)
        child.owner = null
        objects.append(child)
    scene_root.free()
    return objects


## Runs `task` on a WorkerTaskQueue worker while the main thread keeps drawing frames with
## the progress reported; its result, null when the load is cancelled (cancel_loading())
static func _run_on_worker(
    root:MaszynaIncludeNode, task:Callable, progress:float, stage:MaszynaIncludeNode.LoadStage, message:String
) -> Variant:
    var queue := WorkerTaskQueue.new()
    _active_queues.append(queue)
    var task_id:int = queue.submit(task)
    while not queue.is_done(task_id):
        await _report_progress(root, progress, stage, message)
    _active_queues.erase(queue)
    return queue.wait(task_id)


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
    var scenery_dir:String = UserSettings.get_maszyna_game_dir().path_join("scenery")
    return scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, filename)).simplify_path()


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
## in world coordinates - and whether a region file holds the terrain, which leaves its shapes out.
## The subscene's terrain is chunked in a sink of its own, cached with it,
## and added to the scenery's sink.
func parse_subscene_task(
    filename:String, parameters:Dictionary, state:Dictionary, queue:WorkerTaskQueue
) -> MaszynaImporterContext:
    var source_path:String = _get_source_path(filename)
    var state_hash:String = var_to_str(
        [state["origin"], state["rotate"], state["binary_terrain"], state["binary_terrain_state"]]
    ).md5_text()
    var cache_path:String = _get_cache_path(source_path, state_hash)
    var scenery_sink:SceneryTrianglesSink = state["triangles_sink"]
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
        cached.trainsets.assign(compiled.trainsets)
        cached.region_files.assign(compiled.region_files)
        for chunk_path:String in compiled.triangle_chunk_paths:
            scenery_sink.add_geometry_file(chunk_path)
        cached.dependencies = compiled.dependencies.duplicate(true)
        cached.objects = _instantiate_cached_nodes(compiled.nodes)
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
    var subscene_sink:SceneryTrianglesSink = SceneryTrianglesSink.create(
        "" if is_saving else _cache.get_file_path(cache_path.get_basename()), SUBSCENE_TRIANGLES_BUDGET_BYTES
    )
    state["triangles_sink"] = subscene_sink
    var context:MaszynaImporterContext = parse_file_task(filename, parameters, state, queue)
    if is_saving:
        for geometry:MaszynaTrianglesChunkGeometry in subscene_sink.get_geometries():
            scenery_sink.add_geometry(geometry)
        return context

    var chunk_paths:PackedStringArray = []
    for descriptor:Dictionary in subscene_sink.finish():
        chunk_paths.append(descriptor["path"])
        scenery_sink.add_geometry_file(descriptor["path"])
    if context.cacheable:
        var subscene := MaszynaCompiledSubscene.new()
        subscene.triangle_chunk_paths = chunk_paths
        if _compile_scenery(source_path, state_hash, context, context.objects, subscene):
            _cache.set(cache_path, subscene)
    _saving_subscenes_mutex.lock()
    _saving_subscenes.erase(cache_path)
    _saving_subscenes_mutex.unlock()
    return context


## Parses root's scenery on WorkerTaskQueue workers (every include is a task), reporting
## progress every frame: finished tasks / tasks submitted so far.
func _parse_file_with_progress(
    root:MaszynaIncludeNode, filename:String, parameters:Dictionary, triangles_sink:SceneryTrianglesSink
) -> MaszynaImporterContext:
    var root_context := MaszynaImporterContext.new()
    root_context.rotate = root.context_rotate
    root_context.origin = root.context_origin
    root_context.triangles_sink = triangles_sink
    var queue := WorkerTaskQueue.new()
    _active_queues.append(queue)
    # the scenario's region file first: what is parsed after it depends on whether it is there
    root_context.load_scenario_binary_terrain(filename)
    var task_id:int = queue.submit(parse_file_task.bind(filename, parameters, root_context.get_state(), queue))
    # every include is a task submitted while parsing, so the total grows with the parse; the bar
    # does not go back when it does
    var parsed:float = 0.0
    while not queue.is_done(task_id):
        parsed = maxf(parsed, float(queue.get_completed_count()) / float(queue.get_submitted_count()))
        _file_in_parse_mutex.lock()
        var file_in_parse:String = _file_in_parse
        _file_in_parse_mutex.unlock()
        root.load_files_parsed.emit(queue.get_completed_count(), file_in_parse)
        await _report_progress(
            root, PARSE_PROGRESS * parsed, MaszynaIncludeNode.LoadStage.FILES, tr("Parsing %s") % filename
        )
    var context:MaszynaImporterContext = queue.wait(task_id) as MaszynaImporterContext
    _active_queues.erase(queue)
    _include_buffers_mutex.lock()
    _include_buffers.clear()
    _include_buffers_mutex.unlock()
    if not context:
        # a drained queue (cancel_loading()) is a load given up, not one that failed
        if not _is_load_given_up(root):
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


## Mirrors TrackNormal3D/TrackSwitch3D's own _create_track()/_update_track_data()/
## _update_track_rendering() (addons/libmaszyna/tracks/track_normal_3d.gd,
## track_switch_3d.gd), minus the Node - see instantiate()'s doc comment for why. The rendering
## meshes are not built here: stream_track() leaves that to SceneryStreamingServer.
static func _build_track(track_data:MaszynaTrackData, world_3d:World3D) -> Dictionary:
    var track_rid:RID = TrackServer.track_create()
    var track_render_rid:RID = TrackRenderingServer.create_track(track_rid)
    TrackRenderingServer.set_track_scenario(track_render_rid, world_3d.scenario)

    TrackServer.track_update_curves(track_rid, track_data.curve, track_data.diverging_curve)
    TrackServer.track_update(track_rid, track_data.type, track_data.track_name, track_data.width)
    TrackServer.track_update_properties(
            track_rid, track_data.quality_flag, track_data.environment, track_data.sound_distance)
    if track_data.type == TrackServer.TRACK_SWITCH:
        TrackServer.switch_set_active_track(track_rid, TrackServer.TRACK_COMMON)
    # the speed limit a `trackvel` event changes (Track.cpp:851-858)
    TrackServer.track_set_velocity(track_rid, float(track_data.parameters.get("velocity", -1.0)))

    TrackRenderingServer.set_track_render_options(
        track_render_rid,
        track_data.tex_length,
        track_data.tex_height,
        track_data.tex_width,
        track_data.tex_slope,
        track_data.material1,
        track_data.material2,
        track_data.parameters.get("trackbed", ""), # optional switch attribute, Track.cpp:2378
        track_data.railprofile,
        true, # rail_visible
        true, # ballast_visible
    )
    TrackRenderingServer.set_track_visible(track_render_rid, track_data.visible)
    TrackRenderingServer.stream_track(track_render_rid)

    return {"track_rid": track_rid, "track_render_rid": track_render_rid}


## Mirrors Traction3D's own _enter_tree()/_update() (addons/libmaszyna/traction/
## traction_3d.gd), minus the Node. contact_p1/p2/support_p1/p2 are already absolute
## world coordinates read straight from the .scn (same as track curve points), so the traction's
## own transform is identity - there's nothing local left to place.
static func _build_traction(traction_data:MaszynaTractionData, world_3d:World3D) -> RID:
    var traction_rid:RID = TractionRenderingServer.create_traction()
    TractionRenderingServer.set_traction_geometry(
        traction_rid,
        traction_data.contact_p1,
        traction_data.contact_p2,
        traction_data.support_p1,
        traction_data.support_p2,
        traction_data.wire_thickness,
        traction_data.wires,
        traction_data.wire_offset,
        traction_data.min_height,
        traction_data.segment_length,
    )
    TractionRenderingServer.set_traction_transform(traction_rid, Transform3D.IDENTITY)
    TractionRenderingServer.set_traction_visible(traction_rid, traction_data.visible)
    TractionRenderingServer.set_traction_scenario(traction_rid, world_3d.scenario)
    TractionRenderingServer.set_traction_material(traction_rid, _get_traction_material(traction_data).get_rid())
    TractionRenderingServer.stream_traction(traction_rid)
    return traction_rid


## Registers the placement with E3DRenderingServer instead of instancing it (a real scenery places
## hundreds of thousands of submodels - instancing them all at load costs both the loading time and
## the frame rate). The server loads the model and builds the instance once the streaming camera
## comes within the node's range of the chunk it falls into, and clears it when the camera leaves.
## The original's signal heads have no kind: a model shows whatever `lights` events the scenery aims
## at it (Event.cpp:1741-1803). Every lit model, and every model such an event is aimed at, is
## given a kind made of those events (MaszynaLegacySignalHeadKindFactory); the copies of one include
## end up with equal kinds, which are then one resource. A lit model no event reaches gets the
## generic kind. Target names are matched in lower case, as the original reads them (Event.cpp:327).
static func assign_signal_head_kinds(
    models:Array[MaszynaModelData], scenery_events:Array[MaszynaEventData]
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
    for model_data:MaszynaModelData in models:
        var events:Array = events_by_target.get(model_data.name.to_lower(), []) if model_data.name else []
        if not events:
            model_data.signal_head_kind = GENERIC_SIGNAL_HEAD_KIND if model_data.lights else null
            continue
        var aspects:Dictionary = {}
        for event:MaszynaEventData in events:
            # one TAnimModel::LightSet() value per light, light 0 first
            var values:PackedFloat32Array = []
            for value:String in event.parameters:
                values.append(float(value))
            aspects[MaszynaLegacySignalHeadKindFactory.get_aspect_name(event.name, model_data.name)] = values
        var key:String = var_to_str(aspects)
        if not kinds.has(key):
            kinds[key] = MaszynaLegacySignalHeadKindFactory.create_kind(aspects)
        model_data.signal_head_kind = kinds[key]


static func _build_model(model_data:MaszynaModelData, world_3d:World3D, signalling_system:RID) -> RID:
    var model_rid:RID = E3DRenderingServer.instance_register(
        model_data.data_path,
        model_data.model_filename,
        model_data.skins,
        Transform3D(Basis.from_euler(model_data.rotation), model_data.position),
        model_data.range_min,
        model_data.range_max,
        world_3d.scenario,
    )
    # the declared modes outlive the streaming, so they are set once here and not on every build
    if model_data.lights:
        E3DRenderingServer.instance_set_lights_modes(model_rid, model_data.lights)
    if model_data.signal_head_kind:
        # goes with the instance when the include frees it
        var signal_head:RID = SignallingServer.signal_head_create(model_rid)
        SignallingServer.signal_head_set_kind(signal_head, model_data.signal_head_kind)
        SignallingServer.signal_head_set_name(signal_head, model_data.name)
        SignallingServer.system_add_signal_head(signalling_system, signal_head)
    if model_data.light_colors:
        E3DRenderingServer.instance_set_lights_colors(model_rid, model_data.light_colors)
    return model_rid


## Mirrors _build_traction() - a tractionpowersource node has no visual representation, so this
## only ever registers electrical data against TractionServer.
static func _build_power_source(power_source_data:MaszynaPowerSourceData) -> RID:
    var power_source_rid:RID = TractionServer.power_source_create()
    TractionServer.power_source_set_params(
        power_source_rid,
        power_source_data.name,
        power_source_data.nominal_voltage,
        power_source_data.voltage_frequency,
        power_source_data.internal_resistance,
        power_source_data.max_output_current,
        power_source_data.fast_fuse_timeout,
        power_source_data.fast_fuse_repetition,
        power_source_data.slow_fuse_timeout,
        power_source_data.recuperation,
        false,
        power_source_data.is_section,
    )
    return power_source_rid


## Registers a traction wire's electrical data (as opposed to _build_traction()'s visual mesh)
## against TractionServer - a second, purely-electrical RID for the same wire span.
static func _build_wire_power(traction_data:MaszynaTractionData) -> RID:
    var wire_rid:RID = TractionServer.wire_create()
    TractionServer.wire_set_params(
        wire_rid,
        traction_data.contact_p1,
        traction_data.contact_p2,
        traction_data.power_supply_name,
        traction_data.nominal_voltage,
        traction_data.max_current,
        traction_data.resistivity,
    )
    # the span this one shares its running with, which a pantograph cannot reach along the chain
    TractionServer.wire_set_parallel(wire_rid, traction_data.parallel)
    return wire_rid


static func _get_traction_material(traction_data:MaszynaTractionData) -> Material:
    var is_copper:bool = traction_data.material == 0 # TractionMaterial.COPPER
    if traction_data.damage_flag & Traction3D.DamageFlag.PATINA:
        return (
            Traction3D.TRACTION_CU_PATINA_MATERIAL if is_copper
            else Traction3D.TRACTION_AL_PATINA_MATERIAL
        )
    return Traction3D.TRACTION_CU_MATERIAL if is_copper else Traction3D.TRACTION_AL_MATERIAL
