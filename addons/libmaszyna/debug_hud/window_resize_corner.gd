class_name WindowResizeCorner
extends Control

## A corner grip that resizes `window` with the mouse while held, never below the window's own
## minimum size. It draws three short diagonal strokes; the look of the window around it is the
## window's own.

const STROKE_COUNT:int = 3
## Between the strokes, from the corner inwards
const STROKE_SPACING:float = 4.0
const STROKE_WIDTH:float = 1.5
const STROKE_COLOR:Color = Color(0.6, 0.66, 0.75, 0.55)
const STROKE_COLOR_HELD:Color = Color(0.45, 0.72, 1.0, 0.9)

## The control the corner resizes
@export var window:Control = null

var _held:bool = false
var _grab_position:Vector2 = Vector2.ZERO
var _grab_size:Vector2 = Vector2.ZERO


func _gui_input(event:InputEvent) -> void:
    var button:InputEventMouseButton = event as InputEventMouseButton
    if button and button.button_index == MOUSE_BUTTON_LEFT:
        _held = button.pressed
        _grab_position = button.global_position
        _grab_size = window.size
        queue_redraw()
        accept_event()
        return
    var motion:InputEventMouseMotion = event as InputEventMouseMotion
    if motion and _held:
        window.size = (_grab_size + motion.global_position - _grab_position).max(window.get_combined_minimum_size())
        accept_event()


## The strokes, across the bottom right corner
func _draw() -> void:
    var color:Color = STROKE_COLOR_HELD if _held else STROKE_COLOR
    for stroke:int in range(1, STROKE_COUNT + 1):
        var reach:float = stroke * STROKE_SPACING
        draw_line(Vector2(size.x - reach, size.y), Vector2(size.x, size.y - reach), color, STROKE_WIDTH, true)
