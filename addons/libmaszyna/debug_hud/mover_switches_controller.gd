extends "mover_switches_section.gd"


## Two different things end up in the main readout, which is why it has two sources: a vehicle
## with a universal controller shows that controller's selector, and every other vehicle shows the
## position of its main controller. Dropping the second one emptied the panel on every locomotive
## that has no universal controller, which is most of them.
func _on_refresh_timer_timeout() -> void:
    if not target_vehicle.is_valid():
        return
    var state:Dictionary = VehicleServer.vehicle_dump_state(target_vehicle)
    var direction:int = state.get("direction", VehicleController.DIRECTION_NEUTRAL)
    %Forward.modulate = Color.GREEN if direction == VehicleController.DIRECTION_FORWARD else Color.WHITE
    %Reverse.modulate = Color.GREEN if direction == VehicleController.DIRECTION_BACKWARD else Color.WHITE
    if universal_controller:
        %MainPosition.text = tr("Pos: %s") % universal_controller.get_selector_position()
    else:
        %MainPosition.text = tr("Pos: %s") % state.get("controller_main_position", 0)
    %SecondPosition.text = tr("Pos: %s") % state.get("controller_second_position", 0)
