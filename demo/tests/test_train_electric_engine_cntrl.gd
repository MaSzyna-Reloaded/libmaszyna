extends MaszynaGutTest

var train: VehicleController
var engine: RailVehicleElectricSeriesEngine
var power_source: RailVehicleEnginePowerSource

func before_each():
    train = build_vehicle("TestTrain")

    engine = MoverRailVehicleElectricSeriesEngine.new()
    train.add_component(engine)
    power_source = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_ACCUMULATOR
    train.add_component(power_source)
    await step(2)

func test_defaults():
    assert_false(power_source.cntrl_pantograph_auto_valve)
    assert_eq(engine.cntrl_main_switch_start_mode, RailVehicleController.START_MODE_MANUAL)

func test_round_trip_and_update_without_crashing():
    engine.cntrl_converter_overload_relay_start_mode = RailVehicleController.START_MODE_CONVERTER
    engine.cntrl_converter_overload_relay_off_when_main_is_off = true
    power_source.cntrl_pantograph_compressor_start_mode = RailVehicleController.START_MODE_AUTOMATIC
    power_source.cntrl_pantograph_auto_valve = true
    engine.cntrl_main_switch_start_mode = RailVehicleController.START_MODE_AUTOMATIC
    await step(2)

    assert_true(power_source.cntrl_pantograph_auto_valve)
    assert_true(train.get_state().has("main_switch_enabled"), "RailVehicleElectricEngine should keep functioning after configuring the Cntrl. section")


func _pantograph_vehicle(master_valve_start:RailVehicleController.StartMode) -> VehicleController:
    var vehicle:VehicleController = build_vehicle("TestPantographValves")
    vehicle.add_component(build_power_supply(110.0))
    var electric := MoverRailVehicleElectricSeriesEngine.new()
    vehicle.add_component(electric)
    var collector := MoverRailVehicleEnginePowerSource.new()
    collector.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    collector.current_collector_number_of_collectors = 1
    collector.cntrl_pantographs_valve_start_mode = master_valve_start
    vehicle.add_component(collector)
    vehicle.apply_configuration()
    await step(2)
    vehicle.send_command("battery", true)
    await step(2)
    return vehicle


# LoadFIZ_Cntrl (Mover.cpp:10930) - without PantEPValveStart the master valve opens by itself with
# low voltage, which is what lets an EP07 raise a pantograph from its own switch alone
func test_the_pantographs_master_valve_is_automatic_by_default():
    var vehicle:VehicleController = await _pantograph_vehicle(RailVehicleController.START_MODE_AUTOMATIC)
    assert_true(vehicle.get_state()["current_collector/valve_active"])


# PantEPValveStart=Manual (dynamic/pkp/e186_v2) - it waits for the pantograph lever
func test_a_manual_master_valve_waits_for_the_lever():
    var vehicle:VehicleController = await _pantograph_vehicle(RailVehicleController.START_MODE_MANUAL)
    assert_false(vehicle.get_state()["current_collector/valve_active"])
