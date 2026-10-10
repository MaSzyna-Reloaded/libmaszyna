@tool
extends Node3D
class_name MaszynaIncludeNode

signal loaded
## What a load is doing, in the order SceneryInstancer goes through it: the scenery's files read
## (parsed, or its cache), tracks, traction and models registered, terrain built, objects
## instanced, vehicles built - and then, the game's own, the player's surroundings built by the
## streaming around the player's camera
enum LoadStage { FILES, INFRASTRUCTURE, TERRAIN, OBJECTS, VEHICLES, SURROUNDINGS }
## Emitted by SceneryInstancer as the load goes (progress 0..1 of the whole load, its stage, what
## it is doing now)
signal load_progress(progress:float, stage:LoadStage, message:String)
## Emitted by SceneryInstancer while the files are parsed: includes parsed so far and the file being
## parsed now - the progress of the files stage alone cannot tell a large include from a stall
signal load_files_parsed(count:int, filename:String)
## A load() has ended, whatever came of it: loaded, given up, or nothing to load
signal load_ended
## The content is about to be freed (a reload, an unload): whatever runs on it - its scenario,
## MaszynaLegacyScenario - stops now, while everything it reaches still exists
signal unloading
## Emitted as the content is freed over frames (load(), clear()): the share of it freed so far
signal clear_progress(progress:float)
## The content freed over frames is gone - a load() goes on loading
signal cleared

## Milliseconds spent freeing content per frame while reloading
const CLEAR_BUDGET_MSEC:int = 8

var _dirty:bool = false
var _editor_dirty:bool = false

const SceneryEditor = preload("res://addons/libmaszyna/editor/scenery_toolbar/scenery_editor.gd")

## Changing filename does not reload the content - call load() explicitly.
@export var filename:String = ""
@export var parameters:Dictionary = {}
@export var context_rotate: Vector3 = Vector3.ZERO
@export var context_origin: Vector3 = Vector3.ZERO
@export var autoload:bool = true
@export var use_cache:bool = true
## The vehicles of one trainset as the player arranged them for this load (the scenery
## selector), front first - MaszynaTrainsetData.get_arranged_dynamics(). The scenery cache holds
## what the .scn declares; this only replaces it for this load. Empty: the trainsets as declared.
@export var trainset_override:Array[MaszynaDynamicData] = []

## Off by default: loaded content gets no owner and stays unselectable in the editor (matches
## E3DModelInstance/MaszynaRailVehiclePhysicsNode's own default). Toggle via the "Edit SCN" editor toolbar
## button (addons/libmaszyna/editor/scenery_toolbar/) to make it inspectable/selectable while
## authoring - see scenery_instancer.gd's attach loop for what this actually changes.
@export var editable_in_editor:bool = false:
    set(x):
        if not x == editable_in_editor:
            editable_in_editor = x
            _editor_dirty = true

var _loading:bool = false
## The load under way was given up (stop_loading())
var _load_given_up:bool = false
## The content of the file is here - the game's data read again loads it again
var _loaded:bool = false

## Tracks, traction and models are built directly against TrackServer/TrackRenderingServer/
## TractionRenderingServer/TractionServer/E3DRenderingServer/SignallingServer RIDs, not as scene nodes (see
## scenery_instancer.gd's instantiate()
## doc comment for why) - so unlike triangle children, they aren't cleaned up just by
## removing children from the tree. scenery_instancer.gd appends the RIDs it creates here;
## _free_owned_rids() releases them all, called before every reload and when the node is freed.
var _track_rids:Array[RID] = []
var _track_render_rids:Array[RID] = []
var _traction_rids:Array[RID] = []
var _wire_power_rids:Array[RID] = []
var _power_source_rids:Array[RID] = []
var _e3d_rids:Array[RID] = []
var _signalling_system_rids:Array[RID] = []
var _triangle_chunk_rids:Array[RID] = []
## SceneryStreamingServer's providers of the scenery's region files
var _provider_rids:Array[RID] = []
var _launcher_rids:Array[RID] = []
var _pickable_rids:Array[RID] = []
var _event_rids:Array[RID] = []
var _memory_rids:Array[RID] = []
var _event_track_rids:Array[RID] = []
var _isolated_rids:Array[RID] = []
var _event_isolated_rids:Array[RID] = []
var _trainset_rids:Array[RID] = []
## The scenery's vehicles (MaszynaLegacyVehicleSystem), in the order of its file
var _vehicle_rids:Array[RID] = []
## What the scenario starts (MaszynaLegacyScenario) - loaded here, run only by the game: the
## scenery's `lua` scripts and its sounds
var _scenario_scripts:Array[String] = []
var _scenery_sounds:MaszynaLegacyScenerySounds = null

## Initial loading (autoload) is deferred to the first _process.
func _ready() -> void:
    _dirty = autoload


func _enter_tree() -> void:
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)
    VehicleServer.vehicle_freed.connect(_on_vehicle_freed)


## A vehicle of the scenery freed by somebody else (the HUD removes a trainset) is not the
## scenery's to free any more
func _on_vehicle_freed(vehicle:RID) -> void:
    _vehicle_rids.erase(vehicle)


## A loaded scenery is the game's data - its tracks, events, drivers and vehicles are loaded again,
## from the data read again, once the load under way (if any) is done
func _on_data_reload_requested() -> void:
    if _loaded or _loading:
        _dirty = true


func _exit_tree() -> void:
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)
    VehicleServer.vehicle_freed.disconnect(_on_vehicle_freed)
    # a load awaits frames of the tree it has just left
    if _loading:
        SceneryInstancer.cancel_loading()


## Out of the tree the content stays, out of the world: the editor takes a scene out of the tree
## when another one's tab is shown, and all of them draw into one world. It goes with the node.
func _notification(what:int) -> void:
    match what:
        NOTIFICATION_ENTER_WORLD, NOTIFICATION_EXIT_WORLD:
            var scenario:RID = get_world_3d().scenario if what == NOTIFICATION_ENTER_WORLD else RID()
            for rid:RID in _track_render_rids:
                TrackRenderingServer.set_track_scenario(rid, scenario)
            for rid:RID in _traction_rids:
                TractionRenderingServer.set_traction_scenario(rid, scenario)
            for rid:RID in _e3d_rids:
                E3DRenderingServer.instance_set_scenario(rid, scenario)
            for rid:RID in _triangle_chunk_rids:
                MaszynaSceneryChunkRenderingServer.chunk_set_scenario(rid, scenario)
            for rid:RID in _provider_rids:
                SceneryStreamingServer.provider_set_scenario(rid, scenario)
            # a vehicle still waiting for its build is not drawn yet
            for rid:RID in _vehicle_rids:
                if RailVehicleRenderingServer.vehicle_is_attached(rid):
                    RailVehicleRenderingServer.vehicle_set_scenario(rid, scenario)
        NOTIFICATION_PREDELETE:
            unloading.emit()
            # The planning thread calls back into GDScript (the owner's preload) and can be
            # creating rendering resources for the very RIDs freed below. It is stopped and joined
            # here, while the scripts still exist - the server's own destructor runs long after
            # they are gone.
            SceneryInstancer.cancel_loading()
            SceneryStreamingServer.streaming_drain()
            _free_owned_rids()


## The scenery's trainsets (RailVehicleServer), in the order of its file
func get_trainsets() -> Array[RID]:
    return _trainset_rids


## The scenery's vehicles (MaszynaLegacyVehicleSystem), in the order of its file
func get_vehicles() -> Array[RID]:
    return _vehicle_rids


## The scenery's models (E3DRenderingServer instances)
func get_models() -> Array[RID]:
    return _e3d_rids


## The scenery's `lua` scripts, run by its scenario (MaszynaLegacyScenario)
func get_scenario_scripts() -> Array[String]:
    return _scenario_scripts


## The scenery's sounds, made audible by its scenario (MaszynaLegacyScenario); null while nothing
## is loaded
func get_scenery_sounds() -> MaszynaLegacyScenerySounds:
    return _scenery_sounds


## budget_msec > 0 spreads the freeing over frames, so whatever covers the screen (the loading
## spinner) keeps animating; 0 frees everything at once (the node freed)
func _free_owned_rids(budget_msec:int = 0) -> void:
    var groups:Array = [
        [_trainset_rids, RailVehicleServer.trainset_free],
        [_vehicle_rids, MaszynaLegacyVehicleSystem.vehicle_free],
        [_pickable_rids, SceneryHUDMouseServer.pickable_free],
        [_launcher_rids, ScenarioEventServer.launcher_free],
        [_event_rids, ScenarioEventServer.event_free],
        [_memory_rids, ScenarioEventServer.memory_free],
        [_event_track_rids, ScenarioEventServer.track_clear_events],
        [_event_isolated_rids, ScenarioEventServer.isolated_clear_events],
        [_isolated_rids, TrackServer.isolated_free],
        [_track_render_rids, TrackRenderingServer.free_track],
        [_track_rids, TrackServer.track_free],
        [_traction_rids, TractionRenderingServer.free_traction],
        [_wire_power_rids, TractionServer.wire_free],
        [_power_source_rids, TractionServer.power_source_free],
        [_e3d_rids, E3DRenderingServer.instance_free],
        # after the instances, whose signal heads leave the system as they go
        [_signalling_system_rids, SignallingServer.system_free],
        [_provider_rids, SceneryStreamingServer.provider_free],
        [_triangle_chunk_rids, MaszynaSceneryChunkRenderingServer.free_chunk],
    ]
    var total:int = 0
    for group:Array in groups:
        total += (group[0] as Array[RID]).size()
    var freed:int = 0
    var frame_start:int = Time.get_ticks_msec()
    for group:Array in groups:
        var rids:Array[RID] = group[0]
        var free_rid:Callable = group[1]
        # Taken off the list before it is freed, not after the whole loop: the budgeted path
        # awaits a frame in the middle, and freeing the node during that await runs this again
        # when the node is freed over the very same RIDs - a double free.
        while rids.size() > 0:
            var rid:RID = rids.pop_back()
            if rid.is_valid():
                free_rid.call(rid)
            freed += 1
            if budget_msec > 0 and Time.get_ticks_msec() - frame_start >= budget_msec:
                clear_progress.emit(float(freed) / float(total))
                await get_tree().process_frame
                frame_start = Time.get_ticks_msec()


## Nothing in here is worth simulating while it is being torn down, and a real scenery is
## hundreds of vehicles with their cabins and sounds, all running their own _process for the
## seconds the freeing takes. RailVehicleServer steps those vehicles from its own registry,
## outside this subtree, so disabling the subtree alone leaves the heaviest part running until the
## last vehicle is freed - it is stopped here too and restored once the content is gone.
func _clear_content(budget_msec:int = 0) -> void:
    unloading.emit()
    _scenario_scripts.clear()
    _scenery_sounds = null
    process_mode = Node.PROCESS_MODE_DISABLED
    VehicleServer.stepping_set_enabled(false)
    # Streaming builds content on process_frame, and the freeing below yields a frame for its
    # budget - without this it streams new content into the very RIDs being freed, which the
    # RenderingServer reports as "Initializing already initialized RID" and then aborts.
    SceneryStreamingServer.streaming_set_enabled(false)
    # the preloads in flight load the content freed below - they are stopped and joined first, as
    # when the node is freed (NOTIFICATION_PREDELETE)
    SceneryStreamingServer.streaming_drain()
    await _free_owned_rids(budget_msec)
    var frame_start:int = Time.get_ticks_msec()
    for child:Node in get_children(true):
        child.free()
        if budget_msec > 0 and Time.get_ticks_msec() - frame_start >= budget_msec:
            await get_tree().process_frame
            frame_start = Time.get_ticks_msec()
    SceneryStreamingServer.streaming_set_enabled(true)
    VehicleServer.stepping_set_enabled(true)
    process_mode = Node.PROCESS_MODE_INHERIT
    cleared.emit()


## Loads what the node is set to. Asked for while a load is under way, it stops that one
## (stop_loading()) and the scenery is loaded again once it has stopped.
func load() -> void:
    if _loading:
        stop_loading()
        _dirty = true
        return

    _dirty = false
    _loading = true
    _load_given_up = false
    await _clear_content(CLEAR_BUDGET_MSEC)
    _loaded = false
    if filename:
        await _load_content()
        _loaded = not _load_given_up
        # half a scenery is none: what the load given up had built goes
        if _load_given_up:
            await _clear_content(CLEAR_BUDGET_MSEC)
    _loading = false
    load_ended.emit()
    if _loaded:
        loaded.emit()


## Frees what is loaded; the scenery stays set to its file. A load under way is stopped instead
## (stop_loading()), which frees what it had built.
func clear() -> void:
    if _loading:
        stop_loading()
        return
    _loading = true
    await _clear_content(CLEAR_BUDGET_MSEC)
    _loaded = false
    _loading = false


## Gives the load under way up: it stops at its next step (SceneryInstancer), and what it had
## built is freed. Nothing happens while nothing loads.
func stop_loading() -> void:
    if _loading:
        _load_given_up = true
        SceneryInstancer.cancel_loading()


## Whether the load under way was given up (stop_loading())
func is_load_given_up() -> bool:
    return _load_given_up


func _load_content() -> void:
    await SceneryInstancer.instantiate(self, parameters)


func _process(delta: float) -> void:
    if _dirty and not _loading:
        _dirty = false
        _process_dirty(delta)
    if _editor_dirty:
        _editor_dirty = false
        if Engine.is_editor_hint():
            SceneryEditor.update_owners(self)

func _process_dirty(_delta: float) -> void:
    self.load()
