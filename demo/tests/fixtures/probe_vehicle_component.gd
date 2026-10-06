extends GenericVehicleComponentNode
## Fixture for test_generic_vehicle_component.gd: the smallest component a modder can write -
## a node under a VehiclePhysicsNode, which is how a scripted component is authored.
## It records what the C++ side called and publishes one key of state and one of config, so the
## test can assert that a scripted component reaches the vehicle's dumps like a native one.

var process_calls: int = 0
var last_delta: float = 0.0
var commands_received: int = 0


func _ready() -> void:
    register_command("probe_command", self._on_probe_command)


func _process_component(delta: float) -> void:
    process_calls += 1
    last_delta = delta


func _get_component_state() -> Dictionary:
    var state: Dictionary = {}
    state["probe_process_calls"] = process_calls
    return state


func _get_component_config() -> Dictionary:
    var config: Dictionary = {}
    config["probe_config_key"] = "probe"
    return config


func _on_probe_command(_p1: Variant, _p2: Variant) -> void:
    commands_received += 1
