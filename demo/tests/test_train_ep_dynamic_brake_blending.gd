extends MaszynaGutTest

var train: VehicleController
var ep_brake: RailVehicleElectroPneumaticDynamicBrake

func before_each():
    train = build_vehicle("TestTrain")

    ep_brake = MoverRailVehicleElectroPneumaticDynamicBrake.new()
    train.add_component(ep_brake)
    await wait_idle_frames(2)

func test_blending_defaults():
    assert_eq(ep_brake.blending_max_velocity, 0.0)
    assert_eq(ep_brake.blending_min_velocity, 0.0)
    assert_eq(ep_brake.blending_reference_velocity, 0.0)
    assert_eq(ep_brake.blending_max_deceleration, 9.81)
    assert_false(ep_brake.blending_velocity_correction)
    assert_false(ep_brake.blending_load_correction)
    assert_eq(ep_brake.blending_min_ed_brake_request, 0.0)

func test_blending_round_trip_and_update():
    ep_brake.blending_max_velocity = 120.0
    ep_brake.blending_min_velocity = 5.0
    ep_brake.blending_reference_velocity = 80.0
    ep_brake.blending_max_deceleration = 1.2
    ep_brake.blending_velocity_correction = true
    ep_brake.blending_load_correction = true
    ep_brake.blending_min_ed_brake_request = 0.1
    await wait_idle_frames(2)

    assert_eq(ep_brake.blending_max_velocity, 120.0)
    assert_eq(ep_brake.blending_min_velocity, 5.0)
    assert_eq(ep_brake.blending_reference_velocity, 80.0)
    assert_eq(ep_brake.blending_max_deceleration, 1.2)
    assert_true(ep_brake.blending_velocity_correction)
    assert_true(ep_brake.blending_load_correction)
    assert_eq(ep_brake.blending_min_ed_brake_request, 0.1)
    assert_true(train.get_state().has("dcemued/ep_fuse"), "RailVehicleElectroPneumaticDynamicBrake should keep functioning after configuring blending")

func test_fiz_dcemued_uses_canonical_electro_pneumatic_property():
    var context: FizImportContext = FizImportContext.new()
    context.add_part("RailVehicleElectroPneumaticDynamicBrake", ep_brake)
    var parser: FizTrainElectroPneumaticDynamicBrakeParser = FizTrainElectroPneumaticDynamicBrakeParser.new()
    var line: MaszynaParser = MaszynaParser.new()
    line.initialize("EP_min_Im=0.25".to_utf8_buffer())
    parser.parse(line, context, "DCEMUED:")

    assert_eq(ep_brake.electro_pneumatic_min_regenerative_braking, 0.25)
