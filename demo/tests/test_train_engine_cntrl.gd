extends MaszynaGutTest

var train: VehicleController
var engine: RailVehicleDieselEngine

func before_each():
    train = build_vehicle("TestTrain")

    engine = MoverRailVehicleDieselEngine.new()
    train.add_component(engine)
    await wait_idle_frames(2)

func test_defaults():
    assert_eq(engine.cntrl_auto_relay_mode, RailVehicleEngine.AUTO_RELAY_NO)
    assert_false(engine.cntrl_has_camshaft)
    assert_eq(engine.motor_blowers_start_mode, RailVehicleController.START_MODE_MANUAL)
    assert_eq(engine.fuel_pump_start_mode, RailVehicleController.START_MODE_MANUAL)
    assert_eq(engine.oil_pump_start_mode, RailVehicleController.START_MODE_MANUAL)
    assert_eq(engine.water_pump_start_mode, RailVehicleController.START_MODE_MANUAL)

func test_round_trip_and_update_without_crashing():
    engine.cntrl_eim_control_additional_zeros = true
    engine.cntrl_eim_control_emergency = true
    engine.cntrl_eim_control_type = RailVehicleEngine.EIM_CONTROL_TYPE_2
    engine.cntrl_auto_relay_mode = RailVehicleEngine.AUTO_RELAY_YES
    engine.cntrl_has_camshaft = true
    engine.cntrl_series_shunt_on_series_position = true
    engine.cntrl_fast_series_circuit = true
    engine.fuel_pump_start_mode = RailVehicleController.START_MODE_AUTOMATIC
    engine.oil_pump_start_mode = RailVehicleController.START_MODE_AUTOMATIC
    engine.water_pump_start_mode = RailVehicleController.START_MODE_BATTERY
    await wait_idle_frames(2)

    assert_eq(engine.cntrl_auto_relay_mode, RailVehicleEngine.AUTO_RELAY_YES)
    assert_true(engine.cntrl_has_camshaft)
    assert_eq(engine.fuel_pump_start_mode, RailVehicleController.START_MODE_AUTOMATIC)
    assert_true(train.get_state().has("main_switch_enabled"), "RailVehicleEngine should keep functioning after configuring the Cntrl. section")


func test_master_controller_positions_reach_the_vehicle_config():
    var master_controller: RailVehicleMasterController = MoverRailVehicleMasterController.new()
    master_controller.main_position_count = 5
    master_controller.second_position_count = 3
    master_controller.direction_change_max_position = 1
    master_controller.coupled_controllers = true
    master_controller.initial_delay = 1.5
    master_controller.step_delay = 0.5
    master_controller.step_down_delay = 0.3
    train.add_component(master_controller)
    await wait_idle_frames(2)

    var config: Dictionary = train.get_config()
    assert_eq(config.get("main_controller_position_max"), 5)
    assert_eq(config.get("second_controller_position_max"), 3)


func test_cntrl_oil_start_is_applied_to_diesel_engine():
    FizTrainEngineCommon.apply_cntrl_engine_subset(engine, {"OilStart": "Automatic"})
    assert_eq(engine.oil_pump_start_mode, RailVehicleController.START_MODE_AUTOMATIC)
