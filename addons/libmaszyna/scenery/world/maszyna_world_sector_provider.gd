extends SceneryStreamingProvider
class_name MaszynaWorldSectorProvider

## What is drawn of a world's sectors (MaszynaWorld) - their geometry (buildings, platforms,
## bridges with their orthophotos and facades), their decorations' models and their power lines'
## wires - read as the camera comes near and let go once it has left. SceneryStreamingServer asks
## for a cell of its grid, and the provider reads the loaded sectors (sector_set_loaded(),
## MaszynaWorldSectors) lying across it from their drawn file (MaszynaWorld.SectorFile.DRAWN, a
## MaszynaWorldSector of MaszynaMeshesNode, MaszynaModelsNode and MaszynaLinesNode, at the path
## the world's MaszynaWorldSectorSource gives), handing over what of them stands in the cell. What
## the simulation needs whatever the camera does - tracks, traction, signals - is not here: the
## world loads it whole (its scenery).

## SceneryStreamingServer::CHUNK_SIZE_M (not bound)
const CELL_SIZE:float = 1000.0
## How far a cell's content reaches out of it [m]: a mesh stands in the cell of its centre and is a
## square of a sector's geometry (the importer's SIWorldWriter.MESH_CHUNK), triangles reaching out
## of the square by their centre
const OVERHANG:float = 256.0
## The sectors read last, kept for the cells around: a sector lies across up to four cells, each
## asked for by kind, the nearest first. A 3x3 of sectors
const KEPT_SECTORS:int = 9

var _source:MaszynaWorldSectorSource = null
## Cell -> the ids of the sectors lying across it
var _cell_sectors:Dictionary[Vector2i, PackedStringArray] = {}
## Sector id -> its corner in the world
var _corners:Dictionary[String, Vector3] = {}
## The ids of the loaded sectors; written on the main thread, read on the streaming worker
var _loaded:Dictionary[String, bool] = {}
var _loaded_mutex:Mutex = Mutex.new()
## Sector id -> what is drawn of it, in the world: {meshes: [[mesh, range_end]], models:
## [MaszynaModelData], lines: [MaszynaLinesData]}; the oldest first
var _kept:Dictionary[String, Dictionary] = {}
var _kept_mutex:Mutex = Mutex.new()


## The sectors `sector_ids` of the world at `origin` (MaszynaWorld.origin), their files from `source`
func _init(origin:Vector2 = Vector2.ZERO, sector_ids:PackedStringArray = PackedStringArray(),
        source:MaszynaWorldSectorSource = null) -> void:
    _source = source
    for id:String in sector_ids:
        _corners[id] = MaszynaWorld.sector_corner(origin, MaszynaWorldSector.cell_of(id))
        for cell:Vector2i in sector_get_cells(id):
            if not _cell_sectors.has(cell):
                _cell_sectors[cell] = PackedStringArray()
            _cell_sectors[cell].append(id)


## The sector `id` is loaded or not (MaszynaWorldSectors): only a loaded one is read. A sector
## loaded after its cells were supplied has them asked for again (MaszynaWorld)
func sector_set_loaded(id:String, loaded:bool) -> void:
    _loaded_mutex.lock()
    if loaded:
        _loaded[id] = true
    else:
        _loaded.erase(id)
    _loaded_mutex.unlock()


## The cells of SceneryStreamingServer's grid the sector `id` lies across
func sector_get_cells(id:String) -> Array[Vector2i]:
    var corner:Vector3 = _corners[id]
    var cells:Array[Vector2i] = []
    # X runs west from the corner: the sector is the square west (-X) and north (+Z) of it
    for x:int in range(floori((corner.x - MaszynaWorld.SECTOR_SIZE) / CELL_SIZE), ceili(corner.x / CELL_SIZE)):
        for z:int in range(floori(corner.z / CELL_SIZE), ceili((corner.z + MaszynaWorld.SECTOR_SIZE) / CELL_SIZE)):
            cells.append(Vector2i(x, z))
    return cells


func _get_chunk_cells() -> Array[Vector2i]:
    var cells:Array[Vector2i] = []
    cells.assign(_cell_sectors.keys())
    return cells


func _chunk_get_overhang(_cell:Vector2i) -> float:
    return OVERHANG


func _get_content_kinds() -> PackedInt32Array:
    return PackedInt32Array([CONTENT_MESHES, CONTENT_MODELS, CONTENT_LINES])


func _chunk_load(cell:Vector2i, kind:int) -> Array:
    var items:Array = []
    # the lines' section origin -> their geometry: the lines of the sectors in the cell merge
    var sections:Dictionary[Vector3, MaszynaLinesGeometry] = {}
    for id:String in _cell_sectors.get(cell, PackedStringArray()):
        _loaded_mutex.lock()
        var loaded:bool = _loaded.has(id)
        _loaded_mutex.unlock()
        if not loaded:
            continue
        # what is drawn of the sector: kept from the last time, or read from its drawn file
        _kept_mutex.lock()
        var drawn:Dictionary = _kept.get(id, {})
        _kept_mutex.unlock()
        if not drawn:
            var meshes:Array = []
            var models:Array[MaszynaModelData] = []
            var lines:Array[MaszynaLinesData] = []
            var path:String = _source.sector_get_path(id, MaszynaWorld.SectorFile.DRAWN)
            var scene:PackedScene = ResourceLoader.load(path) as PackedScene if ResourceLoader.exists(path) else null
            var sector:Node = scene.instantiate() if scene else null
            if sector:
                for node:Node in sector.get_children():
                    if node is MaszynaMeshesNode:
                        for mesh:ArrayMesh in (node as MaszynaMeshesNode).meshes:
                            meshes.append([mesh, (node as MaszynaMeshesNode).range_end])
                    elif node is MaszynaModelsNode:
                        for model:MaszynaModelData in (node as MaszynaModelsNode).models:
                            models.append(model.translated(_corners[id]))
                    elif node is MaszynaLinesNode:
                        for wires:MaszynaLinesData in (node as MaszynaLinesNode).lines:
                            lines.append(wires.translated(_corners[id]))
                sector.free()
            drawn = {"meshes": meshes, "models": models, "lines": lines}
            _kept_mutex.lock()
            _kept[id] = drawn
            if _kept.size() > KEPT_SECTORS:
                _kept.erase(_kept.keys()[0])
            _kept_mutex.unlock()
        var transform:Transform3D = Transform3D(Basis.IDENTITY, _corners[id])
        match kind:
            CONTENT_MESHES:
                for piece:Array in drawn["meshes"]:
                    var mesh:ArrayMesh = piece[0]
                    if _cell_of(transform * mesh.get_aabb().get_center()) == cell:
                        items.append(MaszynaMeshPlacement.new(mesh, transform, piece[1]))
            CONTENT_MODELS:
                for model:MaszynaModelData in drawn["models"]:
                    if _cell_of(model.position) == cell:
                        var placement:SceneryModelPlacement = SceneryModelPlacement.new()
                        placement.data_path = model.data_path
                        placement.model_filename = model.model_filename
                        placement.skins = model.skins
                        placement.transform = Transform3D(Basis.from_euler(model.rotation), model.position)
                        placement.range_min = model.range_min
                        placement.range_max = model.range_max
                        items.append(placement)
            CONTENT_LINES:
                # a node's lines go to the section of their centre, as SceneryLines.create_meshes() puts them
                for wires:MaszynaLinesData in drawn["lines"]:
                    var centre:Vector3 = Vector3.ZERO
                    for point:Vector3 in wires.points:
                        centre += point
                    var origin:Vector3 = MaszynaLinesGeometry.position_get_section_origin(centre / wires.points.size())
                    if not _cell_of(origin) == cell:
                        continue
                    if not sections.has(origin):
                        sections[origin] = MaszynaLinesGeometry.new()
                        sections[origin].origin = origin
                    sections[origin].add_lines(wires.points, wires.color, wires.width, wires.range_min, wires.range_max)
    items.append_array(sections.values())
    return items


## The cell of SceneryStreamingServer's grid a point of the world is in
static func _cell_of(point:Vector3) -> Vector2i:
    return Vector2i(floori(point.x / CELL_SIZE), floori(point.z / CELL_SIZE))
