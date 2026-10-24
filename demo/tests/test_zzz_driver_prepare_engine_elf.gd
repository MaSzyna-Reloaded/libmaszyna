extends MaszynaGutTest

## The AI driver prepares EN76-001 (elf_v1, fixtures/scenery/startup_en76-001-a.scn, cut from
## Wrzosy's ic8310_dekoracje.scm: four cars, the A car with pantograph A and the D car with
## pantograph B, each of them powered with a line breaker of its own) to the end of the start
## sequence. The original asks for both pantographs, consist-wide, and
## takes one as raised by the pantograph unit's valve of it (Driver.cpp:2811-2813,
## driverhints.cpp:269-310); the B cars' line breakers count for its readiness
## (Driver.cpp:6143-6144). Told to go, it puts its Elf controller (EIMCtrlType=2) at the driving
## position at once, as the original sets MainCtrlPos (Driver.cpp:3784, 4289); stepped through the
## cab on every update, the relay time started again and the power never rose past its first step
## (CheckEIMIC(), Mover.cpp). Report 2026-10-10 (Wrzosy, IC EIE8310): the EN76 stood at "prepare the
## vehicle", its D car's pantograph never raised, and then at "increase tractive force"; the
## scenario waiting for it never went on.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "startup_en76-001-a.scn"
const DRIVEN:String = "en76-001-a"
## The driver's update it takes the unit ready on: both pantographs raised, the line breakers
## closed and the converters on by the tenth (20.2 s measured) - each PREPARE_TIME after the one
## before, the first within PREPARE_TIME of the order
const READY_UPDATE:int = 11
## The speed asked for on the way off [km/h]
const DEPARTURE_VELOCITY:float = 60.0
## The Elf's power the unit moves off with (0.097 measured) - it rises step_delay a second once
## the controller has stood at driving initial_delay (CheckEIMIC(), Mover.cpp)
const MOVING_POWER:float = 0.1

var _previous_game_dir:String = ""
var scenery:MaszynaSceneryNode
var vehicle_rid:RID


func before_each():
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    scenery = MaszynaSceneryNode.new()
    scenery.filename = SCENERY
    add_child(scenery)
    # the scenery is announced once its vehicles are built and its drivers given their AI
    if not await wait_loaded(scenery.scenery_loaded, SCENERY):
        return
    vehicle_rid = VehicleServer.vehicle_get_rid_by_id(DRIVEN)


func after_each():
    scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_the_driver_raises_the_pantographs_of_every_car_and_takes_the_unit_ready() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(vehicle_rid)
    assert_true(driver.is_valid(), "%s has its scenery driver (headdriver)" % DRIVEN)
    if not driver.is_valid():
        return
    DriverServer.driver_send_command(driver, "Prepare_engine", 1.0, 0.0)
    var ready:Callable = func() -> bool: return get_driver_internal_state(driver).get("engine_active", false)
    if not await wait_simulated_until(ready, READY_UPDATE * MaszynaAIDriver.PREPARE_TIME, "the unit ready for its driver"):
        gut.p(_cars())
        return
    for car:RID in RailVehicleServer.vehicle_get_coupled(
            vehicle_rid, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL):
        # the B and C cars have no engine, nor a line breaker
        var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
                car, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
        if engine:
            assert_true(engine.get_main_switch_enabled(), "%s's line breaker closed: %s" % [
                    VehicleServer.vehicle_get_id(car), _cars()])


func test_the_driver_told_to_go_moves_the_unit_off() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(vehicle_rid)
    assert_true(driver.is_valid(), "%s has its scenery driver (headdriver)" % DRIVEN)
    if not driver.is_valid():
        return
    DriverServer.driver_send_command(driver, "Prepare_engine", 1.0, 0.0)
    var ready:Callable = func() -> bool: return get_driver_internal_state(driver).get("engine_active", false)
    if not await wait_simulated_until(ready, READY_UPDATE * MaszynaAIDriver.PREPARE_TIME, "the unit ready for its driver"):
        return
    DriverServer.driver_send_command(driver, "SetVelocity", DEPARTURE_VELOCITY, DEPARTURE_VELOCITY)
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    # the driver's next update puts the controller at driving, the Elf raises its power once the
    # controller has stood there InitialCtrlDelay (3.5 s after the order measured)
    var moving:Callable = func() -> bool:
        return VehicleServer.vehicle_get_speed(vehicle_rid) > MaszynaDriverTrainset.NO_MOVEMENT_SPEED
    if not await wait_simulated_until(moving, MaszynaAIDriver.PREPARE_TIME + master.initial_delay
            + MOVING_POWER / master.step_delay + TICK, "the unit moving off"):
        var engine:MaszynaMoverRailVehicleElectricInductionEngine = VehicleServer.vehicle_component_get(
                vehicle_rid, VehicleComponentType.COMPONENT_ENGINE) as MaszynaMoverRailVehicleElectricInductionEngine
        gut.p("controller %d, power %.3f" % [master.get_main_position(), engine.get_eimic_real()])


func _cars() -> String:
    var cars:Array[String] = []
    for car:RID in VehicleServer.vehicle_get_rids():
        var source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
                car, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
        var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
                car, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
        cars.append("%s main_switch=%s pantographs=%s/%s voltage=%.0f" % [
                VehicleServer.vehicle_get_id(car), engine.get_main_switch_enabled() if engine else false,
                source.get_collector_pantograph_first_active() if source else false,
                source.get_collector_pantograph_second_active() if source else false,
                source.get_collector_voltage() if source else 0.0])
    return "; ".join(cars)
