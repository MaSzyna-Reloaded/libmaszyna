extends "mover_switches_section.gd"

var _security:RailVehicleSecuritySystem


func _do_update():
    super._do_update()
    _security = _rail_component(RailVehicleComponentType.COMPONENT_SECURITY) as RailVehicleSecuritySystem
    _show_applicable(not _security == null)


func _on_refresh_timer_timeout() -> void:
    if not _security:
        return
    %SecurityLight.enabled = _security.get_blinking()
    %CabSignalLight.enabled = _security.get_cabsignal_blinking()
