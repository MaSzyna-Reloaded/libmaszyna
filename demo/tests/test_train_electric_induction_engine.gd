extends MaszynaGutTest

## The overhead wire's voltage the tests stand under [V]
const WIRE_VOLTAGE:float = 3000.0
## Ticks the closed line breaker is watched under the wire: the voltage check that opened it ran on
## the step after it closed (TractionForce(), Mover.cpp) - a few steps show it holds
const KEPT_CLOSED_TICKS:int = 5

var train: VehicleController
var engine: RailVehicleElectricInductionEngine

func before_each():
    train = build_vehicle("TestTrain")

    engine = MoverRailVehicleElectricInductionEngine.new()
    train.add_component(engine)
    var power_source: RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    train.add_component(power_source)
    await step(2)

func _make_point(x: float, y: float) -> VehicleCurvePointItem:
    var item = VehicleCurvePointItem.new()
    item.x = x
    item.y = y
    return item

func test_defaults():
    assert_eq(engine.slip_current_ratio, 0.0)
    assert_eq(engine.pole_pairs, 0.0)
    assert_eq(engine.max_power, 0.0)
    assert_eq(engine.motor_max_current, 0.0)
    assert_eq(engine.max_power_table.size(), 0)

func test_round_trip_and_update_without_crashing():
    engine.slip_current_ratio = 0.1
    engine.max_slip = 0.2
    engine.pole_pairs = 2.0
    engine.nominal_uf_ratio = 1.5
    engine.current_torque_ratio = 0.9
    engine.current_three_phase_ratio = 0.8
    engine.max_supply_voltage = 2800.0
    engine.max_supply_voltage_braking = 2400.0
    engine.inverter_voltage_drop = 10.0
    engine.no_load_current = 5.0
    engine.inverter_uf_setpoint = 1.0
    engine.inverter_uf_setpoint_braking = 0.9
    engine.initial_force = 200.0
    engine.force_drop_rate = 1.5
    engine.max_power = 1200.0
    engine.max_braking_force = 180.0
    engine.max_braking_power = 1000.0
    engine.braking_decay_velocity = 5.0
    engine.braking_decay_start_velocity = 20.0
    engine.motor_max_current = 600.0
    engine.max_power_table = [_make_point(0.0, 1200.0), _make_point(100.0, 600.0)]
    await step(2)

    assert_eq(engine.slip_current_ratio, 0.1)
    assert_eq(engine.max_power, 1200.0)
    assert_eq(engine.max_power_table.size(), 2)
    assert_true(train.get_state().has("main_switch_enabled"), "RailVehicleElectricInductionEngine should keep functioning after configuring EIM parameters")

func test_line_breaker_stays_closed_under_the_nominal_wire_voltage():
    # Regression: CollectorParameters.MaxV (FIZ MaxVoltage, Mover.cpp:11622) was never set, so an
    # induction motor opened the line breaker above 0 + 200 V right after it closed. The cab is
    # occupied - an unmanned vehicle is not simulated and would never open it.
    var driven:VehicleController = build_vehicle("TestEimTrain", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    driven.add_component(build_power_supply(110.0))
    # the breaker is checked against the voltage in TractionForce(), run only with Power > 0
    driven.power = 5600.0
    var eim: RailVehicleElectricInductionEngine = MoverRailVehicleElectricInductionEngine.new()
    var power_source: RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    var master_controller: RailVehicleMasterController = MoverRailVehicleMasterController.new()
    master_controller.main_position_count = 4
    driven.add_component(master_controller)
    power_source.current_collector_max_voltage = 3900.0
    power_source.current_collector_min_main_switch_voltage = 1900.0
    power_source.current_collector_physical_layout = 3
    power_source.current_collector_number_of_collectors = 2
    driven.add_component(eim)
    driven.add_component(power_source)
    driven.apply_configuration()
    await step(1)
    driven.send_command("battery", true)
    driven.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    if not await _wire_until_closable(driven, power_source):
        return

    driven.send_command("main_switch", true)
    for _tick:int in KEPT_CLOSED_TICKS:
        _feed_wire(power_source)
        await step(1)

    assert_true(driven.get_state()["main_switch_enabled"], "the line breaker should stay closed at 3000 V")


## A driven E186-like vehicle (the Engine: line of dynamic/pkp/e186_v2/p160dc.fiz, without InvNo)
## under 3000 V, with the line breaker closed and a direction set.
## Built as the game builds a vehicle: every component in its description before the vehicle takes
## it, so the backend is configured once and CheckLocomotiveParameters() runs after all of it
func _powered_up_eim(train_id: String) -> VehicleController:
    var description:VehicleController = MoverRailVehicleController.new()
    description.add_component(build_power_supply(110.0))
    description.power = 5600.0
    description.mass = 81000.0
    var wheels: RailVehicleWheels = MoverRailVehicleWheels.new()
    wheels.powered_wheel_diameter = 1.25
    wheels.axle_arrangement = "Bo'Bo'"
    description.add_component(wheels)
    var eim: RailVehicleElectricInductionEngine = MoverRailVehicleElectricInductionEngine.new()
    var power_source: RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    var master_controller: RailVehicleMasterController = MoverRailVehicleMasterController.new()
    master_controller.main_position_count = 4
    description.add_component(master_controller)
    eim.transmission_gear_teeth_motor = 48
    eim.transmission_gear_teeth_wheel = 251
    var line: MaszynaParser = MaszynaParser.new()
    line.initialize(("dfic=861 dfmax=1.84 p=2 cfu=43.7 cim=13.4 icif=0.679 Uzmax=2183 Uzh=2183 DU=20"
            + " I0=20 fcfu=43.7 F0=300 a1=0.4 Pmax=5600 Fh=150 Ph=2600 Vh0=5 Vh1=10 Imax=1950 abed=1"
            + " Flat=Yes").to_utf8_buffer())
    FizTrainElectricInductionEngineParser.new().apply_engine_fields(FizLineUtil.read_key_values(line), eim)
    power_source.current_collector_max_voltage = 3900.0
    power_source.current_collector_min_main_switch_voltage = 1900.0
    power_source.current_collector_physical_layout = 3
    power_source.current_collector_number_of_collectors = 2
    description.add_component(eim)
    description.add_component(power_source)
    var driven:VehicleController = build_vehicle(train_id, description, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    # the vehicle's own copy of the description's power source, the one the wire feeds
    power_source = driven.get_rail_component(RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE)
    driven.send_command("battery", true)
    # the crew switches its cab on - no cab is active before (CabActive = 0, MOVER.h:2090)
    driven.send_command("cab_activation", true)
    driven.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    await _wire_until_closable(driven, power_source)
    driven.send_command("main_switch", true)
    driven.send_command("direction_increase")
    for _tick:int in KEPT_CLOSED_TICKS:
        _feed_wire(power_source)
        await step(1)
    return driven


func test_powered_vehicle_without_inverter_count_does_not_turn_forces_into_nan():
    # Regression: without InvNo the Mover divides by InvertersNo (Mover.cpp:5627); the original
    # gives a powered EIM one inverter (Mover.cpp:11302), the wrapper left it at 0 and every force
    # of the vehicle became NaN as soon as a direction was set
    var driven: VehicleController = await _powered_up_eim("TestEimInverters")

    assert_true(driven.get_state()["main_switch_enabled"], "the line breaker should be closed")
    assert_false(is_nan(float(driven.get_state()["velocity"])), "velocity should not be NaN")
    assert_false(is_nan(float(driven.get_state()["tractive_force"])), "traction force should not be NaN")


func test_the_state_carries_each_inverter():
    # a powered EIM without InvNo has one inverter (Mover.cpp:11302), active and allowed
    var driven: VehicleController = await _powered_up_eim("TestEimInverterState")

    var inverters: Array = driven.get_state()["inverters"]
    assert_eq(inverters.size(), 1)
    var inverter: RailVehicleInverter = inverters[0]
    assert_true(inverter.active)
    assert_false(inverter.error)
    assert_true(inverter.allow)


func test_driven_induction_motor_pulls_once_the_controller_moves():
    # Regression: the setpoint of an integrated controller is computed by DynObj.cpp:3246-3283
    # (CheckEIMIC), which the wrapper did not call - the controller moved and Ft stayed 0
    var driven: VehicleController = await _powered_up_eim("TestEimTraction")
    var power_source: RailVehicleEnginePowerSource = driven.get_rail_component(
            RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE)
    driven.send_command("main_controller_increase")
    for i in 30:
        _feed_wire(power_source)
        await step(1)

    assert_gt(float(driven.get_state()["tractive_force"]), 0.0, "a driven induction motor should pull with the controller up")


func test_apply_power_uses_canonical_current_collector_properties():
    var line: MaszynaParser = MaszynaParser.new()
    line.initialize("EnginePower=CurrentCollector CollectorsNo=2 MaxVoltage=3000.0 MaxCurrent=800.0".to_utf8_buffer())
    var context: FizImportContext = FizImportContext.new()
    context.power_kv = FizLineUtil.read_key_values(line)
    var power_source: RailVehicleEnginePowerSource = FizTrainPowerParser.create_node(context)

    assert_eq(power_source.source_type, RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR)
    assert_eq(power_source.current_collector_number_of_collectors, 2)
    assert_eq(power_source.current_collector_max_voltage, 3000.0)
    assert_eq(power_source.current_collector_max_current, 800.0)


func test_the_engine_publishes_its_voltage_and_the_line_current():
    # EngineVoltage (Mover.cpp:4542) was published by the series motor only - the E186 screen's
    # kV bar (eimp_c1_uhv, Train.cpp:8723) stayed at 0
    var driven: VehicleController = await _powered_up_eim("TestEimVoltage")

    assert_gt(float(driven.get_state()["engine_voltage"]), 2000.0, "an EIM under 3000 V should see it on its motors")
    assert_true(driven.get_state().has("total_current"), "the line current (Itot) should be in the state")
    assert_true(driven.get_state().has("force_full"), "the full force (eimv[eimv_Fful]) should be in the state")


func test_the_screen_state_shows_the_line_voltage_of_the_powered_car():
    var driven: VehicleController = await _powered_up_eim("TestEimScreen")

    var screen: Dictionary = PythonScreenState.compose(driven.get_rid(), {})

    assert_gt(float(screen["eimp_c1_uhv"]), 2000.0, "the kV bar of traxx_renderer reads eimp_c1_uhv")
    assert_gt(float(screen["voltage"]), 2000.0)
    # Train.cpp:718 - under voltage and past its init time, the circuit is ready while the line
    # breaker is open
    assert_eq(screen["main_ready"], not driven.get_state()["main_switch_enabled"])


func test_main_init_time_reaches_the_config():
    engine.main_init_time = 2.5
    train.apply_configuration()
    await step(2)

    assert_eq(float(train.get_config()["main_init_time"]), 2.5)


## What the vehicles' step does for a vehicle standing under a live wire, for one standing on
## no track: the wire's voltage on the first pantograph and the vehicle fed with it
## The wire fed to the raised pantograph: its voltage reaches the Mover on the next step, and the
## line breaker's relays are closable with it; false when they are not (the test has failed there)
func _wire_until_closable(driven:VehicleController, power_source:RailVehicleEnginePowerSource) -> bool:
    return await wait_simulated_until(func() -> bool:
            _feed_wire(power_source)
            return driven.get_state()["main_switch_closable"], TICK, "the line breaker closable at 3000 V")


func _feed_wire(power_source:RailVehicleEnginePowerSource) -> void:
    power_source.set_pantograph_wire_voltage(RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, WIRE_VOLTAGE)
    power_source.set_collector_voltage(WIRE_VOLTAGE)
