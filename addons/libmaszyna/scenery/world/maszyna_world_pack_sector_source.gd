extends MaszynaWorldSectorSource
class_name MaszynaWorldPackSectorSource

## The sectors of a world in the packs of its blocks on the disk (worlds/<world>/<block>.pck of
## the game's directory, MaszynaWorld.block_of()): a block's pack is mounted when the first of its
## sectors is asked for - mounting reads only its list of files, and a pack cannot be unmounted -
## and its sectors are available at once. A block without a pack has none, nor has a sector its
## block's pack lacks (one exported as far terrain only: MaszynaWorld.FAR_TERRAIN_DIR). The editor
## mounts nothing: the importer's working copy of the world is under the same paths.

## The pack of the block `block` was mounted (or found missing: get_blocks())
signal block_mounted(block:String)

var _world_name:String = ""
## Block id -> {mounted: bool, path: String, bytes: int, msec: int (Time.get_ticks_msec() of the mount)}
var _blocks:Dictionary[String, Dictionary] = {}


func _init(world_name:String = "") -> void:
    _world_name = world_name


func sector_request(id:String) -> void:
    var block:String = MaszynaWorld.block_of(id)
    if not _blocks.has(block):
        var path:String = UserSettings.get_maszyna_game_dir().path_join(MaszynaWorld.WORLDS_DIR).path_join(
                _world_name).path_join(block + "." + MaszynaWorld.PACK_EXTENSION)
        _blocks[block] = {
            "mounted": Engine.is_editor_hint() or ProjectSettings.load_resource_pack(path),
            "path": path,
            "bytes": FileAccess.get_size(path) if FileAccess.file_exists(path) else 0,
            "msec": Time.get_ticks_msec(),
        }
        block_mounted.emit(block)
    if _blocks[block]["mounted"] and ResourceLoader.exists(sector_get_path(id, MaszynaWorld.SectorFile.DRAWN)):
        sector_available.emit(id)


## The blocks whose packs were asked for: block id -> {mounted, path, bytes, msec - when it was
## mounted, Time.get_ticks_msec()}; a block not mounted had no pack
func get_blocks() -> Dictionary[String, Dictionary]:
    return _blocks.duplicate(true)


func sector_get_path(id:String, file:MaszynaWorld.SectorFile) -> String:
    return MaszynaWorld.sector_path(_world_name, id, file)
