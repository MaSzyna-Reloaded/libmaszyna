extends "mover_switches_section.gd"

var _brakes:RailVehicleBrake


func _do_update():
    super._do_update()
    _brakes = _rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    _show_applicable(not _brakes == null)


func _on_refresh_timer_timeout() -> void:
    if not _brakes:
        return
    %BrakeCylinderPressure.value = _brakes.get_air_pressure()
    %BrakePipePressure.value = _brakes.get_pipe_pressure()
    %LocalBrake.value = _brakes.get_local_position_normalized()
