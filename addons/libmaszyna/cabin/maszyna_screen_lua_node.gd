extends LuaCabinTextureNode
class_name MaszynaScreenLuaNode

## A cab screen drawn by a Lua script: a `pyscreen:` of an MMD whose script has a `.lua` file beside
## it (MmdCabinInstancer), drawn on the screen's submodel instead of a Python script's picture. After
## its drawing, the cab's vehicle and cabin, the script gets the screen's `parameters:` and its
## update interval: `local drawing, vehicle, cabin, parameters, update_time = ...`, update_time in
## seconds, negative for a screen drawn once (Train.cpp:10729-10742). The script redraws itself
## (reloaded.sim.every(update_time, ...)).

const MSEC_PER_SEC:float = 1000.0

## Submodel the screen is shown on; null for target "none", whose script still runs
var mesh:MeshInstance3D = null
## The screen's `parameters:` from the MMD
var parameters:Dictionary = {}
## Redraw interval in milliseconds; -1 draws the screen once
var update_time_msec:int = 0


func _enter_tree() -> void:
    if mesh:
        MaszynaMaterialManager.material_replace_texture(mesh.material_override, texture)


func _exit_tree() -> void:
    if mesh:
        MaszynaMaterialManager.material_restore_texture(mesh.material_override)


func _get_script_arguments() -> Array:
    return [parameters, update_time_msec / MSEC_PER_SEC]
