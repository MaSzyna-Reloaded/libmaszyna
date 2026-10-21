@tool
extends EditorInspectorPlugin

## The inspector of a .scn scenery (MaszynaIncludeNode): its `filename` gets "Browse" for the
## sceneries of the game directory

const FILENAME_PROPERTY:PackedScene = preload("./scenery_filename_property.tscn")
## The property the control above stands in place of
const FILENAME:String = "filename"


func _can_handle(object:Object) -> bool:
    return object is MaszynaIncludeNode


func _parse_property(
        _object:Object, _type:Variant.Type, name:String, _hint_type:PropertyHint, _hint_string:String,
        _usage_flags:int, _wide:bool) -> bool:
    if name == FILENAME:
        add_property_editor(FILENAME, FILENAME_PROPERTY.instantiate())
        return true
    return false
