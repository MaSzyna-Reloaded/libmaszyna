extends RefCounted
class_name MaszynaWorldSectorSource

## Where the files of a world's sectors (MaszynaWorld) come from - the one place: asked for a
## sector the camera comes near (MaszynaWorldSectors), it makes the sector's files readable - at
## once from a pack on the disk (MaszynaWorldPackSectorSource), or once they are downloaded - and
## says so (sector_available); told a sector is no longer wanted, it may let its files go.
## Implemented by overriding the sector_* methods.

## The files of the sector `id` may be read, at the paths sector_get_path() gives
signal sector_available(id:String)


## The sector `id` is wanted: sector_available once its files can be read; never when it has none
func sector_request(_id:String) -> void:
    pass


## The sector `id` is no longer wanted
func sector_release(_id:String) -> void:
    pass


## Where the file `file` of the sector `id` is read once it is available; called from any thread
func sector_get_path(_id:String, _file:MaszynaWorld.SectorFile) -> String:
    return ""
