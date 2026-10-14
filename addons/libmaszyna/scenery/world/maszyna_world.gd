@tool
extends MaszynaSceneryContentNode
class_name MaszynaWorld

## A world of the game's own format - a .pck the game mounts, described by a .json beside it: the
## squares of the grid it covers (MaszynaWorldSector, a scene each) and its scenario's data. Its load
## composes the sectors' data, each moved from its corner to its place in the world, into one
## compiled scenery and registers it the way a .scn's is (SceneryInstancer.instantiate_compiled()).
## Never through SceneryInstancer's cache: the sectors are already what a cache would hold.

## Side of a square of the grid [m]
const SECTOR_SIZE:float = 1024.0
## Where the worlds are, under the game's directory: <name>.json describing a world, its .pck beside it
const WORLDS_DIR:String = "worlds"
const DESCRIPTION_EXTENSION:String = "json"
## The world's scenario (its trainsets...) beside its scene in the pack: a MaszynaCompiledScenery
const SCENARIO_FILE:String = "scenario.res"

## The world's name: its .json and .pck, and its signalling system's (get_scenery_name())
@export var world_name:String = ""
## The projected metres (east, north) of the world's origin - multiples of SECTOR_SIZE
@export var origin:Vector2 = Vector2.ZERO
@export var sectors:Array[PackedScene] = []
## The world's own data, of no sector: its trainsets, events, memory cells, sounds, scripts
@export var scenario:MaszynaCompiledScenery
## The scenario clock the world declares for its start, hours
@export var declared_start_time:float = START_TIME_DEFAULT


func get_scenery_name() -> String:
    return world_name


## The description of the world `filename` (worlds/<filename> of the game, written by the importer):
## {format, name, title, description, pck, scene, frame, sectors, terrain}; {} when there is none
static func read_description(filename:String) -> Dictionary:
    var path:String = UserSettings.get_maszyna_game_dir().path_join(WORLDS_DIR).path_join(filename)
    var description:Variant = JSON.parse_string(FileAccess.get_file_as_string(path)) if FileAccess.file_exists(path) else null
    return description if description is Dictionary else {}


## Mounts the .pck of the world `description` describes (read_description()), its files under the
## paths they have in it; false when it cannot be - the editor mounts none (the importer's working copy
## of a world is under the same paths)
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


## Where the corner of the square `cell` (MaszynaWorldSector.get_cell()) is in the world
func get_sector_corner(cell:Vector2i) -> Vector3:
    return Vector3(origin.x - cell.x * SECTOR_SIZE, 0.0, cell.y * SECTOR_SIZE - origin.y)


func _has_source() -> bool:
    return not sectors.is_empty()


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
    # meshes are placed as they are, the sector's corner in their transform
    var meshes:Array[MaszynaMeshesNode] = []
    var mesh_corners:Array[Vector3] = []
    for sector_scene:PackedScene in sectors:
        var sector:MaszynaWorldSector = sector_scene.instantiate() as MaszynaWorldSector
        var corner:Vector3 = get_sector_corner(sector.get_cell())
        for node:Node in sector.get_children():
            if node is MaszynaTracksNode:
                for track:MaszynaTrackData in (node as MaszynaTracksNode).tracks:
                    compiled.tracks.append(track.translated(corner))
            elif node is MaszynaTractionNode:
                for span:MaszynaTractionData in (node as MaszynaTractionNode).traction:
                    compiled.traction.append(span.translated(corner))
                for source:MaszynaPowerSourceData in (node as MaszynaTractionNode).power_sources:
                    compiled.power_sources.append(source.translated(corner))
            elif node is MaszynaModelsNode:
                for model:MaszynaModelData in (node as MaszynaModelsNode).models:
                    compiled.models.append(model.translated(corner))
            elif node is MaszynaMeshesNode:
                meshes.append(node as MaszynaMeshesNode)
                mesh_corners.append(corner)
                sector.remove_child(node)
        sector.free()
    await SceneryInstancer.instantiate_compiled(self, compiled, measurement)
    if is_load_given_up():
        for node:MaszynaMeshesNode in meshes:
            node.free()
        return
    var scenario_rid:RID = get_world_3d().scenario
    for index:int in meshes.size():
        for mesh:ArrayMesh in meshes[index].meshes:
            for surface:int in mesh.get_surface_count():
                # a surface of its own material (an orthophoto of the sector) keeps it
                if not mesh.surface_get_material(surface):
                    mesh.surface_set_material(surface, MaterialManager.get_material("", mesh.surface_get_name(surface)))
            _mesh_rids.append(SceneryMeshRenderingServer.mesh_create(
                    mesh, Transform3D(Basis.IDENTITY, mesh_corners[index]), meshes[index].range_end, scenario_rid))
        meshes[index].free()


func _read_environment_declarations() -> void:
    start_time = declared_start_time

