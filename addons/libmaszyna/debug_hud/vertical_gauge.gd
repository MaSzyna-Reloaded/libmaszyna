@tool
extends VBoxContainer

## A bar filling upwards with its reading printed on it - a compact gauge for a value whose
## number matters more than a needle

## The reading, in the unit the gauge shows; the bar is full at max_value
@export var value:float = 0.0:
    set(x):
        value = x
        _dirty = true
        set_process(true)
@export var max_value:float = 1.0:
    set(x):
        max_value = x
        _dirty = true
        set_process(true)
@export var unit:String = ""
@export var decimals:int = 0
@export var label:String = "":
    set(x):
        label = x
        _dirty = true
        set_process(true)
## The bar's and the caption's width; a window with many gauges side by side makes them narrower
@export var bar_width:float = 64.0:
    set(x):
        bar_width = x
        _dirty = true
        set_process(true)

var _dirty:bool = true


func _process(_delta:float) -> void:
    if not _dirty:
        set_process(false)
        return
    _dirty = false
    %Bar.max_value = max_value
    %Bar.value = value
    %Value.text = "%.*f %s" % [decimals, value, unit]
    %Caption.text = label
    %Bar.custom_minimum_size.x = bar_width
    %Caption.custom_minimum_size.x = bar_width
