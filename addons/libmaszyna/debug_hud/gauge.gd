@tool
extends Control

## The reading, in the unit the gauge shows; the needle sweeps max_angle at max_value
@export var value:float = 0.0:
    set(x):
        value = x
        _target = clampf(x / max_value * max_angle, 0.0, max_angle) if max_value > 0.0 else 0.0
        _value_dirty = true
@export var max_value:float = 1.0
@export var unit:String = ""
@export var decimals:int = 0
@export var max_angle = 270.0;
@export var start_angle = 270.0;
@export var label = "":
    set(x):
        label = x
        $Label.text = x

var _current = 0.0;
var _target = 0.0;
var _value_dirty:bool = true


func _process(_delta):
    _current = lerpf(_current, _target, 2.0 * _delta)
    $Arrow.rotation_degrees = _current + start_angle
    if _value_dirty:
        _value_dirty = false
        $Value.text = "%.*f %s" % [decimals, value, unit]
