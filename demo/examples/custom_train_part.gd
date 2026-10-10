extends GenericVehicleComponentNode

var _total_time = 0.0

var state = {
    "custom_train_part_calls": 0,
}

func _ready():
    register_command("custom_command", self._handle_custom_command)

func _handle_custom_command(p1, p2):
    log_warning("CUSTOM COMMAND HERE")

func _process_component(delta):
    _total_time += delta
    if _total_time > 2:
        _total_time = 0
        log_debug("Here is a custom train part")
        state["custom_train_part_calls"] += 1

func _get_component_state():
    return state
