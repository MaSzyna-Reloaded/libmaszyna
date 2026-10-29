@tool
extends SceneryContentNode
class_name MaszynaIncludeNode

## The content of a .scn file of the original (an include of a scenery, or the scenery itself:
## MaszynaSceneryNode), parsed - or read from its cache - by SceneryServer.scenery_load().

## Changing filename does not reload the content - call load() explicitly.
@export var filename:String = ""
@export var parameters:Dictionary = {}
@export var context_rotate: Vector3 = Vector3.ZERO
@export var context_origin: Vector3 = Vector3.ZERO
@export var use_cache:bool = true

## What a missing file of each kind does to the scenario: a part of the scenery or a vehicle's
## description or cab may keep it from running as written; a model, a texture or a sound from being
## drawn or heard
const MISSING_FILE_EFFECTS:Dictionary[MaszynaFileIndexer.FileType, MissingFileEffect] = {
    MaszynaFileIndexer.MASZYNA_FILE_INCLUDE: MissingFileEffect.SCENARIO,
    MaszynaFileIndexer.MASZYNA_FILE_FIZ: MissingFileEffect.SCENARIO,
    MaszynaFileIndexer.MASZYNA_FILE_MMD: MissingFileEffect.SCENARIO,
    MaszynaFileIndexer.MASZYNA_FILE_MODEL: MissingFileEffect.APPEARANCE,
    MaszynaFileIndexer.MASZYNA_FILE_TEXTURE: MissingFileEffect.APPEARANCE,
    MaszynaFileIndexer.MASZYNA_FILE_SOUND: MissingFileEffect.APPEARANCE,
}

## What its last load found of the game's data and gave up on (MaszynaFileIndexer, counted from the
## scenery's compile() on); none while nothing is loaded
var _found_files_count:int = 0
var _missing_files:Dictionary = {}


func get_scenery_name() -> String:
    return filename


func _has_source() -> bool:
    return not filename.is_empty()


## The files its last load found of the game's data
func get_found_files_count() -> int:
    return _found_files_count


## The files its last load gave up on: path -> MaszynaFileIndexer.FileType
func get_missing_files_by_type() -> Dictionary:
    return _missing_files


func get_missing_files() -> Dictionary[String, MissingFileEffect]:
    var missing:Dictionary[String, MissingFileEffect] = {}
    for path:String in _missing_files:
        missing[path] = MISSING_FILE_EFFECTS[_missing_files[path]]
    return missing


func _instantiate_content() -> void:
    await SceneryServer.scenery_load(self, MaszynaSceneryProvider, filename, parameters)
    _found_files_count = MaszynaFileIndexer.found_files_get_count()
    _missing_files = MaszynaFileIndexer.missing_files_get()


func _clear_content(budget_msec:int = 0) -> void:
    _found_files_count = 0
    _missing_files = {}
    await super(budget_msec)
