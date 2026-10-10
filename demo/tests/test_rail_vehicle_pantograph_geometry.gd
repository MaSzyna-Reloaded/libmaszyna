extends MaszynaGutTest

## Where each pantograph sits on the vehicle is the vehicle's own geometry, and it comes from the
## model: the original reads it off the pantograph submodel's matrix (TAnimPant::vPos,
## DynObj.cpp:5508-5549 - sideways, up, and along the length). RailVehicleRenderingServer reads the
## same thing off the arm submodels of the model and publishes it to RailVehicleServer, because the
## wire is sampled at those points - one per pantograph.
##
## The test discriminates: until the position was published, both pantographs of every vehicle in
## the game sampled the wire at the same place - the vehicle's origin - because the two exported
## offsets on the node were never written by anything.

const FRONT_ALONG:float = -5.0
const REAR_ALONG:float = 5.0
const LOWER_HEIGHT:float = 4.0
const UPPER_HEIGHT:float = 4.6
const SLIDER_HEIGHT:float = 5.2
## the upper arm leans along the vehicle, so the arm has a length to read at all
const UPPER_LEAN:float = 0.1
const ARM_NODE_COUNT:int = 5
const TOLERANCE:float = 0.001
## The wire over the track the pantograph is raised to, at 1 m over its lower arm's pivot - in reach
## of the arms above, and fed at the voltage a raised pantograph reads [m, V]
const WIRE_HEIGHT:float = 5.0
const WIRE_VOLTAGE:float = 3000.0
## A step of the vehicles, and the most steps the small compressor gets to fill the pantographs'
## tank and the arms to reach the wire [s]
const STEP:float = 0.5
const MAX_RAISE_STEPS:int = 2000
## The pantographs' tank as a real one is kept [bar] - the arms rise from 3.45 bar up
const PANTOGRAPH_TANK_PRESSURE:float = 5.0
## An electric locomotive's power [kW]
const ENGINE_POWER:float = 2000.0
## A loss of the wire shorter and longer than the 0.2 s the original holds through [s]
const SHORT_LOSS:float = 0.1
const LONG_LOSS:float = 0.3
## Read by a pantograph at the wire [V]
const POWERED_VOLTAGE:float = 100.0

var vehicle:RailVehicle3D
var physics_node:VehiclePhysicsNode
var engine:RailVehicleElectricEngine
var _tracks:Array[RID] = []
var _wires:Array[RID] = []
var _sources:Array[RID] = []


func after_each() -> void:
    for wire:RID in _wires:
        TractionServer.wire_free(wire)
    _wires.clear()
    for source:RID in _sources:
        TractionServer.power_source_free(source)
    _sources.clear()
    for track:RID in _tracks:
        TrackServer.track_free(track)
    _tracks.clear()
    TrackServer.topology_rebuild()
    if is_instance_valid(vehicle):
        if vehicle.get_parent():
            vehicle.get_parent().remove_child(vehicle)
        vehicle.queue_free()
    vehicle = null
    engine = null
    physics_node = null


## A model with the five submodels a pantograph is animated through at each of `alongs`, in the order
## they are named: lower arm 1, lower arm 2, upper arm 1, upper arm 2, slider. The second arm of each
## pair is optional in the data, so it stands where the first one does.
func _add_model(alongs:Array[float]) -> void:
    var submodels:Dictionary = {}
    for pantograph:int in alongs.size():
        var along:float = alongs[pantograph]
        var positions:Array[Vector3] = [
            Vector3(0.0, LOWER_HEIGHT, along),
            Vector3(0.0, LOWER_HEIGHT, along),
            Vector3(0.0, UPPER_HEIGHT, along + UPPER_LEAN),
            Vector3(0.0, UPPER_HEIGHT, along + UPPER_LEAN),
            Vector3(0.0, SLIDER_HEIGHT, along),
        ]
        for index:int in range(ARM_NODE_COUNT):
            submodels["arm%d_%d" % [pantograph, index]] = Transform3D(Basis(), positions[index])
    var no_parents:Dictionary = {}
    vehicle.add_child(build_model_instance(submodels, no_parents))
    vehicle.model_instance_path = NodePath("Model")


## The paths of the pantograph's submodels, as a vehicle assembled by hand names them
func _arm_paths(pantograph:int) -> Array[NodePath]:
    var paths:Array[NodePath] = []
    for index:int in range(ARM_NODE_COUNT):
        paths.append(NodePath("Model/arm%d_%d" % [pantograph, index]))
    return paths


func _build_electric_vehicle() -> void:
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.type_name = "test"
    physics_node = build_vehicle_node("test_pantograph_geometry", model)
    engine = MoverRailVehicleElectricSeriesEngine.new()
    var power_source:RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    power_source.current_collector_number_of_collectors = 2
    var controller:VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())
    controller.add_component(engine)
    controller.add_component(power_source)

    vehicle = RailVehicle3D.new()
    var alongs:Array[float] = [FRONT_ALONG, REAR_ALONG]
    _add_model(alongs)
    vehicle.pantograph_front_arm_paths = _arm_paths(0)
    vehicle.pantograph_rear_arm_paths = _arm_paths(1)
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)


func test_each_pantograph_publishes_its_own_position_to_the_vehicle() -> void:
    _build_electric_vehicle()

    assert_almost_eq(
            RailVehicleServer.vehicle_get_pantograph_position(vehicle.get_rid(), RailVehicleEnginePowerSource.PANTOGRAPH_FIRST),
            Vector3(0.0, LOWER_HEIGHT, FRONT_ALONG),
            Vector3(TOLERANCE, TOLERANCE, TOLERANCE),
            "the front pantograph's position is where its lower arm stands on the vehicle")
    assert_almost_eq(
            RailVehicleServer.vehicle_get_pantograph_position(vehicle.get_rid(), RailVehicleEnginePowerSource.PANTOGRAPH_SECOND),
            Vector3(0.0, LOWER_HEIGHT, REAR_ALONG),
            Vector3(TOLERANCE, TOLERANCE, TOLERANCE),
            "the rear pantograph's position is where its lower arm stands on the vehicle")


func test_the_two_pantographs_do_not_share_one_sampling_point() -> void:
    _build_electric_vehicle()

    var front:Vector3 = RailVehicleServer.vehicle_get_pantograph_position(vehicle.get_rid(), RailVehicleEnginePowerSource.PANTOGRAPH_FIRST)
    var rear:Vector3 = RailVehicleServer.vehicle_get_pantograph_position(vehicle.get_rid(), RailVehicleEnginePowerSource.PANTOGRAPH_SECOND)
    assert_almost_eq(
            rear.z - front.z, REAR_ALONG - FRONT_ALONG, TOLERANCE,
            "the two pantographs are as far apart along the vehicle as the model puts them")
    assert_true(
            front.length() > 0.0 and rear.length() > 0.0,
            "neither pantograph samples the wire at the vehicle's origin, got %s and %s" % [front, rear])


func test_a_vehicle_without_pantograph_arms_publishes_no_position() -> void:
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.type_name = "test"
    physics_node = build_vehicle_node("test_pantograph_geometry_bare", model)
    engine = MoverRailVehicleElectricSeriesEngine.new()
    var power_source:RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    var controller:VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())
    controller.add_component(engine)
    controller.add_component(power_source)

    vehicle = RailVehicle3D.new()
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)

    assert_eq(
            RailVehicleServer.vehicle_get_pantograph_position(vehicle.get_rid(), RailVehicleEnginePowerSource.PANTOGRAPH_FIRST), Vector3(),
            "a vehicle whose model carries no pantograph has nothing to publish")


## A vehicle with one pantograph, standing under a powered wire on its track, with the pantograph
## raised until it reaches the wire (or MAX_RAISE_STEPS run out); its controller
func _raise_pantograph_at_wire(vehicle_name:String) -> VehicleController:
    var track:RID = TrackServer.track_create()
    _tracks.append(track)
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = Vector3.ZERO
    curve.p2 = Vector3(0.0, 0.0, 60.0)
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, "start", 1.435)
    TrackServer.topology_rebuild()
    var source:RID = TractionServer.power_source_create()
    _sources.append(source)
    TractionServer.power_source_set_params(source, "test_power", WIRE_VOLTAGE, 0.0, 0.2, 2000.0, 1.0, 3, 60.0, false)
    var wire:RID = TractionServer.wire_create()
    _wires.append(wire)
    TractionServer.wire_set_params(
            wire, Vector3(0.0, WIRE_HEIGHT, -50.0), Vector3(0.0, WIRE_HEIGHT, 100.0), "test_power", WIRE_VOLTAGE,
            2000.0, 0.01)
    TractionServer.network_build()

    var model:RailVehicleController = MoverRailVehicleController.new()
    # a vehicle with no power has no pantographs' tank to fill (Mover.cpp:1530)
    model.type_name = "test"
    model.add_component(build_power_supply(110.0))
    model.power = ENGINE_POWER
    physics_node = build_vehicle_node(vehicle_name, model, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    engine = MoverRailVehicleElectricSeriesEngine.new()
    var power_source:RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    power_source.current_collector_physical_layout = 1
    power_source.current_collector_max_voltage = 3600.0
    power_source.current_collector_number_of_collectors = 1
    power_source.current_collector_max_pantograph_tank_pressure = PANTOGRAPH_TANK_PRESSURE
    var controller:VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())
    controller.add_component(engine)
    controller.add_component(power_source)
    vehicle = RailVehicle3D.new()
    vehicle.start_track_name = "start"
    vehicle.start_track_offset = 20.0
    var alongs:Array[float] = [FRONT_ALONG]
    _add_model(alongs)
    vehicle.pantograph_front_arm_paths = _arm_paths(0)
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    await wait_idle_frames(2)
    # no main reservoir here: the tank filled by the small compressor, as PrepareEngine() does
    controller.send_command("battery", true)
    # only a vehicle with its cab switched on is simulated (FINDINGS.md, 09-23)
    controller.send_command("cab_activation", true)
    controller.send_command("pantograph_compressor_valve", true)
    controller.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    for step:int in MAX_RAISE_STEPS:
        # held, as the driver holds it (Train.cpp:2912): it starts once the battery feeds 24 V
        controller.send_command("pantograph_compressor", true)
        VehicleServer.stepping_advance(STEP)
        if _front_voltage(controller) > POWERED_VOLTAGE:
            break
    return controller


## FINDINGS.md 2026-09-29: entering the cab of a running 3E/1-42 rebuilt its model, and the arms
## taken again from the model's rest pose came down - the vehicle lost its voltage and its line
## breaker opened. How far a pantograph is raised is the vehicle's own state (RailVehicleServer),
## and a model rebuilt to draw it keeps it.
func test_a_model_rebuilt_keeps_the_pantograph_at_the_wire() -> void:
    var controller:VehicleController = await _raise_pantograph_at_wire("test_pantograph_rebuilt")
    assert_gt(_front_voltage(controller), POWERED_VOLTAGE, "the raised pantograph reaches the wire")

    # the model rebuilt: new arm nodes, at rest, as a model loaded again puts them
    (vehicle.get_node("Model") as E3DModelInstance).reload()
    VehicleServer.stepping_advance(STEP)

    assert_gt(_front_voltage(controller), POWERED_VOLTAGE, "and stays at it when the model is rebuilt")

    # FINDINGS.md 2026-09-29: the wire gone for less than the original's 0.2 s - the arm catching up
    # at a switch - is held and does not trip the line breaker; for longer it is not
    TractionServer.wire_free(_wires[0])
    _wires.clear()
    TractionServer.network_build()
    VehicleServer.stepping_advance(SHORT_LOSS)
    assert_gt(_vehicle_voltage(controller), POWERED_VOLTAGE, "a short loss keeps the vehicle's voltage")
    VehicleServer.stepping_advance(LONG_LOSS)
    assert_eq(_vehicle_voltage(controller), 0.0, "a longer one does not")


## The pantograph losing its wire is announced with the cause, beside the warning (Bad traction,
## scene.cpp:112) - what the game logs to place a main switch trip (#308, reports#16)
func test_a_pantograph_losing_its_wire_announces_the_loss() -> void:
    var controller:VehicleController = await _raise_pantograph_at_wire("test_pantograph_contact_lost")
    assert_gt(_front_voltage(controller), POWERED_VOLTAGE, "the raised pantograph reaches the wire")
    var losses:Array = []
    var record:Callable = func(lost:RID, pantograph:int, cause:RailVehicleServer.PantographContactLoss) -> void:
        losses.append([lost, pantograph, cause])
    RailVehicleServer.vehicle_pantograph_contact_lost.connect(record)

    TractionServer.wire_free(_wires[0])
    _wires.clear()
    TractionServer.network_build()
    VehicleServer.stepping_advance(STEP)

    RailVehicleServer.vehicle_pantograph_contact_lost.disconnect(record)
    assert_eq(losses, [[controller.get_rid(), 0, RailVehicleServer.PANTOGRAPH_CONTACT_LOSS_NO_WIRE]],
            "the loss of the wire under the front pantograph, once")


func _vehicle_voltage(controller:VehicleController) -> float:
    return float(VehicleServer.vehicle_dump_state(controller.get_rid()).get("current_collector/voltage", 0.0))


func _front_voltage(controller:VehicleController) -> float:
    return float(VehicleServer.vehicle_dump_state(controller.get_rid()).get("current_collector/pantograph_first_voltage", 0.0))
