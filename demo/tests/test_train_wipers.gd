extends MaszynaGutTest

## RailVehicleWipers: the wiper switch (Train.cpp:2638-2661) and the movement of the wipers
## (DynObj.cpp:4048-4115), both kept in the node - the vendored Mover has neither.


var train: VehicleController
var wipers: RailVehicleWipers


func before_each():
    train = build_vehicle("TestTrainWipers", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    train.add_component(build_power_supply(110.0))
    train.apply_configuration()
    wipers = MoverRailVehicleWipers.new()
    # WiperList: of ep09_v2/104e-mod-dod-zal.fiz - mask, sweep time, interval, delay at the far end
    wipers.positions = [
        _item(0, 1.0, 0.0, 0.5),
        _item(3, 1.0, 5.0, 0.5),
        _item(3, 0.2, 0.0, 0.1),
    ]
    train.add_component(wipers)
    # a VehicleComponent takes its configuration and its commands on its first step
    if not await wait_simulated_until(func() -> bool: return train.get_state()["wiper_positions"].size() > 0,
            TICK, "the wipers' first step"):
        return


func _item(mask: int, transit_time: float, period: float, return_delay: float) -> RailVehicleWiperListItem:
    var item: RailVehicleWiperListItem = RailVehicleWiperListItem.new()
    item.wiper_mask = mask
    item.transit_time = transit_time
    item.period = period
    item.return_delay = return_delay
    return item


func test_defaults():
    var defaults: RailVehicleWipers = MoverRailVehicleWipers.new()

    assert_eq(defaults.angle, 0.0)
    assert_eq(defaults.default_position, 0)
    assert_eq(defaults.positions.size(), 0)


func test_round_trip_and_update_without_crashing():
    wipers.angle = 58.0
    wipers.default_position = 2
    wipers.positions = [
        _item(0, 0.7, 0.0, 0.5),
        _item(3, 0.7, 7.0, 0.5),
        _item(3, 0.7, 0.7, 0.0),
        _item(3, 0.5, 0.5, 0.0),
    ]
    # a step with the new list
    await step(1)

    assert_eq(wipers.angle, 58.0)
    assert_eq(wipers.positions.size(), 4)
    assert_eq((wipers.positions[1] as RailVehicleWiperListItem).period, 7.0)
    assert_true(is_instance_valid(train), "VehicleController should keep functioning after configuring RailVehicleWipers")


func test_switch_is_limited_to_the_wiper_list():
    assert_eq(train.get_state()["wipers_switch_position"], 0)
    assert_eq(train.get_config()["wipers_switch_position_max"], 2)

    for i in 5:
        train.send_command("wipers_switch_increase")
    assert_eq(train.get_state()["wipers_switch_position"], 2)

    for i in 5:
        train.send_command("wipers_switch_decrease")
    assert_eq(train.get_state()["wipers_switch_position"], 0)


func test_wiper_count_comes_from_the_masks():
    assert_eq(train.get_state()["wiper_positions"].size(), 2)


func test_wipers_sweep_out_and_back_with_active_cab_and_battery():
    train.send_command("battery", true)
    train.send_command("cab_activation", true)
    train.send_command("wipers_switch_increase")
    train.send_command("wipers_switch_increase")

    # the wiper is out and back past its far end in the transit time of the position it starts its
    # sweep with - a parked wiper holds the list's first one, the switch is taken once it parks again
    # (workingSwitchPos, DynObj.cpp:4150, 4197); that position's period is 0, so it starts on
    # the next step. [out, return]
    var reached: Array[bool] = [false, false]
    var working: RailVehicleWiperListItem = wipers.positions[0]
    if not await wait_simulated_until(func() -> bool:
            var position: float = train.get_state()["wiper_positions"][0]
            reached[0] = reached[0] or (position > 0.0 and position <= 1.0)
            reached[1] = reached[1] or position > 1.0
            return reached[0] and reached[1], working.transit_time + TICK, "the wiper's sweep out and back"):
        return

    assert_true(reached[0], "wiper should sweep out (0..1)")
    assert_true(reached[1], "wiper should come back (1..2)")


func test_wipers_stay_parked_without_battery():
    train.send_command("cab_activation", true)
    train.send_command("wipers_switch_increase")
    train.send_command("wipers_switch_increase")
    # the switched position's period is 0: a wiper that ran would be out on the first step
    await step(1)

    assert_eq(train.get_state()["wiper_positions"][0], 0.0)


## e186_v2 has four wipers, two per end, and a list that switches wipers 1 and 2: from cab 1 these
## are the first two, the wipers of the other end stay parked (DynObj.cpp:4056-4063)
func test_wiper_count_of_the_model_limits_the_sweep_to_the_active_end():
    wipers.wiper_count = 4
    wipers.apply_config()
    train.send_command("battery", true)
    train.send_command("cab_activation", true)
    train.send_command("wipers_switch_increase")
    train.send_command("wipers_switch_increase")
    # the switched position's period is 0: the wipers that run are out on the first step, and the
    # other end's would be as well
    if not await wait_simulated_until(func() -> bool:
            var swept: PackedFloat64Array = train.get_state()["wiper_positions"]
            return swept[0] > 0.0 and swept[1] > 0.0, TICK, "the active end's wipers out"):
        return

    var positions: PackedFloat64Array = train.get_state()["wiper_positions"]
    assert_eq(positions.size(), 4)
    assert_gt(positions[0], 0.0)
    assert_gt(positions[1], 0.0)
    assert_eq(positions[2], 0.0)
    assert_eq(positions[3], 0.0)
