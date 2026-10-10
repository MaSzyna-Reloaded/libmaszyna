extends MaszynaGutTest

## Regression test for the reported "na postoju pojazdy sa odwrocone domyslnie, a jak ruszysz to
## sie odwraca / w koncu gasnie i nie drgnie" bug - drives the REAL EP07-424 from td.scn
## (dynamic/pkp/303e_v1), asserting its actual global orientation stays stable while parked and
## while being driven.
##
## Root cause (confirmed with this test before the fix, still printed as a running dump for
## future debugging): RailVehicle3D::_update_track_transform()'s bogie-refined orientation
## branch sampled "front" and "rear" track-offset distances swapped (RailVehicleServer's
## offset-distance sign convention is rear-relative, not what the naive +0.5/-0.5 sampling
## assumed), so body_forward pointed opposite the vehicle's real forward direction. That branch
## only runs once bogies are resolved *and* the vehicle has visibly moved - while parked the
## correct coarse transform is all that's applied, so the vehicle looked fine until it moved,
## at which point the very first recompute flipped it 180 degrees. Fixed in RailVehicle3D.cpp by
## swapping which distance sample is "front" vs "rear". See
## test_rail_vehicle_idle_orientation_regression.gd for the isolated, asset-free reproduction of
## the same bug (that one is faster to run and easier to read; keep this real-scenery test too,
## since a synthetic fixture can miss real-world specifics like async E3D bogie resolution).

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"

## How long the EP07 drives on its first notch [s of simulated time] - the 20 x 0.5 s the test was
## written with - and how often it is dumped [steps]
const DRIVE_SECONDS:float = 10.0
const DUMP_EVERY_STEPS:int = 10

var _previous_game_dir:String = ""
var scenery:MaszynaSceneryNode


func before_each():
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    scenery = MaszynaSceneryNode.new()
    scenery.filename = SCENERY
    add_child(scenery)
    # the scenery is announced once its vehicles are built and its drivers given their AI
    if not await wait_loaded(scenery.scenery_loaded, SCENERY):
        return


func after_each():
    # out of the cab before the vehicle goes
    PlayerServer.player_leave_vehicle()
    scenery.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


## The way the vehicle is drawn facing (RailVehicleRenderingServer)
func _forward(vehicle:RID) -> Vector3:
    return -RailVehicleRenderingServer.vehicle_get_transform(vehicle).basis.z.normalized()


func _dump_orientation(label:String, vehicle:RID, controller:VehicleController) -> void:
    print(
        "[%s] forward=%s pos=%s velocity=%s main_switch_enabled=%s" % [
            label,
            _forward(vehicle),
            RailVehicleRenderingServer.vehicle_get_transform(vehicle).origin,
            controller.get_state().get("velocity", null),
            controller.get_state().get("main_switch_enabled", null),
        ]
    )


func test_ep07_orientation_stays_stable_while_parked_and_while_driving() -> void:
    var rail_vehicle:RID = VehicleServer.vehicle_get_rid_by_name("EP07-424")
    assert_true(VehicleServer.vehicle_is_simulation_ready(rail_vehicle), "EP07-424 should be a vehicle of the loaded scenery")
    if not VehicleServer.vehicle_is_simulation_ready(rail_vehicle):
        return
    var controller:VehicleController = VehicleServer.vehicle_get_controller(rail_vehicle)

    # Sit parked for a while right after spawn, exactly as a player would see before boarding.
    for i in range(10):
        await wait_idle_frames(1)
        _dump_orientation("parked frame %d" % i, rail_vehicle, controller)
    var forward_while_parked:Vector3 = _forward(rail_vehicle)
    # td.scn's trainset velocity is 0.0 - the original spawns such a vehicle cold
    # (Mover.cpp:8943 only turns Battery on for a vehicle ready to depart).
    assert_false(
        controller.get_state().get("battery_enabled", true),
        "EP07-424 on td.scn should spawn with its battery off, like in the original",
    )

    # Battery on arms the cab signal (Mover.cpp:131), so acknowledge only once it is powered.
    controller.send_command("battery", true)
    # the player takes the vehicle over from its driver, as entering the cab does
    # (RailVehicle3D.cpp:185-188), and switches the cab on - none is active before (MOVER.h:2090)
    PlayerServer.player_take_over_vehicle(controller.get_rid())
    controller.send_command("cab_activation", true)
    var power_supply:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            rail_vehicle, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    # the battery's low voltage is there from the next step on
    if not await wait_simulated_until(power_supply.get_power24_available, TICK, "the battery's low voltage"):
        return
    controller.send_command("security_acknowledge", true)
    controller.send_command("security_acknowledge", false)
    controller.send_command("brake_level_set", 0.25)
    # its driver may have held it by the independent brake while it stood parked, before the
    # player took it over (Driver.cpp:8166-8180) - released, as the player does
    controller.send_command("local_brake_set", 0.0)
    controller.send_command("brake_releaser", true)
    controller.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    # the fixture's EP07 has no model, so no arms to raise: its pantograph is at the wire the step it
    # goes up (RailVehicleServer::vehicle_collect_current()), its tank spawned above MinPress
    # (303e-ep.fiz)
    if not await wait_simulated_until(
            func() -> bool: return controller.get_state().get("current_collector/pantograph_first_voltage", 0.0) > 100.0,
            TICK, "the wire's voltage at the first pantograph"):
        return
    controller.send_command("direction_increase")
    controller.send_command("converter_fuse_reset")
    controller.send_command("fuse_reset")
    # the line breaker closes only CtrlDelay (SCDelay) after its last switching, counted from the
    # vehicle's spawn (Mover.cpp:3362)
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            rail_vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            rail_vehicle, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    if not await wait_simulated_until(engine.get_main_switch_closable, master.step_delay + TICK,
            "a closable line breaker"):
        return
    controller.send_command("main_switch", true)
    controller.send_command("converter", true)
    # the converter starts ConverterStartDelay after it is switched on (Mover.cpp:2055-2067)
    if not await wait_simulated_until(power_supply.get_converter_enabled,
            power_supply.cntrl_converter_start_delay + TICK, "the converter"):
        return
    controller.send_command("compressor", true)
    _dump_orientation("just before first notch", rail_vehicle, controller)

    controller.send_command("main_controller_increase")
    # simulated seconds, not frames: a headless run draws frames as fast as it can; every frame of
    # the drive is looked at, and the drive ends early on a dropped main switch or a flip
    var tripped:bool = false
    var forward:Vector3 = forward_while_parked
    for drive_step:int in ticks(DRIVE_SECONDS):
        await step(1)
        if drive_step % DUMP_EVERY_STEPS == 0:
            _dump_orientation("driving step %d" % drive_step, rail_vehicle, controller)
        if not controller.get_state().get("main_switch_enabled", false):
            _dump_orientation("MAIN SWITCH DROPPED at step %d" % drive_step, rail_vehicle, controller)
            tripped = true
            break
        forward = _forward(rail_vehicle)
        if forward_while_parked.distance_to(forward) > 0.1:
            _dump_orientation("ORIENTATION FLIPPED at step %d" % drive_step, rail_vehicle, controller)
            break
    if forward_while_parked.distance_to(forward) > 0.1:
        assert_true(
            false,
            "vehicle orientation flipped while driving: parked=%s now=%s" % [forward_while_parked, forward],
        )
        return

    _dump_orientation("final", rail_vehicle, controller)
    assert_false(tripped, "main switch should not self-trip while accelerating away from a stop")
    assert_true(
        float(controller.get_state().get("velocity", 0.0)) > 0.0,
        "vehicle should have actually started moving",
    )
