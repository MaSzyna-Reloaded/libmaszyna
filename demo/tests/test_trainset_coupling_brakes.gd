extends MaszynaGutTest

## A trainset is the vehicles' Movers joined by their couplers (TMoverParameters::Attach). The
## brake pipe of every vehicle is fed through the brake hose from the one the driver operates, so
## what the handle does has to reach the last vehicle. Only the pipe is asserted: the fixture's
## W_Lu_L valve has no distributor of its own in the Mover (Mover.cpp:8715 builds a plain TBrake),
## so its cylinder says nothing about the trainset.

const FIXTURE_PATH:String = "res://tests/fixtures/test_vehicle.fiz"
const VEHICLE_COUNT:int = 3
## Original engine: coupling::coupler | coupling::brakehose (MOVER.h:161)
const COUPLING_WITH_BRAKE_HOSE:int = (RailVehicleController.COUPLING_FLAG_COUPLER
        | RailVehicleController.COUPLING_FLAG_BRAKEHOSE)
## Front and rear coupler of a vehicle standing the normal way (TDynamicObject::AttachNext,
## DynObj.cpp:2590)
## A non-zero scenery velocity makes a vehicle ready to depart - reservoirs full and the brake
## pipe charged (TMoverParameters::CheckLocomotiveParameters, Mover.cpp:8902); 0.1 is what
## scenery authors write for a standing, ready vehicle
const READY_TO_DEPART_VELOCITY:float = 0.1
## Simulated seconds the pipe of the last vehicle takes to follow the handle - braked, then refilled
const BRAKING_SECONDS:float = 10.0
const RELEASING_SECONDS:float = 30.0
## the brake pipe of a released train (CntrlPipePress)
const CHARGED_PIPE_PRESSURE:float = 5.0
const PRESSURE_TOLERANCE:float = 0.1
## a service braking lowers the pipe by at least this much (FV4a full service: about 1.5 bar)
const SERVICE_BRAKING_PIPE_DROP:float = 1.0
## Steps the releaser is held over - it has to stay on through them
const RELEASER_HELD_TICKS:int = 3
## Steps the vehicles just built take to stand ready: they take their configuration on their first
const SETTLE_TICKS:int = 2

var nodes:Array[VehiclePhysicsNode] = []
var controllers:Array[VehicleController] = []
var brakes:Array[RailVehicleBrake] = []


func before_each() -> void:
    nodes.clear()
    controllers.clear()
    brakes.clear()
    var model:VehicleController = FizVehicleBuilder.build_description_at(FIXTURE_PATH)
    for index:int in range(VEHICLE_COUNT):
        # the first vehicle is driven - an unmanned one is not simulated (FINDINGS, 09-23)
        var node:VehiclePhysicsNode = build_vehicle_node("TrainsetVehicle%d" % index, model,
                READY_TO_DEPART_VELOCITY,
                MaszynaDynamicData.DriverType.DRIVER_HEAD if index == 0 else MaszynaDynamicData.DriverType.DRIVER_NOBODY)
        nodes.append(node)
        var controller:VehicleController = VehicleServer.vehicle_get_controller(node.get_vehicle_rid())
        controllers.append(controller)
        brakes.append(controller.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake)
    await step(SETTLE_TICKS)
    for index:int in range(1, VEHICLE_COUNT):
        controllers[index - 1].couple(controllers[index], RailVehicleController.COUPLER_END_REAR,
                RailVehicleController.COUPLER_END_FRONT, COUPLING_WITH_BRAKE_HOSE)


func after_each() -> void:
    nodes.clear()
    controllers.clear()
    brakes.clear()


func test_every_vehicle_is_joined_by_the_brake_hose() -> void:
    for index:int in range(VEHICLE_COUNT - 1):
        assert_true(controllers[index].is_coupled(RailVehicleController.COUPLER_END_REAR), "vehicle %d rear coupler" % index)
        assert_true(controllers[index].is_coupled_by(RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLING_FLAG_BRAKEHOSE),
                "vehicle %d rear brake hose" % index)
        assert_eq(controllers[index].get_coupled_controller(RailVehicleController.COUPLER_END_REAR), controllers[index + 1])
    var joined:Array = RailVehicleServer.vehicle_get_coupled(
            controllers[0].get_rid(), RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_BRAKEHOSE)
    assert_eq(joined.size(), VEHICLE_COUNT)


func test_the_pipe_of_the_last_vehicle_follows_the_handle() -> void:
    var last:RailVehicleBrake = brakes[VEHICLE_COUNT - 1]
    assert_almost_eq(last.get_pipe_pressure(), CHARGED_PIPE_PRESSURE, PRESSURE_TOLERANCE, "a ready train starts charged")

    controllers[0].send_command("brake_level_set_position", "full")
    if not await wait_simulated_until(
            func() -> bool: return last.get_pipe_pressure() < CHARGED_PIPE_PRESSURE - SERVICE_BRAKING_PIPE_DROP,
            BRAKING_SECONDS, "the pipe of the last vehicle emptied"):
        return
    assert_lt(last.get_pipe_pressure(), CHARGED_PIPE_PRESSURE - SERVICE_BRAKING_PIPE_DROP, "the pipe of the last vehicle empties")

    controllers[0].send_command("brake_level_set_position", "drive")
    if not await wait_simulated_until(
            func() -> bool: return absf(last.get_pipe_pressure() - CHARGED_PIPE_PRESSURE) <= PRESSURE_TOLERANCE,
            RELEASING_SECONDS, "the pipe of the last vehicle refilled"):
        return
    assert_almost_eq(last.get_pipe_pressure(), CHARGED_PIPE_PRESSURE, PRESSURE_TOLERANCE, "the pipe of the last vehicle refills")


func test_uncouple_parts_the_vehicles() -> void:
    controllers[0].uncouple(RailVehicleController.COUPLER_END_REAR)
    assert_false(controllers[0].is_coupled(RailVehicleController.COUPLER_END_REAR))
    assert_false(controllers[1].is_coupled(RailVehicleController.COUPLER_END_FRONT))


func test_uncoupling_announces_the_trainset_change_once() -> void:
    watch_signals(controllers[0])
    controllers[0].uncouple(RailVehicleController.COUPLER_END_REAR)
    assert_signal_emit_count(controllers[0], "trainset_changed", 1)
    # the coupler and the brake hose part; the coupler comes first
    var first_detached:Array = get_signal_parameters(controllers[0], "coupler_detached", 0)
    assert_eq(first_detached, [RailVehicleController.COUPLING_FLAG_COUPLER])


# simulation.cpp:184, vehicleparams.cpp:289-293 - the releaser is held while its button is held
func test_the_consist_releaser_is_held_while_its_button_is() -> void:
    controllers[0].send_command("consist_releaser", true)
    assert_true(brakes[0].get_releaser_active(), "switched on")
    await step(RELEASER_HELD_TICKS)
    assert_true(brakes[0].get_releaser_active(), "still held")
    controllers[0].send_command("consist_releaser", false)
    assert_false(brakes[0].get_releaser_active(), "let go with the button")


func test_a_freed_vehicle_leaves_its_neighbours_uncoupled() -> void:
    var first:RID = controllers[0].get_rid()
    watch_signals(controllers[0])
    nodes[1].free()

    assert_false(controllers[0].is_coupled(RailVehicleController.COUPLER_END_REAR), "the front neighbour lets go of the freed vehicle")
    assert_false(controllers[2].is_coupled(RailVehicleController.COUPLER_END_FRONT), "the rear neighbour lets go of the freed vehicle")
    assert_eq(RailVehicleServer.vehicle_get_coupled(
            first, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER).size(), 1)
    assert_signal_emitted(controllers[0], "trainset_changed")
