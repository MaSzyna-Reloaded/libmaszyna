@tool
extends MaszynaSceneryContentNode
class_name MaszynaWorld

## A world of the game's own format, described by worlds/<name>.json of the game's directory, of the
## squares of the grid (MaszynaWorldSector, sector_ids) in blocks of BLOCK_SECTORS x BLOCK_SECTORS
## and its scenario's data. Its files are in packs in worlds/<name>/: WORLD_PACK holds what is
## loaded whole - the world's scene, its scenery (the sectors' railways, traction and signals, moved
## from their corners to their places, add_sector()), its scenario - and a pack a block
## (<block>.pck, block_of()) what is drawn of its sectors and their roads, sector by sector. No file of a world
## refers to another of its own: a sector's file holds its roofs' orthophoto, the scenery and the
## scenario are read beside the world's scene by their names (SCENERY_FILE, SCENARIO_FILE) - so its
## files can be anywhere, in the packs or in the scenery editor's sources. What worlds share - the
## terrain's atlas, the facades, the plants - is in packs of its own in the game's ASSETS_DIR
## (common_*.pck), mounted with the world (mount()) and referred to by their paths. The scenery is
## registered the way a .scn's is (SceneryInstancer.instantiate_compiled()), never through
## SceneryInstancer's cache: it is already what a cache would hold. The sectors are loaded around
## the camera (sectors, MaszynaWorldSectors) from the world's sector_source and drawn while loaded
## (MaszynaWorldSectorProvider).

## Side of a square of the grid [m]
const SECTOR_SIZE:float = 1024.0
## Side of a block of the grid, in sectors: a pack each (block_of())
const BLOCK_SECTORS:int = 8
## Where the worlds are, under the game's directory: <name>.json describing a world, its packs in <name>/
const WORLDS_DIR:String = "worlds"
const DESCRIPTION_EXTENSION:String = "json"
const PACK_EXTENSION:String = "pck"
## The pack of what is loaded whole, in the world's directory of packs
const WORLD_PACK:String = "world.pck"
## The .json's "format" the game reads. 5: what worlds share in packs of ASSETS_DIR, the .json's
## "assets"; no file of a world refers to another of its own
const FORMAT:int = 5
## The packs of what worlds share, under the game's directory: <pack>.pck, its files under
## ASSETS_RESOURCES_DIR/<pack>/
const ASSETS_DIR:String = "world_assets"
const ASSETS_RESOURCES_DIR:String = "res://world_assets"
## Where a world's files are in the packs: RESOURCES_DIR/<name>/, its blocks' in BLOCKS_DIR/<block>/
const RESOURCES_DIR:String = "res://worlds"
const BLOCKS_DIR:String = "blocks"
const SECTORS_DIR:String = "sectors"
## The world's scenery (scenery) beside its scene
const SCENERY_FILE:String = "scenery.res"
## The world's scenario (its trainsets...) beside its scene: a MaszynaCompiledScenery
const SCENARIO_FILE:String = "scenario.res"
## The world's far terrain beside its scene: a low-poly mesh of the ground a block
## (far_terrain_file()), in the world's coordinates, lying under the terrain it stands for - drawn
## whole, past the terrain streamed around the camera and alone over the sectors exported as far
## terrain only (whose block packs lack them: MaszynaWorldPackSectorSource)
const FAR_TERRAIN_DIR:String = "far"
## The pack of the far terrain in the world's directory of packs, mounted with the world (mount())
const FAR_TERRAIN_PACK:String = "far.pck"
## What the far terrain is drawn with: its mesh's material within its file (the scenery importer's
## SIFarTerrain), this shader with the block's maps - the ground's colour and its normal
const FAR_TERRAIN_SHADER:Shader = preload("far_terrain.gdshader")
const FAR_TERRAIN_ALBEDO:StringName = &"albedo_map"
const FAR_TERRAIN_NORMALS:StringName = &"normal_map"
## How far the camera sees by default, the far terrain's mountains with it (the project setting
## "maszyna/scenery/far_terrain_distance") [m]
const FAR_TERRAIN_DISTANCE:float = 30000.0
## The project setting of how far the camera sees
const FAR_TERRAIN_DISTANCE_SETTING:String = "maszyna/scenery/far_terrain_distance"

## A sector's files (sector_path())
enum SectorFile {
    ## What is drawn of it: its geometry, decorations and wires (MaszynaWorldSectorProvider)
    DRAWN,
    ## Its Terrain3D region
    TERRAIN,
    ## Its roads (add_sector()): a MaszynaCompiledScenery of their tracks, in the world's
    ## coordinates, registered while the sector is loaded
    ROADS,
}
const SECTOR_FILE_SUFFIXES:Dictionary[SectorFile, String] = {
    SectorFile.DRAWN: ".drawn.scn",
    SectorFile.TERRAIN: ".terrain.res",
    SectorFile.ROADS: ".roads.res",
}

## The roads of the sector `id` (SectorFile.ROADS) are registered and connected among themselves -
## TrackServer's tracks of GROUP_ROAD
signal sector_roads_built(id:String, track_rids:Array[RID])
## The roads of the sector `id` are about to be freed; their RIDs still stand
signal sector_roads_freeing(id:String, track_rids:Array[RID])

## The world's name: its .json and .pck, and its signalling system's (get_scenery_name())
@export var world_name:String = ""
## The projected metres (east, north) of the world's origin - multiples of SECTOR_SIZE
@export var origin:Vector2 = Vector2.ZERO
## The scenario clock the world declares for its start, hours
@export var declared_start_time:float = START_TIME_DEFAULT
## The ids of the world's sectors (MaszynaWorldSector.id)
@export var sector_ids:PackedStringArray = PackedStringArray()

## The sectors' data the simulation needs, in the world's coordinates (add_sector()); read from
## SCENERY_FILE beside the world's scene as it is loaded, unless given before
var scenery:MaszynaCompiledScenery = null
## The world's own data, of no sector: its trainsets, events, memory cells, sounds, scripts; read
## from SCENARIO_FILE beside the world's scene as it is loaded, unless given before
var scenario:MaszynaCompiledScenery = null
## Where the sectors' files come from; set before the world is loaded, the packs of its blocks
## (MaszynaWorldPackSectorSource) without one
var sector_source:MaszynaWorldSectorSource = null
## The sectors loaded around the camera, which what draws them follows
var sectors:MaszynaWorldSectors = MaszynaWorldSectors.new()

var _provider:MaszynaWorldSectorProvider = null
var _provider_rid:RID = RID()
## Sector id -> its roads registered while it is loaded: [Array[RID] of TrackServer, Array[RID] of
## TrackRenderingServer]
var _sector_roads:Dictionary[String, Array] = {}


func get_scenery_name() -> String:
    return world_name


## The description of the world `filename` (worlds/<filename> of the game, written by the importer):
## {format, name, title, description, pck, scene, frame, terrain}; {} when there is none,
## or it is of a FORMAT the game does not read (an import older than the game)
static func read_description(filename:String) -> Dictionary:
    var path:String = UserSettings.get_maszyna_game_dir().path_join(WORLDS_DIR).path_join(filename)
    var description:Variant = JSON.parse_string(FileAccess.get_file_as_string(path)) if FileAccess.file_exists(path) else null
    if not description is Dictionary:
        return {}
    if not int(description.get("format", 0)) == FORMAT:
        push_warning("World %s is of format %d, the game reads %d: import it again" % [
                filename, int(description.get("format", 0)), FORMAT])
        return {}
    return description


## Mounts the packs of what the world `description` describes (read_description()) shares with
## others ("assets", ASSETS_DIR) and its WORLD_PACK, their files under the paths they have in them;
## false when it cannot be - the editor mounts none (Godot's editor has packs off). Its blocks'
## packs are its sector_source's
static func mount(description:Dictionary) -> bool:
    if Engine.is_editor_hint() or not description.has("pck"):
        return false
    for pack:String in description.get("assets", []):
        if not ProjectSettings.load_resource_pack(UserSettings.get_maszyna_game_dir().path_join(ASSETS_DIR).path_join(
                pack + "." + PACK_EXTENSION)):
            push_error("World %s: no pack %s in %s" % [description["name"], pack, ASSETS_DIR])
    var worlds_dir:String = UserSettings.get_maszyna_game_dir().path_join(WORLDS_DIR)
    var far_pack:String = worlds_dir.path_join(String(description["pck"]).get_base_dir()).path_join(FAR_TERRAIN_PACK)
    if FileAccess.file_exists(far_pack) and not ProjectSettings.load_resource_pack(far_pack):
        push_error("World %s: cannot mount %s" % [description["name"], far_pack])
    return ProjectSettings.load_resource_pack(worlds_dir.path_join(description["pck"]))


## The scenario of the world `filename` (worlds/<filename>): its pack mounted, its SCENARIO_FILE
## read; null when it has none
static func read_scenario(filename:String) -> MaszynaCompiledScenery:
    var description:Dictionary = read_description(filename)
    if not description.has("scene"):
        return null
    mount(description)
    var path:String = String(description["scene"]).get_base_dir().path_join(SCENARIO_FILE)
    return load(path) as MaszynaCompiledScenery if ResourceLoader.exists(path) else null


## Where the corner of the square `cell` (MaszynaWorldSector.get_cell()) is in a world whose origin
## is `world_origin` (origin)
static func sector_corner(world_origin:Vector2, cell:Vector2i) -> Vector3:
    return Vector3(world_origin.x - cell.x * SECTOR_SIZE, 0.0, cell.y * SECTOR_SIZE - world_origin.y)


## The cell (MaszynaWorldSector.get_cell()) of the square a point of a world whose origin is
## `world_origin` (origin) is in - sector_corner() turned round
static func cell_at(world_origin:Vector2, position:Vector3) -> Vector2i:
    return Vector2i(floori((world_origin.x - position.x) / SECTOR_SIZE), floori((position.z + world_origin.y) / SECTOR_SIZE))


## The block of the grid the sector `sector_id` is in: <projection>-B<BLOCK_SECTORS>-E<column>-N<row>,
## its sectors' columns and rows divided by BLOCK_SECTORS (e.g. PL92-B8-E0054-N0087)
static func block_of(sector_id:String) -> String:
    var cell:Vector2i = MaszynaWorldSector.cell_of(sector_id)
    return "%s-B%d-E%04d-N%04d" % [sector_id.rsplit("-", true, 2)[0], BLOCK_SECTORS,
            floori(cell.x / float(BLOCK_SECTORS)), floori(cell.y / float(BLOCK_SECTORS))]


## Where the file `file` of the sector `sector_id` is in its world's directory
static func sector_file(sector_id:String, file:SectorFile) -> String:
    return BLOCKS_DIR.path_join(block_of(sector_id)).path_join(SECTORS_DIR).path_join(sector_id + SECTOR_FILE_SUFFIXES[file])


## Where the far terrain of the block `block` (block_of()) is in its world's directory
static func far_terrain_file(block:String) -> String:
    return FAR_TERRAIN_DIR.path_join(block + ".res")


## Where the file `file` of the sector `sector_id` of the world `name` is in the packs
static func sector_path(name:String, sector_id:String, file:SectorFile) -> String:
    return RESOURCES_DIR.path_join(name).path_join(sector_file(sector_id, file))


## The data of `sector` the simulation needs - its railways, traction and signals (the models named
## or lit: a signal's events reach them by name) - taken out of it, moved from its corner to its
## place in the world and added to the world's scenery. Its roads are taken out and returned, moved
## the same way, for the sector's own file (SectorFile.ROADS): they are registered only while the
## sector is loaded, a graph of their own - a world's roads registered whole were tens of thousands
## of tracks to free one by one (2026-10-16, trzebinia). What is left in the sector is what is only
## drawn - its geometry, the other models, the wires (SectorFile.DRAWN): the world draws it around
## the camera
func add_sector(sector:MaszynaWorldSector) -> MaszynaCompiledScenery:
    if not scenery:
        scenery = MaszynaCompiledScenery.new()
    var roads:MaszynaCompiledScenery = MaszynaCompiledScenery.new()
    var corner:Vector3 = sector_corner(origin, sector.get_cell())
    for node:Node in sector.get_children():
        if node is MaszynaTracksNode:
            var taken:MaszynaCompiledScenery = roads if (node as MaszynaTracksNode).type == TrackServer.GROUP_ROAD else scenery
            for track:MaszynaTrackData in (node as MaszynaTracksNode).tracks:
                taken.tracks.append(track.translated(corner))
            sector.remove_child(node)
            node.free()
        elif node is MaszynaTractionNode:
            for span:MaszynaTractionData in (node as MaszynaTractionNode).traction:
                scenery.traction.append(span.translated(corner))
            for source:MaszynaPowerSourceData in (node as MaszynaTractionNode).power_sources:
                scenery.power_sources.append(source.translated(corner))
            sector.remove_child(node)
            node.free()
        elif node is MaszynaModelsNode:
            var drawn:Array[MaszynaModelData] = []
            for model:MaszynaModelData in (node as MaszynaModelsNode).models:
                if model.name or model.lights:
                    scenery.models.append(model.translated(corner))
                else:
                    drawn.append(model)
            (node as MaszynaModelsNode).models = drawn
    return roads


func _has_source() -> bool:
    return scenery != null or not sector_ids.is_empty()


func _instantiate_content() -> void:
    var measurement:SceneryLoadMeasurement = SceneryLoadMeasurement.new(self)
    var compiled:MaszynaCompiledScenery = MaszynaCompiledScenery.new()
    # beside the world's scene, wherever it is (the packs, the scenery editor's sources)
    var dir:String = scene_file_path.get_base_dir()
    if not scenery and dir and ResourceLoader.exists(dir.path_join(SCENERY_FILE)):
        scenery = load(dir.path_join(SCENERY_FILE)) as MaszynaCompiledScenery
    if not scenario and dir and ResourceLoader.exists(dir.path_join(SCENARIO_FILE)):
        scenario = load(dir.path_join(SCENARIO_FILE)) as MaszynaCompiledScenery
    if scenario:
        compiled.trainsets.append_array(scenario.trainsets)
        compiled.events.append_array(scenario.events)
        compiled.memcells.append_array(scenario.memcells)
        compiled.launchers.append_array(scenario.launchers)
        compiled.sounds.append_array(scenario.sounds)
        compiled.isolated_sections.append_array(scenario.isolated_sections)
        compiled.scripts.append_array(scenario.scripts)
        compiled.nodes = scenario.nodes
    if scenery:
        compiled.tracks.append_array(scenery.tracks)
        compiled.traction.append_array(scenery.traction)
        compiled.power_sources.append_array(scenery.power_sources)
        compiled.models.append_array(scenery.models)
    await SceneryInstancer.instantiate_compiled(self, compiled, measurement)
    if is_load_given_up() or not sector_ids:
        return
    # the far terrain of the sectors' blocks, beside the scene too; children of the content, freed
    # with it
    var blocks:Dictionary[String, bool] = {}
    for id:String in sector_ids:
        blocks[block_of(id)] = true
    for block:String in blocks:
        var far_path:String = dir.path_join(far_terrain_file(block))
        if not dir or not ResourceLoader.exists(far_path):
            continue
        var far:MeshInstance3D = MeshInstance3D.new()
        far.name = "FarTerrain_" + block
        far.mesh = load(far_path) as Mesh
        far.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
        add_child(far)
    if not sector_source:
        sector_source = MaszynaWorldPackSectorSource.new(world_name)
    _provider = MaszynaWorldSectorProvider.new(origin, sector_ids, sector_source)
    _provider_rid = SceneryStreamingServer.provider_register(_provider, get_world_3d().scenario)
    _provider_rids.append(_provider_rid)
    sectors.sector_loaded.connect(_on_sector_loaded)
    sectors.sector_unloaded.connect(_on_sector_unloaded)
    sectors.start(origin, sector_ids, sector_source)


## The sectors are let go - what draws them takes them out - before the content goes
func _clear_content(budget_msec:int = 0) -> void:
    if _provider:
        sectors.stop()
        sectors.sector_loaded.disconnect(_on_sector_loaded)
        sectors.sector_unloaded.disconnect(_on_sector_unloaded)
        _provider = null
        _provider_rid = RID()
    await super(budget_msec)


## The sectors' roads are drawn in the world the node is in, as its content is
## (MaszynaSceneryContentNode), and go with the world - freed - before its content does
## (NOTIFICATION_PREDELETE reaches the derived script first)
func _notification(what:int) -> void:
    match what:
        NOTIFICATION_ENTER_WORLD, NOTIFICATION_EXIT_WORLD:
            var scenario:RID = get_world_3d().scenario if what == NOTIFICATION_ENTER_WORLD else RID()
            for roads:Array in _sector_roads.values():
                for rid:RID in roads[1]:
                    TrackRenderingServer.set_track_scenario(rid, scenario)
        NOTIFICATION_PREDELETE:
            if _provider:
                sectors.stop()


## A sector loaded after its cells were supplied without it has them supplied again. Its roads
## (SectorFile.ROADS; a world written before has them in its scenery) are registered and connected
## among themselves
func _on_sector_loaded(id:String) -> void:
    _provider.sector_set_loaded(id, true)
    for cell:Vector2i in _provider.sector_get_cells(id):
        SceneryStreamingServer.provider_refresh_cell(_provider_rid, cell)
    var path:String = sector_source.sector_get_path(id, SectorFile.ROADS)
    if not ResourceLoader.exists(path):
        return
    var track_rids:Array[RID] = []
    var track_render_rids:Array[RID] = []
    for track_data:MaszynaTrackData in (ResourceLoader.load(path) as MaszynaCompiledScenery).tracks:
        var built:Dictionary = SceneryInstancer.build_track(track_data, get_world_3d())
        track_rids.append(built["track_rid"])
        track_render_rids.append(built["track_render_rid"])
    TrackServer.track_connect_many(track_rids)
    _sector_roads[id] = [track_rids, track_render_rids]
    sector_roads_built.emit(id, track_rids)


## A sector unloaded has its roads freed
func _on_sector_unloaded(id:String) -> void:
    _provider.sector_set_loaded(id, false)
    if not _sector_roads.has(id):
        return
    sector_roads_freeing.emit(id, _sector_roads[id][0])
    TrackRenderingServer.free_track_many(_sector_roads[id][1])
    TrackServer.track_free_many(_sector_roads[id][0])
    _sector_roads.erase(id)


func _read_environment_declarations() -> void:
    start_time = declared_start_time

