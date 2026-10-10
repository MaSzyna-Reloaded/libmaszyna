class_name WindowMoveHandle
extends Control

## A handle that moves `window` with the mouse while held, keeping it on the screen. It draws a
## short bar in its middle; the look of the window around it is the window's own.

## The bar drawn in the middle of the handle
const BAR_SIZE:Vector2 = Vector2(36.0, 4.0)
const BAR_COLOR:Color = Color(0.6, 0.66, 0.75, 0.45)
const BAR_COLOR_HELD:Color = Color(0.45, 0.72, 1.0, 0.9)

## The control the handle moves
@export var window:Control = null

var _held:bool = false
var _grab_offset:Vector2 = Vector2.ZERO


func _gui_input(event:InputEvent) -> void:
    var button:InputEventMouseButton = event as InputEventMouseButton
    if button and button.button_index == MOUSE_BUTTON_LEFT:
        _held = button.pressed
        _grab_offset = button.global_position - window.global_position
        queue_redraw()
        accept_event()
        return
    var motion:InputEventMouseMotion = event as InputEventMouseMotion
    if motion and _held:
        var furthest:Vector2 = (window.get_viewport_rect().size - window.size).max(Vector2.ZERO)
        window.global_position = (motion.global_position - _grab_offset).clamp(Vector2.ZERO, furthest)
        accept_event()


## The bar, its ends rounded
func _draw() -> void:
    var color:Color = BAR_COLOR_HELD if _held else BAR_COLOR
    var radius:float = BAR_SIZE.y / 2.0
    var start:Vector2 = Vector2((size.x - BAR_SIZE.x) / 2.0 + radius, size.y / 2.0)
    var end:Vector2 = Vector2((size.x + BAR_SIZE.x) / 2.0 - radius, size.y / 2.0)
    draw_line(start, end, color, BAR_SIZE.y)
    draw_circle(start, radius, color)
    draw_circle(end, radius, color)
