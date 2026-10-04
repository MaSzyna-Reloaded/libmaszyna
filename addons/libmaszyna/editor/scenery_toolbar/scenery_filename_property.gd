@tool
extends EditorProperty

## The `filename` of a scenery (MaszynaIncludeNode) in the inspector: the name as text, and
## "Browse" to pick a scenery file of the game directory - the name is kept relative to its
## `scenery` directory, as the scenery reads it.

## Where the game keeps its sceneries, under the game directory
const SCENERY_DIRECTORY:String = "scenery"


func _ready() -> void:
    add_focusable(%Filename)


func _update_property() -> void:
    %Filename.text = get_edited_object().get(get_edited_property())


func _on_filename_submitted(text:String) -> void:
    emit_changed(get_edited_property(), text)


func _on_filename_focus_exited() -> void:
    emit_changed(get_edited_property(), %Filename.text)


func _on_browse_pressed() -> void:
    %Dialog.current_dir = UserSettings.get_maszyna_game_dir().path_join(SCENERY_DIRECTORY)
    %Dialog.popup_file_dialog()


func _on_dialog_file_selected(path:String) -> void:
    var scenery_dir:String = UserSettings.get_maszyna_game_dir().path_join(SCENERY_DIRECTORY)
    emit_changed(get_edited_property(), path.trim_prefix(scenery_dir).trim_prefix("/"))
