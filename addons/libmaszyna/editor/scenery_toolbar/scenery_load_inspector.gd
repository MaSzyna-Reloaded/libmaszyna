@tool
extends EditorInspectorPlugin

## The inspector of a scenery: its `filename` gets "Browse" for the sceneries of the game
## directory, and "Load" and "Clear" stand side by side where the "Load" tool button
## (MaszynaSceneryNode.load_action) would stand alone

const SceneryLoadButtons = preload("./scenery_load_buttons.gd")
const LOAD_BUTTONS:PackedScene = preload("./scenery_load_buttons.tscn")
const FILENAME_PROPERTY:PackedScene = preload("./scenery_filename_property.tscn")
## The properties the controls above stand in place of
const FILENAME:String = "filename"
const LOAD_ACTION:String = "load_action"


func _can_handle(object:Object) -> bool:
    return object is MaszynaIncludeNode


func _parse_property(
        object:Object, _type:Variant.Type, name:String, _hint_type:PropertyHint, _hint_string:String,
        _usage_flags:int, _wide:bool) -> bool:
    if name == FILENAME:
        add_property_editor(FILENAME, FILENAME_PROPERTY.instantiate())
        return true
    if name == LOAD_ACTION:
        var buttons:SceneryLoadButtons = LOAD_BUTTONS.instantiate()
        buttons.set_scenery(object as MaszynaSceneryNode)
        add_custom_control(buttons)
        return true
    return false
