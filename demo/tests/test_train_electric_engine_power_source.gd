extends MaszynaGutTest

## Regression test: the power source's state fetch (once RailVehicleElectricEngine's
## _do_fetch_state_from_mover, now RailVehicleEnginePowerSource's) used to unconditionally
## reverse-map EnginePowerSource.RAccumulator.RechargeSource and .RPowerCable.PowerTrans, both
## of which are only initialized by the configuration when source_type is the matching variant
## (Accumulator / PowerCable respectively). For any other source_type - notably CurrentCollector,
## used by every real pantograph-powered electric locomotive - those fields held uninitialized
## memory, and get_state() crashed the whole process with an uncaught std::out_of_range from
## std::map::at() on the very first _process() tick.

var train: VehicleController


func before_each():
    train = build_vehicle()


func test_current_collector_power_source_does_not_crash_on_process():
    var power_source := MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    train.add_component(power_source)
    await wait_idle_frames(3)

    assert_true(power_source.get_state().has("power_source"))
    assert_false(
            power_source.get_state().has("accumulator/recharge_source"),
            "recharge_source wasn't configured for this power source and shouldn't be reported")


func test_default_power_source_does_not_crash_on_process():
    # The compiled default (source_type == NotDefined) hits the same unconditional-read path.
    var power_source := MoverRailVehicleEnginePowerSource.new()
    train.add_component(power_source)
    await wait_idle_frames(3)

    assert_true(power_source.get_state().has("power_source"))


func test_accumulator_power_source_still_reports_recharge_source():
    var power_source := MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_ACCUMULATOR
    power_source.accumulator_recharge_source = RailVehicleController.POWER_SOURCE_GENERATOR
    train.add_component(power_source)
    await wait_idle_frames(3)

    assert_eq(power_source.get_state().get("accumulator/recharge_source"), RailVehicleController.POWER_SOURCE_GENERATOR)
