extends RefCounted
class_name MaszynaWorldSectors

## Which sectors of a world (MaszynaWorld) are wanted - the one owner of it: the ones within the
## draw distance and REQUEST_MARGIN of SceneryStreamingServer's camera are asked of the world's
## source (MaszynaWorldSectorSource), the ones farther than the draw distance and RELEASE_MARGIN
## let go. A sector asked for is loaded once its source has made it available - from a pack at
## once, from a server once downloaded - and what draws the sectors (MaszynaWorldSectorProvider,
## the game's terrain) follows it by sector_loaded and sector_unloaded.

signal sector_loaded(id:String)
signal sector_unloaded(id:String)

## Asked for this far past the draw distance [m] - past SceneryStreamingServer's prefetch
## (PREFETCH_M) and a sector's reach out of its cells - so a sector is there before it is drawn
const REQUEST_MARGIN:float = 1000.0
## Let go this far past the draw distance [m]: SceneryStreamingServer::HYSTERESIS_M past
## REQUEST_MARGIN, so a camera moving along the edge does not ask for them again and again
const RELEASE_MARGIN:float = 2000.0

var _source:MaszynaWorldSectorSource = null
var _origin:Vector2 = Vector2.ZERO
## Cell (MaszynaWorldSector.get_cell()) -> the id of the world's sector there
var _cells:Dictionary[Vector2i, String] = {}
## Sector id -> whether it is loaded (false while its source has not made it available)
var _wanted:Dictionary[String, bool] = {}


## Follows the camera over the sectors `sector_ids` of the world at `origin` (MaszynaWorld.origin),
## asking `source` for them
func start(origin:Vector2, sector_ids:PackedStringArray, source:MaszynaWorldSectorSource) -> void:
    _origin = origin
    _source = source
    for id:String in sector_ids:
        _cells[MaszynaWorldSector.cell_of(id)] = id
    _source.sector_available.connect(_on_sector_available)
    SceneryStreamingServer.streaming_camera_changed.connect(update)
    SceneryStreamingServer.streaming_camera_chunk_changed.connect(update.unbind(1))
    update()


## Stops following the camera: every sector wanted is let go, the loaded ones unloaded
func stop() -> void:
    _source.sector_available.disconnect(_on_sector_available)
    SceneryStreamingServer.streaming_camera_changed.disconnect(update)
    SceneryStreamingServer.streaming_camera_chunk_changed.disconnect(update.unbind(1))
    for id:String in _wanted.keys():
        _source.sector_release(id)
        if _wanted[id]:
            sector_unloaded.emit(id)
    _wanted.clear()
    _cells.clear()
    _source = null


func is_sector_loaded(id:String) -> bool:
    return _wanted.get(id, false)


## How many sectors the world has
func get_sector_count() -> int:
    return _cells.size()


## The ids of the sectors wanted - asked of the source, loaded or not yet
func get_wanted_sectors() -> PackedStringArray:
    return PackedStringArray(_wanted.keys())


## The id of the world's sector at `position`; "" where the world has none
func get_sector_at(position:Vector3) -> String:
    return _cells.get(MaszynaWorld.cell_at(_origin, position), "")


## The sectors around the camera as it is now: the ones gone out of reach let go, the ones come
## within it asked for
func update() -> void:
    if not SceneryStreamingServer.streaming_has_camera():
        return
    var camera:Vector3 = SceneryStreamingServer.streaming_get_camera_position()
    var reach:float = SceneryStreamingServer.streaming_get_draw_distance()
    for id:String in _wanted.keys():
        if _distance(MaszynaWorldSector.cell_of(id), camera) > reach + RELEASE_MARGIN:
            var loaded:bool = _wanted[id]
            _wanted.erase(id)
            _source.sector_release(id)
            if loaded:
                sector_unloaded.emit(id)
    # the cell under the camera and the ones in reach of it
    var centre:Vector2i = MaszynaWorld.cell_at(_origin, camera)
    var radius:int = ceili((reach + REQUEST_MARGIN) / MaszynaWorld.SECTOR_SIZE)
    for x:int in range(centre.x - radius, centre.x + radius + 1):
        for y:int in range(centre.y - radius, centre.y + radius + 1):
            var id:String = _cells.get(Vector2i(x, y), "")
            if id and not _wanted.has(id) and _distance(Vector2i(x, y), camera) <= reach + REQUEST_MARGIN:
                _wanted[id] = false
                _source.sector_request(id)


## A sector let go before its source made it available is not loaded
func _on_sector_available(id:String) -> void:
    if _wanted.has(id) and not _wanted[id]:
        _wanted[id] = true
        sector_loaded.emit(id)


## How far the camera at `camera` is from the sector at `cell` [m]
func _distance(cell:Vector2i, camera:Vector3) -> float:
    var corner:Vector3 = MaszynaWorld.sector_corner(_origin, cell)
    # X runs west from the corner: the sector is the square west (-X) and north (+Z) of it
    var square:Rect2 = Rect2(corner.x - MaszynaWorld.SECTOR_SIZE, corner.z, MaszynaWorld.SECTOR_SIZE, MaszynaWorld.SECTOR_SIZE)
    var point:Vector2 = Vector2(camera.x, camera.z)
    return point.distance_to(point.clamp(square.position, square.end))
