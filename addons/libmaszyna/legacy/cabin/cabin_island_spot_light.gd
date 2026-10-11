class_name CabinIslandSpotLight
extends SpotLight3D

## A copy of an instrument lamp's light at one piece of its "_on" mesh
## (MmdSemanticCatalog.IslandLights.WIDGET_LIGHT). While the setting is on the copies light the
## lamp and the lamp's own light is off; off, the lamp's own light shines instead. It follows the
## setting while it stands (ProjectSettings.settings_changed).

## Project setting choosing the copies over the lamp's own light
var enabled_setting: StringName = &""
## The lamp whose light this copies
var widget: CabinSpotLight3D = null

## The lamp is lit
var _lit: bool = false
## The setting chooses the copies
var _enabled: bool = true


func _enter_tree() -> void:
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    _on_project_settings_changed()


func _exit_tree() -> void:
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)


## The lamp went on or off - its lit_changed is wired straight to this
func set_lit(p_lit: bool) -> void:
    _lit = p_lit
    visible = _lit and _enabled


func _on_project_settings_changed() -> void:
    _enabled = bool(ProjectSettings.get_setting(enabled_setting, true))
    widget.light_enabled = not _enabled
    visible = _lit and _enabled
