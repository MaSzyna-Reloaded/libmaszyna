class_name CabinGlow
extends OmniLight3D

## The glow a cab element throws into the cab - an indicator lamp, an instrument's backlight, a
## screen. Its energy, its range and whether it shines at all are project settings, and it follows
## them while it stands (ProjectSettings.settings_changed): the settings change it live.

## Project setting saying whether the glow shines at all; empty for a glow that always does
var enabled_setting: StringName = &""
var energy_setting: StringName = &""
var energy_default: float = 0.0
var range_setting: StringName = &""
var range_default: float = 0.0

## The element it belongs to is lit
var _lit: bool = false
## The setting lets it shine
var _enabled: bool = true


func _enter_tree() -> void:
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    _on_project_settings_changed()


func _exit_tree() -> void:
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)


## The element went on or off - lit_changed of an indicator is wired straight to this
func set_lit(p_lit: bool) -> void:
    _lit = p_lit
    visible = _lit and _enabled


func _on_project_settings_changed() -> void:
    light_energy = float(ProjectSettings.get_setting(energy_setting, energy_default))
    omni_range = float(ProjectSettings.get_setting(range_setting, range_default))
    _enabled = not enabled_setting or bool(ProjectSettings.get_setting(enabled_setting, true))
    visible = _lit and _enabled
