extends MaszynaGutTest

var train: TrainController
var engine: TrainElectricSeriesEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainElectricSeriesEngine.new()
    engine.set("power/source", TrainController.POWER_SOURCE_ACCUMULATOR)
    train.add_child(engine)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(engine.get("cntrl/converter_start_mode"), TrainEngine.START_MODE_MANUAL)
    assert_false(engine.get("cntrl/pantograph_auto_valve"))
    assert_eq(engine.get("cntrl/main_switch_start_mode"), TrainEngine.START_MODE_MANUAL)

func test_round_trip_and_update_without_crashing():
    engine.set("cntrl/converter_start_mode", TrainEngine.START_MODE_AUTOMATIC)
    engine.set("cntrl/converter_start_delay", 2.0)
    engine.set("cntrl/converter_overload_relay_start_mode", TrainEngine.START_MODE_CONVERTER)
    engine.set("cntrl/converter_overload_relay_off_when_main_is_off", true)
    engine.set("cntrl/pantograph_compressor_start_mode", TrainEngine.START_MODE_AUTOMATIC)
    engine.set("cntrl/pantograph_auto_valve", true)
    engine.set("cntrl/main_switch_start_mode", TrainEngine.START_MODE_AUTOMATIC)
    await wait_idle_frames(2)

    assert_eq(engine.get("cntrl/converter_start_mode"), TrainEngine.START_MODE_AUTOMATIC)
    assert_true(engine.get("cntrl/pantograph_auto_valve"))
    assert_true(train.state.has("main_switch_enabled"), "TrainElectricEngine should keep functioning after configuring the Cntrl. section")
