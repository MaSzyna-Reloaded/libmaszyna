extends RefCounted
class_name MaszynaImporterContext

## An include submitted to [member queue]: its task and where its result belongs in this file
class PendingInclude:
    var task_id:int = -1
    ## Sizes of the result lists when the include was reached
    var sizes:Dictionary[String, int] = {}

const RESULT_LISTS:Array[String] = [
    "tracks", "traction", "power_sources", "models", "events", "memcells", "launchers", "sounds", "isolated_sections", "terrains",
    "scripts", "region_files", "trainsets"
]

## The region file's extension (EU07_FILEEXTENSION_REGION, scene.cpp:27)
const REGION_EXTENSION:String = ".sbt"
## What Rainsted prefixes a scenario it modified with, and the base scenario it writes
const RAINSTED_PREFIX:String = "$"
const RAINSTED_SCENARIO:String = "$.scn"

var _states: Array[Dictionary] = []
var include_depth: int = 0
## Number of cached subscenes (SceneryInstancer.parse_subscene_task()) enclosing the parsed file
var subscene_depth:int = 0
var rotate := Vector3.ZERO
var origin := Vector3.ZERO
var tracks:Array[MaszynaTrackData] = []
var traction:Array[MaszynaTractionData] = []
var power_sources:Array[MaszynaPowerSourceData] = []
var models:Array[MaszynaModelData] = []
var events:Array[MaszynaEventData] = []
var memcells:Array[MaszynaMemcellData] = []
var launchers:Array[MaszynaEventLauncherData] = []
var sounds:Array[MaszynaSoundData] = []
var isolated_sections:Array[MaszynaIsolatedData] = []
## The trainsets, each `dynamic` outside one a trainset of its own, in the order of the file
var trainsets:Array[MaszynaTrainsetData] = []
## The `lua` scripts, relative to the scenery directory
var scripts:Array[String] = []
## The region files (.sbt) whose terrain the scenery is drawn with (MaszynaLegacySBTTerrainProvider)
var region_files:Array[String] = []
var terrains: Array = []
## Where the "triangles" nodes go as they are parsed - shared by the includes of one scenery, a
## subscene's own (SceneryInstancer.parse_subscene_task())
var triangles_sink:SceneryTrianglesSink = SceneryTrianglesSink.create("")
## The original's Scratchpad.binary.terrain: the scenery's shapes come from a region file (.sbt),
## so its "triangles" nodes and terrain models are left out (simulationstateserializer.cpp:503-600).
## Like the original's, it holds for the rest of the load from where it is set: push_state() and
## pop_state() leave it, an include inherits it (load_binary_terrain())
var binary_terrain:bool = false
## The original's Global.file_binary_terrain_state: includes of "_ter.scm" files are left out
## (parser.cpp:330)
var binary_terrain_state:bool = false
var dependencies:Dictionary = {}
var cacheable:bool = true
## Objects parsed from the file (set by SceneryInstancer.parse_file_task())
var objects:Array = []
## When set, "include" is parsed as a task of this queue instead of in place
## (see submit_include()/merge_pending_includes())
var queue:WorkerTaskQueue = null

## The trainset "trainset:" opened and "endtrainset:" has not closed yet - the `dynamic`s read
## meanwhile are its vehicles (scene::scratch_data::trainset_data, simulationstateserializer.h).
## An include reached meanwhile is parsed in place (maszyna_include_importer.gd), so its vehicles
## join the trainset where the include stands.
var trainset:MaszynaTrainsetData = null

var _rotates = []
var _origins = []
var _active_files:Dictionary = {}


func register_dependency(path:String, size:int = -1) -> void:
    var normalized_path:String = path.simplify_path()
    if not FileAccess.file_exists(normalized_path):
        cacheable = false
        return
    var dependency_size:int = size
    if dependency_size < 0:
        var file:FileAccess = FileAccess.open(normalized_path, FileAccess.READ)
        if not file:
            cacheable = false
            return
        dependency_size = file.get_length()
    dependencies[normalized_path] = {
        "modified_time": FileAccess.get_modified_time(normalized_path),
        "size": dependency_size,
    }


func begin_file(path:String) -> bool:
    if _active_files.has(path):
        cacheable = false
        return false
    _active_files[path] = true
    return true


func end_file(path:String) -> void:
    _active_files.erase(path)

## The scenery's terrain from here on is the region file's: `terrain <file>.sbt` in the scenery
## (state_serializer::deserialize_terrain(), simulationstateserializer.cpp:786), or the scenario's
## own <scenario>.sbt from the start (:51-66). Its shapes are not read here: the scenery supplies
## them as the camera comes near (MaszynaLegacySBTTerrainProvider)
func load_binary_terrain(path:String) -> void:
    binary_terrain = MaszynaLegacySBTTerrainProvider.is_region(path)
    binary_terrain_state = true
    if binary_terrain:
        register_dependency(path)
        region_files.append(path)


## The scenario's own region file, <scenario>.sbt beside it, holds its terrain from the start
## (simulationstateserializer.cpp:51-66). basic_region::is_scene() trims Rainsted's leading "$",
## and the "$.scn" Rainsted writes has none (scene.cpp:1113)
func load_scenario_binary_terrain(scenario:String) -> void:
    if scenario == RAINSTED_SCENARIO:
        return
    var scenery_dir:String = UserSettings.get_maszyna_game_dir().path_join("scenery")
    var region_file:String = scenario.lstrip(RAINSTED_PREFIX).get_basename() + REGION_EXTENSION
    var path:String = scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, region_file))
    if MaszynaLegacySBTTerrainProvider.is_region(path):
        load_binary_terrain(path)


func push_rotate(new_rotate: Vector3):
    _rotates.push_front(rotate)
    rotate = new_rotate

func pop_rotate():
    rotate = _rotates.pop_front()

## An origin inside an origin adds to it (simulationstateserializer.cpp:651)
func push_origin(offset: Vector3):
    _origins.push_front(origin)
    origin += offset
    
func pop_origin():
    if _origins.size() > 0:
        origin = _origins.pop_front()


## State inherited by an included file (see from_state())
func get_state() -> Dictionary:
    return {
        "include_depth": include_depth,
        "subscene_depth": subscene_depth,
        "rotate": rotate,
        "origin": origin,
        "triangles_sink": triangles_sink,
        "binary_terrain": binary_terrain,
        "binary_terrain_state": binary_terrain_state,
        "active_files": _active_files.duplicate(),
    }


static func from_state(state:Dictionary) -> MaszynaImporterContext:
    var context := MaszynaImporterContext.new()
    context.include_depth = state["include_depth"]
    context.subscene_depth = state["subscene_depth"]
    context.rotate = state["rotate"]
    context.origin = state["origin"]
    context.triangles_sink = state["triangles_sink"]
    context.binary_terrain = state["binary_terrain"]
    context.binary_terrain_state = state["binary_terrain_state"]
    context._active_files = state["active_files"]
    return context


## Submits task(filename, parameters, state, queue) -> MaszynaImporterContext parsing an included
## file; returns the placeholder that stands in the parsed objects until merge_pending_includes()
func submit_include(task:Callable, filename:String, parameters:Dictionary) -> PendingInclude:
    var pending := PendingInclude.new()
    for list_name:String in RESULT_LISTS:
        pending.sizes[list_name] = (get(list_name) as Array).size()
    var state:Dictionary = get_state()
    state["include_depth"] = include_depth + 1
    # one bind() - chained binds prepend the later arguments
    pending.task_id = queue.submit(task.bind(filename, parameters, state, queue))
    return pending


## Waits for the submitted includes and puts their results where the includes were in the file
func merge_pending_includes() -> void:
    var pendings:Array[PendingInclude] = []
    var children:Array[MaszynaImporterContext] = []
    var own_objects:Array = []
    var object_sizes:Array[int] = []
    for object:Variant in objects:
        if object is PendingInclude:
            pendings.append(object)
            object_sizes.append(own_objects.size())
        else:
            own_objects.append(object)
    if not pendings:
        return

    for pending:PendingInclude in pendings:
        var child:MaszynaImporterContext = queue.wait(pending.task_id) as MaszynaImporterContext
        children.append(child)
        if not child:
            cacheable = false
            continue
        dependencies.merge(child.dependencies)
        cacheable = cacheable and child.cacheable

    objects = _merge_list(own_objects, object_sizes, children, "objects")
    for list_name:String in RESULT_LISTS:
        var sizes:Array[int] = []
        for pending:PendingInclude in pendings:
            sizes.append(pending.sizes[list_name])
        (get(list_name) as Array).assign(_merge_list(get(list_name), sizes, children, list_name))


## own split at sizes, with the children's list_name inserted in between
static func _merge_list(
    own:Array, sizes:Array[int], children:Array[MaszynaImporterContext], list_name:String
) -> Array:
    var merged:Array = []
    var start:int = 0
    for i:int in sizes.size():
        merged.append_array(own.slice(start, sizes[i]))
        if children[i]:
            merged.append_array(children[i].get(list_name))
        start = sizes[i]
    merged.append_array(own.slice(start))
    return merged


func push_state() -> void:
    _states.push_front({
        "include_depth": include_depth,
        "rotate": rotate,
        "origin": origin,
        "rotates_size": _rotates.size(),
        "origins_size": _origins.size(),
        "trainset": trainset,
    })


func pop_state() -> void:
    if not _states:
        return

    var state: Dictionary = _states.pop_front()
    include_depth = state["include_depth"]
    rotate = state["rotate"]
    origin = state["origin"]
    trainset = state["trainset"]

    while _rotates.size() > state["rotates_size"]:
        _rotates.pop_front()

    while _origins.size() > state["origins_size"]:
        _origins.pop_front()
