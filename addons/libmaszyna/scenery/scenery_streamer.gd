@tool
extends Node
class_name SceneryStreamer

## Owns the streaming camera's lifecycle, independently of the player's vehicle and view.
@export var active:bool = false:
    set = set_active

@export var camera:Camera3D:
    set = set_camera

## In the editor, use its temporary view instead of the scene's configured camera.
@export var use_editor_camera:bool = true:
    set = set_use_editor_camera

## Editor view supplied by the editor plugin, without changing the scene's configured camera.
var editor_camera:Camera3D:
    set = set_editor_camera


func set_active(value:bool) -> void:
    active = value
    streaming_update_camera()


func set_camera(value:Camera3D) -> void:
    camera = value
    if active:
        streaming_update_camera()


func set_editor_camera(value:Camera3D) -> void:
    editor_camera = value
    if active:
        streaming_update_camera()


func set_use_editor_camera(value:bool) -> void:
    use_editor_camera = value
    if active:
        streaming_update_camera()


## Also on coming back: the editor takes a scene of a tab it leaves out of the tree
func _enter_tree() -> void:
    streaming_update_camera()


func streaming_update_camera() -> void:
    if not active:
        SceneryStreamingServer.streaming_set_camera(0)
        return
    if is_inside_tree():
        var streaming_camera:Camera3D = editor_camera if use_editor_camera and editor_camera else camera
        SceneryStreamingServer.streaming_set_camera(streaming_camera.get_instance_id() if streaming_camera else 0)


## The streaming stops, and active stays as the scene has it - a scene saved after it left the tree
## kept active off and streamed nothing
func _exit_tree() -> void:
    if active:
        SceneryStreamingServer.streaming_set_camera(0)
