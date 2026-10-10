@tool
extends Node
class_name MaszynaEditorViewportDrop

## Something dragged out of an editor panel and dropped into the 3D view - a nodebank model, a
## vehicle of the Vehicles tab. While the left mouse button is held, the preview (if there is one)
## stands where the point of the 3D view under the mouse lands (MaszynaEditorViewportPoint);
## released over the view, the drop is called with that point. Processes only while dragging.

## The dragged thing is over the 3D view, above world_position - every frame it is
signal dragged(world_position:Vector3)
## The drag ended, dropped or not
signal drag_stopped

var _preview:Node3D = null
var _drop:Callable = Callable()


func _ready() -> void:
    set_process(false)


## A drag starts: the preview - optional, the drag's own from now on - stands in the edited scene,
## and drop(world_position) is called when the button is released over the 3D view
func start(preview:Node3D, drop:Callable) -> void:
    stop()
    _drop = drop
    var scene_root:Node = EditorInterface.get_edited_scene_root()
    if preview and scene_root:
        _preview = preview
        scene_root.add_child(preview, false, Node.INTERNAL_MODE_BACK)
    set_process(true)


## The drag ends, dropping nothing
func stop() -> void:
    if _preview:
        _preview.queue_free()
    _preview = null
    _drop = Callable()
    set_process(false)
    drag_stopped.emit()


func _process(_delta:float) -> void:
    var viewport:SubViewport = EditorInterface.get_editor_viewport_3d(0)
    var mouse_position:Vector2 = viewport.get_mouse_position()
    var over_view:bool = viewport.get_visible_rect().has_point(mouse_position)
    var world_position:Vector3 = Vector3.ZERO
    if over_view:
        world_position = MaszynaEditorViewportPoint.world_point(viewport.get_camera_3d(), mouse_position)
    if _preview:
        _preview.visible = over_view
        _preview.global_position = world_position
    if over_view:
        dragged.emit(world_position)
    if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
        return
    # dropped while whoever follows the drag still knows what it was over
    if over_view:
        _drop.call(world_position)
    stop()
