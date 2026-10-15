@tool
extends MaszynaSceneryContentNode
class_name MaszynaWorld

## A world of the game's own format, described by worlds/<name>.json of the game's directory, of the
## squares of the grid (MaszynaWorldSector, sector_ids) in blocks of BLOCK_SECTORS x BLOCK_SECTORS
## and its scenario's data. Its files are in packs in worlds/<name>/: WORLD_PACK holds what is
## loaded whole - the world's scene, its scenery (the sectors' tracks, traction and signals, moved
## from their corners to their places, add_sector()), its scenario - and a pack a block
## (<block>.pck, block_of()) what is drawn of its sectors, sector by sector. The scenery is
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
## The .json's "format" the game reads. 4: the world in packs of blocks (block_of()), a sector's
## decorations and wires drawn with it (SectorFile.DRAWN)
const FORMAT:int = 4
## Where a world's files are in the packs: RESOURCES_DIR/<name>/, its blocks' in BLOCKS_DIR/<block>/
const RESOURCES_DIR:String = "res://worlds"
const BLOCKS_DIR:String = "blocks"
const SECTORS_DIR:String = "sectors"
## The world's scenery (scenery) beside its scene
const SCENERY_FILE:String = "scenery.res"
## The world's scenario (its trainsets...) beside its scene: a MaszynaCompiledScenery
const SCENARIO_FILE:String = "scenario.res"

## A sector's files (sector_path())
enum SectorFile {
    ## What is drawn of it: its geometry, decorations and wires (MaszynaWorldSectorProvider)
    DRAWN,
    ## Its Terrain3D region
    TERRAIN,
}
const SECTOR_FILE_SUFFIXES:Dictionary[SectorFile, String] = {
    SectorFile.DRAWN: ".drawn.scn",
    SectorFile.TERRAIN: ".terrain.res",
}

## The world's name: its .json and .pck, and its signalling system's (get_scenery_name())
@export var world_name:String = ""
## The projected metres (east, north) of the world's origin - multiples of SECTOR_SIZE
@export var origin:Vector2 = Vector2.ZERO
## The sectors' data the simulation needs, in the world's coordinates (add_sector())
@export var scenery:MaszynaCompiledScenery
## The world's own data, of no sector: its trainsets, events, memory cells, sounds, scripts
@export var scenario:MaszynaCompiledScenery
## The scenario clock the world declares for its start, hours
@export var declared_start_time:float = START_TIME_DEFAULT
## The ids of the world's sectors (MaszynaWorldSector.id)
@export var sector_ids:PackedStringArray = PackedStringArray()

## Where the sectors' files come from; set before the world is loaded, the packs of its blocks
## (MaszynaWorldPackSectorSource) without one
var sector_source:MaszynaWorldSectorSource = null
## The sectors loaded around the camera, which what draws them follows
var sectors:MaszynaWorldSectors = MaszynaWorldSectors.new()

var _provider:MaszynaWorldSectorProvider = null
var _provider_rid:RID = RID()


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


## Mounts the WORLD_PACK of the world `description` describes (read_description()), its files under
## the paths they have in it; false when it cannot be - the editor mounts none (the importer's
## working copy of a world is under the same paths). Its blocks' packs are its sector_source's
static func mount(description:Dictionary) -> bool:
    if Engine.is_editor_hint() or not description.has("pck"):
        return false
    return ProjectSettings.load_resource_pack(
        UserSettings.get_maszyna_game_dir().path_join(WORLDS_DIR).path_join(description["pck"]))


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


## Where the file `file` of the sector `sector_id` of the world `name` is in the packs
static func sector_path(name:String, sector_id:String, file:SectorFile) -> String:
    return RESOURCES_DIR.path_join(name).path_join(BLOCKS_DIR).path_join(block_of(sector_id)).path_join(
            SECTORS_DIR).path_join(sector_id + SECTOR_FILE_SUFFIXES[file])


## The data of `sector` the simulation needs - its tracks, traction and signals (the models named
## or lit: a signal's events reach them by name) - taken out of it, moved from its corner to its
## place in the world and added to the world's scenery. What is left in the sector is what is only
## drawn - its geometry, the other models, the wires (SectorFile.DRAWN): the world draws it around
## the camera
func add_sector(sector:MaszynaWorldSector) -> void:
    if not scenery:
        scenery = MaszynaCompiledScenery.new()
    var corner:Vector3 = sector_corner(origin, sector.get_cell())
    for node:Node in sector.get_children():
        if node is MaszynaTracksNode:
            for track:MaszynaTrackData in (node as MaszynaTracksNode).tracks:
                scenery.tracks.append(track.translated(corner))
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


func _has_source() -> bool:
    return scenery != null or not sector_ids.is_empty()


func _instantiate_content() -> void:
    var measurement:SceneryLoadMeasurement = SceneryLoadMeasurement.new(self)
    var compiled:MaszynaCompiledScenery = MaszynaCompiledScenery.new()
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


## A sector loaded after its cells were supplied without it has them supplied again
func _on_sector_loaded(id:String) -> void:
    _provider.sector_set_loaded(id, true)
    for cell:Vector2i in _provider.sector_get_cells(id):
        SceneryStreamingServer.provider_refresh_cell(_provider_rid, cell)


func _on_sector_unloaded(id:String) -> void:
    _provider.sector_set_loaded(id, false)


func _read_environment_declarations() -> void:
    start_time = declared_start_time

