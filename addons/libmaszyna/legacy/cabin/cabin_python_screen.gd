extends Node
class_name CabinPythonScreen

## A cab screen drawn by a Python 2 script (`pyscreen:` of an MMD). The texture PythonScreenServer
## draws replaces the one of the target submodel, and the screen is redrawn from the vehicle's
## state at its own interval (TTrain::update_screens(), Train.cpp:10288).

## The albedo texture of the cab's materials (MaszynaMaterialFactory)
const TEXTURE_PARAMETER:StringName = &"texture_albedo"
const MSEC_PER_SEC:float = 1000.0
## Whether the submodel's material shines (MaszynaMaterialFactory, from the E3D self-illumination)
const SELF_ILLUMINATION_PARAMETER:StringName = &"emission_enabled"
## The glow the screen throws onto the desk in the colour of what it shows: a weak light reaching
## far, so that it lights the desk around the screen without dazzling
const SCREEN_GLOW_ENABLED_SETTING:StringName = &"maszyna/cabin/screen_glow_enabled"
const SCREEN_GLOW_ENERGY_SETTING:StringName = &"maszyna/cabin/screen_glow_energy"
const SCREEN_GLOW_ENERGY_DEFAULT:float = 0.05
const SCREEN_GLOW_RANGE_SETTING:StringName = &"maszyna/cabin/screen_glow_range"
const SCREEN_GLOW_RANGE_DEFAULT:float = 1.0
## How far in front of the screen the glow stands, so it is not inside the panel
const SCREEN_GLOW_OFFSET:float = 0.05

var vehicle_rid:RID
## Absolute path of the script, without ".py"
var script_path:String = ""
## The screen's `parameters:` from the MMD
var parameters:Dictionary = {}
## Redraw interval in milliseconds; -1 draws the screen once
var update_time_msec:int = 0
## Submodel the screen is shown on; null for target "none", whose script still runs
var mesh:MeshInstance3D = null

var _screen:RID = RID()
## Lit in the average colour of each frame the script draws, while SCREEN_GLOW_ENABLED_SETTING is
## on - the glow follows it itself; null without a shining mesh
var _glow:CabinGlow = null
## Script commands with no wrapper equivalent yet, reported once each
var _unsupported_commands:Dictionary[String, bool] = {}


func _enter_tree() -> void:
    _screen = PythonScreenServer.screen_create(script_path, _on_commands_received)
    if mesh:
        # the cab's materials are shared through MaterialManager's cache - the screen gets its own
        var material:ShaderMaterial = mesh.material_override.duplicate()
        material.set_shader_parameter(TEXTURE_PARAMETER, PythonScreenServer.screen_get_texture(_screen))
        mesh.material_override = material
    # only a screen that shines - a self-illuminated submodel; E186's log book "okladka" is paper
    # the script only draws on
    if (mesh and mesh.mesh
            and (mesh.material_override as ShaderMaterial).get_shader_parameter(SELF_ILLUMINATION_PARAMETER)):
        _glow = CabinGlow.new()
        _glow.name = "Glow"
        _glow.enabled_setting = SCREEN_GLOW_ENABLED_SETTING
        _glow.energy_setting = SCREEN_GLOW_ENERGY_SETTING
        _glow.energy_default = SCREEN_GLOW_ENERGY_DEFAULT
        _glow.range_setting = SCREEN_GLOW_RANGE_SETTING
        _glow.range_default = SCREEN_GLOW_RANGE_DEFAULT
        _glow.light_color = Color.BLACK
        _glow.shadow_enabled = false
        _glow.set_lit(true)
        add_child(_glow)
        # in front of the screen: its centre, moved along the way its face looks
        var normal_sum:Vector3 = Vector3.ZERO
        for surface:int in mesh.mesh.get_surface_count():
            var normals:PackedVector3Array = mesh.mesh.surface_get_arrays(surface)[Mesh.ARRAY_NORMAL]
            for normal:Vector3 in normals:
                normal_sum += normal
        var facing:Vector3 = (mesh.global_basis * normal_sum).normalized()
        _glow.global_position = mesh.to_global(mesh.get_aabb().get_center()) + facing * SCREEN_GLOW_OFFSET
        PythonScreenServer.screen_rendered.connect(_on_screen_rendered)


func _exit_tree() -> void:
    if _glow:
        PythonScreenServer.screen_rendered.disconnect(_on_screen_rendered)
    PythonScreenServer.screen_free(_screen)


func _on_screen_rendered(screen:RID) -> void:
    if screen == _screen:
        _glow.light_color = PythonScreenServer.screen_get_average_color(_screen)


func _ready() -> void:
    if update_time_msec < 0:
        _render()
        return
    var timer := Timer.new()
    timer.wait_time = update_time_msec / MSEC_PER_SEC
    timer.autostart = true
    timer.timeout.connect(_render)
    add_child(timer)


func _render() -> void:
    PythonScreenServer.screen_request_render(_screen, PythonScreenState.compose(vehicle_rid, parameters))


## "command;param1;param2" (PyInt.cpp:151) - the original's command names are not mapped to the
## wrapper's commands yet (TODO.md)
func _on_commands_received(commands:PackedStringArray) -> void:
    for command:String in commands:
        var command_name:String = command.get_slice(";", 0)
        if not command_name in _unsupported_commands:
            _unsupported_commands[command_name] = true
            push_warning("CabinPythonScreen: %s sends '%s', which is not supported" % [script_path, command_name])
