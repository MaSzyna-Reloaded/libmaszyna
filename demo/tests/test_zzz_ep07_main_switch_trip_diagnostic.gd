extends MaszynaGutTest

## Regression test for the reported "wylacznik szybki wybija po paru sekundach na drugiej pozycji
## nastawnika" (main switch trips a couple seconds after the second controller notch, can't
## accelerate) bug - drives the real EP07-424 from td.scn (dynamic/pkp/303e_v1) through the
## operator's own in-game command sequence and asserts Mains stays closed and DamageFlag stays
## clear while advancing the controller.
##
## Root cause (confirmed via this test's own diagnostic dumps before the fix): FizTrainElectric
## SeriesEngineParser.apply_engine_fields() divided "nmax" by 60 in GDScript AND
## RailVehicleElectricSeriesEngine::_do_update_internal_mover divided by 60 again in C++ - nmax ended up
## 3600x too small. Mover's own motor-overspeed damage check (Mover.cpp:446, FuzzyLogic(abs(enrot),
## nmax*1.11, p_elengproblem)) then had a chance to fire at a tiny fraction of a km/h instead of
## near the real max speed, latching DamageFlag's dtrain_engine bit almost immediately once any
## current flowed - which then keeps re-tripping Mains (Mover.cpp:440-443) for as long as the
## motor draws current, matching the reported symptom exactly. Fixed by not pre-dividing in
## GDScript (fiz_train_electric_series_engine_parser.gd) since the C++ layer already does the
## conversion, mirroring the original engine's own single LoadFIZ_Engine "nmax /= 60.0"
## (Mover.cpp:10884).
##
## Modeled on test_zzz_ep07_pantograph_power_smoke.gd's real-scenery pattern.

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"
## Simulated seconds each controller notch is driven and watched for a trip - the 180 frames at
## 60 fps the test was written with
const NOTCH_SECONDS:float = 3.0

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


func _find_train_controller(vehicle_name:String) -> VehicleController:
    var vehicle:RID = VehicleServer.vehicle_get_rid_by_name(vehicle_name)
    return VehicleServer.vehicle_get_controller(vehicle) if VehicleServer.vehicle_is_simulation_ready(vehicle) else null


func test_ep07_main_switch_stays_closed_while_advancing_controller() -> void:
    var controller:VehicleController = _find_train_controller("EP07-424")
    assert_not_null(controller, "EP07-424 should exist somewhere under the loaded scenery")
    if not controller:
        return

    # the scenery gives EP07-424 its driver (headdriver); the test drives it as a player does, who
    # takes the controls from the driver (MaszynaPlayer, drivermode.cpp:266)
    PlayerServer.player_take_over_vehicle(controller.get_rid())
    var power_supply:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            controller.get_rid(), RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    assert_not_null(power_supply, "the EP07's FIZ (Light: LMaxVoltage) gives it a power supply")
    if not power_supply:
        return
    # Battery on arms the cab signal (Mover.cpp:131), so acknowledge only once it is powered - the
    # battery's low voltage is there from the next step on.
    controller.send_command("battery", true)
    if not await wait_simulated_until(power_supply.get_power24_available, TICK, "the battery's low voltage"):
        return
    # the controllers answer only from an active cab (IncMainCtrl, Mover.cpp:2226)
    controller.send_command("cab_activation", true)
    controller.send_command("security_acknowledge", true)
    controller.send_command("security_acknowledge", false)
    controller.send_command("brake_level_set", 0.25)
    controller.send_command("brake_releaser", true)
    controller.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    # main_switch must not be sent before EnginePowerSourceVoltage() has anything to report, since
    # MainSwitchCheck's powerisavailable check is evaluated once at the moment of the command and
    # silently refuses the close otherwise (it isn't retried later just because voltage shows up
    # afterward). The fixture's EP07 has no model, so no arms to raise: its pantograph is at the
    # wire the step it goes up (RailVehicleServer::vehicle_collect_current()), its tank spawned
    # above MinPress (303e-ep.fiz).
    if not await wait_simulated_until(
            func() -> bool: return controller.get_state().get("current_collector/pantograph_first_voltage", 0.0) > 100.0,
            TICK, "the wire's voltage at the first pantograph"):
        return
    assert_true(power_supply.get_power24_available(), "the battery gives the low voltage")
    controller.send_command("direction_increase")
    controller.send_command("converter_fuse_reset")
    controller.send_command("fuse_reset")
    # the line breaker closes only CtrlDelay (SCDelay) after its last switching, counted from the
    # vehicle's spawn (Mover.cpp:3362)
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            controller.get_rid(), VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            controller.get_rid(), RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
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

    assert_true(
            controller.get_state().get("main_switch_enabled", false),
            "main switch should be closed before advancing the controller")
    assert_true(power_supply.get_converter_enabled(), "the converter runs")
    assert_true(power_supply.get_power110_available(), "and gives the 110 V circuits")

    var tripped:bool = false
    for notch in range(1, 6):
        controller.send_command("main_controller_increase")
        # Watched every single frame (not just every 0.5s) so the exact frame Mains flips is
        # caught, instead of a coarser 0.5s snapshot that could miss a one-frame relay blip that
        # already self-recovered by the next sample.
        for _tick:int in ticks(NOTCH_SECONDS):
            await step(1)
            if not controller.get_state().get("main_switch_enabled", false):
                tripped = true
                break
        if tripped:
            break

    assert_false(tripped, "main switch should not self-trip while advancing the controller")
    assert_eq(
            controller.get_state().get("train_damage", 0), 0,
            "no engine damage should latch from normal acceleration")
    assert_true(
            controller.get_state().get("velocity", 0.0) > 2.0,
            "vehicle should have accelerated past 2 m/s across 5 controller notches")
    # Hasler (Train.cpp:6917-6940): wheel-based speed, jumpy needle and the tachoclock gate are
    # all live once the loco has been moving for more than a second.
    assert_gt(float(controller.get_state().get("tachometer_speed", 0.0)), 1.0, "Hasler should see the speed")
    assert_gt(float(controller.get_state().get("tachometer_speed_jump", 0.0)), 0.0, "Hasler needle should move")
    assert_gt(float(controller.get_state().get("tachometer_clock_speed", 0.0)), 1.0, "Hasler should be ticking")
