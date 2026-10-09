extends MaszynaGutTest

var train: VehicleController
var engine: RailVehicleDieselElectricEngine

func before_each():
    train = build_vehicle("TestTrain")

    engine = MoverRailVehicleDieselElectricEngine.new()
    train.add_component(engine)
    await wait_idle_frames(2)

func _make_row(rpm: float, gen_power: float) -> RailVehicleWWListItem:
    var item = RailVehicleWWListItem.new()
    item.rpm = rpm
    item.max_power = gen_power
    return item

func test_defaults():
    engine.apply_config()
    assert_false(engine.generator_voltage_flat)
    assert_eq(engine.hyperbolic_speed, 1.0)
    assert_eq(engine.additional_speed, 1.0)
    assert_eq(engine.power_correction_ratio, 1.0)
    assert_eq(engine.shunt_relay_type, 0)
    assert_false(engine.shunt_mode_allowed)
    assert_eq(engine.heating_rpm, 0.0)
    assert_eq(engine.wwlist.size(), 0)

func test_round_trip_and_wwlist_update():
    engine.generator_voltage_flat = true
    engine.hyperbolic_speed = 1.1
    engine.additional_speed = 1.2
    engine.power_correction_ratio = 0.95
    engine.shunt_relay_type = 1
    engine.shunt_mode_allowed = true
    engine.heating_rpm = 700.0
    engine.wwlist = [_make_row(696, 0), _make_row(629, 96)]
    await wait_idle_frames(2)

    assert_true(engine.generator_voltage_flat)
    assert_eq(engine.wwlist.size(), 2)
    assert_true(train.get_state().has("main_switch_enabled"), "RailVehicleDieselElectricEngine should keep functioning after configuring its Engine: fields and wwlist")

func test_inherited_mechanical_fields_stay_at_defaults_when_unused():
    # RailVehicleDieselElectricEngine inherits RailVehicleDieselEngine's mechanical-transmission
    # properties, but a diesel-electric vehicle should simply leave them at their defaults.
    await wait_idle_frames(2)
    assert_false(engine.torque_converter_present)
    assert_false(engine.retarder_present)

func test_fiz_wwlist_row_uses_canonical_shunting_property():
    var context: FizImportContext = FizImportContext.new()
    context.add_part("RailVehicleEngine", engine)
    var parser: FizTrainDieselElectricEngineParser = FizTrainDieselElectricEngineParser.new()
    var header: MaszynaParser = MaszynaParser.new()
    header.initialize(PackedByteArray())
    parser.parse(header, context, "WWList:")
    var row: MaszynaParser = MaszynaParser.new()
    row.initialize("696 100 3000 800 100 200 50".to_utf8_buffer())
    parser.parse_row(row, context)
    parser.end_table(context)

    assert_eq(engine.wwlist.size(), 1)
    assert_true((engine.wwlist[0] as RailVehicleWWListItem).has_shunting)

## BR285's Engine: gives Vadd and Cr twice; the original reads the first (extract_value's find(),
## utilities.h:170). The last one, Vadd=0, made the traction force 0/0 on a standing vehicle.
const BR285_ENGINE: String = "EngineType=DieselElectric Trans=18:64 Ftmax=300000 Vhyp=24 Vadd=5.5 Cr=1 Vadd=0.0 Cr=1.0 WaterMinTemperature=40 WaterFlowTemperature=70 WaterCoolingTemperature=82 WaterMaxTemperature=91 WaterShutters=Yes WaterAuxCircuit=Yes WaterAuxCoolingTemperature=50 WaterAuxMaxTemperature=74 WaterAuxShutters=Yes OilMinPressure=0.15 HeatKFO2=33"

func _apply_engine_line(line: String) -> void:
    var p: MaszynaParser = MaszynaParser.new()
    p.initialize(line.to_utf8_buffer())
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    FizTrainDieselElectricEngineParser.new().apply_engine_fields(kv, engine)
    FizTrainDieselEngineParser.apply_diesel_common(kv, engine)

func test_fiz_repeated_key_keeps_its_first_value():
    _apply_engine_line(BR285_ENGINE)
    assert_almost_eq(engine.additional_speed, 5.5 / 3.6, 0.0001)

func test_fiz_diesel_electric_inertia_and_rpm_decrease_rate():
    _apply_engine_line("EngineType=DieselElectric")
    assert_eq(engine.mechanical_inertia, 1.25, "the diesel-electric's own AIM default (Mover.cpp:11282)")
    _apply_engine_line("EngineType=DieselElectric AIM=2 RPMDecRate=3")
    assert_eq(engine.mechanical_inertia, 2.0)
    assert_eq(engine.mechanical_rpm_decrease_rate, 3.0)

func test_fiz_diesel_cooling_keys():
    _apply_engine_line(BR285_ENGINE)
    assert_eq(engine.cooling_water_min_temperature, 40.0)
    assert_eq(engine.cooling_water_flow_temperature, 70.0)
    assert_eq(engine.cooling_water_cooling_temperature, 82.0)
    assert_eq(engine.cooling_water_max_temperature, 91.0)
    assert_true(engine.cooling_water_shutters)
    assert_true(engine.cooling_water_aux_circuit)
    assert_eq(engine.cooling_water_aux_cooling_temperature, 50.0)
    assert_eq(engine.cooling_water_aux_max_temperature, 74.0)
    assert_eq(engine.cooling_water_aux_min_temperature, -1.0, "a key the line lacks keeps the default")
    assert_true(engine.cooling_water_aux_shutters)
    assert_eq(engine.cooling_heat_kfo2, 33.0)
    assert_eq(engine.cooling_heat_kw, 0.35)
    assert_eq(engine.cooling_nominal_power, 1235.0)
