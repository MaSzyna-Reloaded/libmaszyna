@tool
extends Control

@export_color_no_alpha var color_active = Color.GREEN:
    set(x):
        if not color_active == x:
            color_active = x
            _update_light()

@export_color_no_alpha var color_inactive = Color.BLACK:
    set(x):
        if not color_inactive == x:
            color_inactive = x
            _update_light()

@export var label:String = "":
    set(x):
        label = x
        $PanelContainer/Label.text = label

@export var enabled:bool = false:
    set(x):
        if not enabled == x:
            enabled = x
            if blink_interval > 0:
                if enabled:
                    timer.start()
                else:
                    timer.stop()
                    light_on = false
            else:
                light_on = x

@export_range(0, 1) var blink_interval:float = 0.0:
    set(x):
        blink_interval = x
        if x > 0:
            timer.wait_time = x

var timer = Timer.new()

var light_on:bool = false:
    set(x):
        light_on = x
        _update_light()

func _ready():
    _update_light()
    timer.autostart = false
    timer.one_shot = false
    timer.timeout.connect(_toggle_light)
    add_child(timer)

func _toggle_light():
    if enabled:
        light_on = not light_on

func _update_light():
    $PanelContainer.get("theme_override_styles/panel").bg_color = color_active if light_on else color_inactive
