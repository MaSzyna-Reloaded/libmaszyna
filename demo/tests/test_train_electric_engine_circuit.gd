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

func test_circuit_defaults():
    assert_eq(engine.get("circuit/resistance"), 0.0)
    assert_eq(engine.get("circuit/imax_low"), 0)
    assert_eq(engine.get("circuit/imax_high"), 0)
    assert_eq(engine.get("circuit/tuhex/sum"), 750.0)
    assert_eq(engine.get("circuit/tuhex/diff"), 10.0)
    assert_eq(engine.get("circuit/tuhex/min_current"), 60.0)
    assert_eq(engine.get("circuit/tuhex/max_current"), 400.0)
    assert_eq(engine.get("circuit/tuhex/stages"), 0)
    assert_eq(engine.get("power/current_collector/physical_layout"), 0)

func test_circuit_round_trip_and_update():
    engine.set("circuit/resistance", 0.35)
    engine.set("circuit/imax_low", 600)
    engine.set("circuit/imax_high", 900)
    engine.set("circuit/imin_low", 100)
    engine.set("circuit/imin_high", 150)
    engine.set("circuit/tuhex/sum", 800.0)
    engine.set("circuit/tuhex/diff", 12.0)
    engine.set("circuit/tuhex/stages", 3)
    engine.set("circuit/tuhex/sum_1", 800.0)
    engine.set("circuit/tuhex/sum_2", 700.0)
    engine.set("circuit/tuhex/sum_3", 600.0)
    await wait_idle_frames(2)

    assert_eq(engine.get("circuit/resistance"), 0.35)
    assert_eq(engine.get("circuit/imax_low"), 600)
    assert_eq(engine.get("circuit/imax_high"), 900)
    assert_eq(engine.get("circuit/tuhex/stages"), 3)
    assert_true(train.state.has("main_switch_enabled"), "TrainElectricEngine should keep functioning after configuring the Circuit section")

func test_physical_layout_updates_without_crashing():
    engine.set("power/source", TrainController.POWER_SOURCE_CURRENTCOLLECTOR)
    engine.set("power/current_collector/physical_layout", 3) # front and rear
    await wait_idle_frames(2)

    assert_eq(engine.get("power/current_collector/physical_layout"), 3)
