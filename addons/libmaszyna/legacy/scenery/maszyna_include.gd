@tool
extends MaszynaSceneryContentNode
class_name MaszynaLegacyIncludeNode

## The content of a .scn file of the original (an include of a scenery, or the scenery itself:
## MaszynaLegacySceneryNode), parsed - or read from its cache - by SceneryInstancer.instantiate().

## Changing filename does not reload the content - call load() explicitly.
@export var filename:String = ""
@export var parameters:Dictionary = {}
@export var context_rotate: Vector3 = Vector3.ZERO
@export var context_origin: Vector3 = Vector3.ZERO
@export var use_cache:bool = true


func get_scenery_name() -> String:
    return filename


func _has_source() -> bool:
    return not filename.is_empty()


func _instantiate_content() -> void:
    await SceneryInstancer.instantiate(self, parameters)
