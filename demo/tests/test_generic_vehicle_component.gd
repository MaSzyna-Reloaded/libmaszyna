extends MaszynaGutTest
## GenericVehicleComponent is the gateway a modder writes a vehicle component in GDScript through.
## Nothing covered it before, so the whole contract is asserted here: the tick, both dumps, the
## way back to the vehicle, and command registration.

const ProbeComponent: GDScript = preload("fixtures/probe_vehicle_component.gd")
## Steps the vehicle is stepped over: it takes the component on its first, and ticks it on each
const SETTLE_TICKS:int = 2

var _vehicle: VehiclePhysicsNode = null
var _probe: GenericVehicleComponentNode = null


func before_each() -> void:
    _vehicle = RailVehiclePhysicsNode.new()
    _vehicle.vehicle_id = "GenericComponentTest"
    _probe = ProbeComponent.new()
    _probe.name = "ProbeComponent"
    _vehicle.add_child(_probe)
    add_child(_vehicle)
    await step(SETTLE_TICKS)


func after_each() -> void:
    remove_child(_vehicle)
    _vehicle.free()
    _vehicle = null
    _probe = null


func test_the_script_is_ticked_with_the_step_delta() -> void:
    var before: int = _probe.process_calls
    await step(SETTLE_TICKS)
    assert_gt(_probe.process_calls, before, "_process_component runs while the component is enabled")
    assert_gt(_probe.last_delta, 0.0, "it is handed the step's delta")


func test_the_scripts_keys_reach_the_vehicle_state_dump() -> void:
    var state: Dictionary = VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid()).get_state()
    assert_true(state.has("probe_process_calls"), "_get_component_state feeds the vehicle dump")
    assert_true(state.has("velocity"), "the vehicle's own keys are there too")


func test_the_scripts_keys_reach_the_vehicle_config_dump() -> void:
    _probe.get_component().apply_config()
    var config: Dictionary = VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid()).get_config()
    assert_eq(config.get("probe_config_key"), "probe", "_get_component_config feeds the config dump")


func test_the_component_reaches_its_vehicle() -> void:
    assert_same(_probe.get_component().get_controller(), VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid()), "get_controller returns the owning vehicle")
    assert_true(_probe.get_component().get_vehicle_state().has("velocity"), "get_vehicle_state is the whole vehicle's")


func test_a_command_registered_from_the_script_is_received() -> void:
    VehicleServer.vehicle_send_command(VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid()).get_rid(), "probe_command", null, null)
    assert_eq(_probe.commands_received, 1, "register_command wired the script's handler")


## A disabled component is left out of the tick and out of the dump, the way a native one is.
func test_a_disabled_component_neither_ticks_nor_publishes() -> void:
    _probe.get_component().enabled = false
    await step(SETTLE_TICKS)
    var before: int = _probe.process_calls
    await step(SETTLE_TICKS)
    assert_eq(_probe.process_calls, before, "a disabled component is not ticked")
    assert_false(VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid()).get_state().has("probe_process_calls"), "nor does it publish state")


## A freed component node takes its component out of the vehicle: left in, it was still ticked
## and read, and called back into the freed node (REQUIRED_CLEANING RC-123).
func test_a_freed_component_node_leaves_the_vehicle() -> void:
    var controller: VehicleController = VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid())
    var count: int = controller.get_components().size()
    _probe.free()
    _probe = null
    await step(SETTLE_TICKS)
    assert_eq(controller.get_components().size(), count - 1, "the component left the vehicle")
    assert_false(controller.get_state().has("probe_process_calls"), "nor is it read into the dump")


## Taken out of the tree and put back ("Edit FIZ"), the node leaves one component, not two.
func test_a_component_node_put_back_is_one_component() -> void:
    var controller: VehicleController = VehicleServer.vehicle_get_controller(_vehicle.get_vehicle_rid())
    var count: int = controller.get_components().size()
    _vehicle.remove_child(_probe)
    _vehicle.add_child(_probe)
    assert_eq(controller.get_components().size(), count, "the component was replaced, not doubled")
