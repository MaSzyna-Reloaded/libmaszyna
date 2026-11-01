extends MaszynaGutTest

## The valves of wipers driven by hand (RailVehicleWipers.valve_set_opening(), an SM42's, an EU07's):
## each wiper's own, going by the air of the main reservoir as far as it is open - a closed one, or
## no air, leaves the wiper where the hand puts it.

const VEHICLE_OF_AIR_PATH:String = "res://tests/fixtures/test_vehicle.fiz"
## The share of the valve's full speed its standing reservoir gives, between 1.5 and 3.5 bar
var _air_share:float = 1.0
const SLOWEST_SWEEP_TIME:float = 0.8
const FASTEST_SWEEP_TIME:float = 0.1
const SENSITIVITY:float = 3.0

var train:VehicleController
var wipers:MaszynaMoverRailVehicleWipers


func _build(with_air:bool) -> void:
    # a vehicle of air as the data gives it, standing; of none, its description bare
    var description:VehicleController = MaszynaMoverRailVehicleController.new()
    if with_air:
        description = FizVehicleBuilder.build_description_at(VEHICLE_OF_AIR_PATH)
    else:
        description.add_component(build_power_supply(110.0))
        description.add_component(MaszynaMoverRailVehicleWipers.new())
    train = build_vehicle("TestWiperValves", description, 0.0, MaszynaVehicleData.DriverType.DRIVER_HEAD)
    wipers = VehicleServer.vehicle_component_get(train.get_rid(), VehicleComponentType.COMPONENT_WIPERS) as MaszynaMoverRailVehicleWipers
    wipers.valve_slowest_sweep_time = SLOWEST_SWEEP_TIME
    wipers.valve_fastest_sweep_time = FASTEST_SWEEP_TIME
    wipers.valve_sensitivity = SENSITIVITY
    var no_switch:Array[RailVehicleWiperListItem] = []
    wipers.positions = no_switch
    wipers.manual_fit(2)
    await step(1)
    _air_share = clampf((float(train.get_state().get("compressor_pressure", 0.0)) - 1.5) / 2.0, 0.0, 1.0)


func test_a_closed_valve_leaves_the_wiper_to_the_hand() -> void:
    await _build(true)
    train.send_command("wipers_manual_set_0", 0.4)
    await step(ticks(0.2))
    assert_almost_eq(wipers.manual_get_position(0), 0.4, 0.001)
    assert_eq(wipers.valve_get_opening(0), 0.0)


func test_an_open_valve_moves_its_own_wiper_alone() -> void:
    await _build(true)
    train.send_command("wipers_valve_set_0", 1.0)
    assert_eq(train.get_state()["wipers_valve_0"], 1.0)
    await step(ticks(0.1))
    assert_gt(wipers.manual_get_position(0), 0.0, "the wiper of the open valve goes by itself")
    assert_eq(wipers.manual_get_position(1), 0.0, "the other's stands")


func test_the_valve_is_sensitive_a_quarter_turn_well_on_the_way_to_the_fastest() -> void:
    await _build(true)
    train.send_command("wipers_valve_set_0", 1.0)
    train.send_command("wipers_valve_set_1", 0.25)
    await step(1)
    var seconds:float = wipers.manual_get_position(0) * FASTEST_SWEEP_TIME / _air_share
    assert_almost_eq(seconds, TICK, TICK * 0.5, "a sweep's time at a full valve and this reservoir")
    var share:float = 1.0 - pow(0.75, SENSITIVITY)
    assert_almost_eq(wipers.manual_get_position(0) / wipers.manual_get_position(1),
            pow(SLOWEST_SWEEP_TIME / FASTEST_SWEEP_TIME, 1.0 - share), 0.2, "a quarter of the turn, most of the speed")


func test_the_motor_takes_the_wiper_there_and_back() -> void:
    await _build(true)
    train.send_command("wipers_valve_set_0", 1.0)
    assert_true(await wait_simulated_until(func() -> bool: return wipers.manual_get_position(0) >= 1.0, 3.0, "the far end"))
    assert_true(await wait_simulated_until(func() -> bool: return wipers.manual_get_position(0) <= 0.5, 3.0, "on the way back"))


func test_closed_again_the_wiper_stands_where_it_is() -> void:
    await _build(true)
    train.send_command("wipers_valve_set_all", 1.0)
    await step(ticks(0.1))
    train.send_command("wipers_valve_set_all", 0.0)
    var stopped:float = wipers.manual_get_position(0)
    assert_gt(stopped, 0.0)
    await step(ticks(0.2))
    assert_eq(wipers.manual_get_position(0), stopped)


func test_no_air_no_motor_and_the_hand_still_moves_it() -> void:
    await _build(false)
    train.send_command("wipers_valve_set_0", 1.0)
    await step(ticks(0.2))
    assert_eq(wipers.manual_get_position(0), 0.0, "an open valve of no air moves nothing")
    train.send_command("wipers_manual_set_0", 0.6)
    await step(ticks(0.1))
    assert_almost_eq(wipers.manual_get_position(0), 0.6, 0.001, "the hand needs no air")


func test_each_vehicle_has_its_own_speeds() -> void:
    await _build(true)
    wipers.valve_fastest_sweep_time = FASTEST_SWEEP_TIME * 4.0
    train.send_command("wipers_valve_set_0", 1.0)
    await step(1)
    var seconds:float = wipers.manual_get_position(0) * FASTEST_SWEEP_TIME * 4.0 / _air_share
    assert_almost_eq(seconds, TICK, TICK * 0.5, "a sweep's time at a full valve of this vehicle")


func test_a_vehicle_built_keeps_the_speeds_of_its_description() -> void:
    var description:VehicleController = MaszynaMoverRailVehicleController.new()
    description.add_component(build_power_supply(110.0))
    var described := MaszynaMoverRailVehicleWipers.new()
    described.valve_slowest_sweep_time = 2.0
    described.valve_fastest_sweep_time = 0.5
    described.valve_sensitivity = 1.5
    description.add_component(described)
    train = build_vehicle("TestWiperValves", description, 0.0, MaszynaVehicleData.DriverType.DRIVER_HEAD)
    var built:MaszynaMoverRailVehicleWipers = VehicleServer.vehicle_component_get(
            train.get_rid(), VehicleComponentType.COMPONENT_WIPERS) as MaszynaMoverRailVehicleWipers
    assert_eq(built.valve_slowest_sweep_time, 2.0)
    assert_eq(built.valve_fastest_sweep_time, 0.5)
    assert_eq(built.valve_sensitivity, 1.5)
