extends MaszynaGutTest

var train: TrainController
var engine: TrainDieselEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainDieselEngine.new()
    train.add_child(engine)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(engine.get("cntrl/main_controller_position_count"), 0)
    assert_eq(engine.get("cntrl/auto_relay_mode"), TrainEngine.AUTO_RELAY_NO)
    assert_false(engine.get("cntrl/coupled_controllers"))
    assert_false(engine.get("cntrl/has_camshaft"))
    assert_eq(engine.get("motor_blowers/start_mode"), TrainEngine.START_MODE_MANUAL)
    assert_eq(engine.get("fuel_pump/start_mode"), TrainEngine.START_MODE_MANUAL)
    assert_eq(engine.get("oil_pump/start_mode"), TrainEngine.START_MODE_MANUAL)
    assert_eq(engine.get("water_pump/start_mode"), TrainEngine.START_MODE_MANUAL)

func test_round_trip_and_update_without_crashing():
    engine.set("cntrl/main_controller_position_count", 5)
    engine.set("cntrl/shunt_controller_position_count", 3)
    engine.set("cntrl/direction_change_max_position", 1)
    engine.set("cntrl/eim_control_additional_zeros", true)
    engine.set("cntrl/eim_control_emergency", true)
    engine.set("cntrl/eim_control_type", TrainEngine.EIM_CONTROL_TYPE_2)
    engine.set("cntrl/auto_relay_mode", TrainEngine.AUTO_RELAY_YES)
    engine.set("cntrl/coupled_controllers", true)
    engine.set("cntrl/has_camshaft", true)
    engine.set("cntrl/series_shunt_on_series_position", true)
    engine.set("cntrl/initial_controller_delay", 1.5)
    engine.set("cntrl/controller_step_delay", 0.5)
    engine.set("cntrl/controller_step_down_delay", 0.3)
    engine.set("cntrl/fast_series_circuit", true)
    engine.set("fuel_pump/start_mode", TrainEngine.START_MODE_AUTOMATIC)
    engine.set("oil_pump/start_mode", TrainEngine.START_MODE_AUTOMATIC)
    engine.set("water_pump/start_mode", TrainEngine.START_MODE_BATTERY)
    await wait_idle_frames(2)

    assert_eq(engine.get("cntrl/main_controller_position_count"), 5)
    assert_eq(engine.get("cntrl/auto_relay_mode"), TrainEngine.AUTO_RELAY_YES)
    assert_true(engine.get("cntrl/has_camshaft"))
    assert_eq(engine.get("fuel_pump/start_mode"), TrainEngine.START_MODE_AUTOMATIC)
    assert_true(train.state.has("main_switch_enabled"), "TrainEngine should keep functioning after configuring the Cntrl. section")
