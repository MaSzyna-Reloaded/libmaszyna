extends MaszynaGutTest

## The player hands the running EP07-424 of td.scn to its AI driver (Shift+Q) and takes it back (Q):
## the vehicle runs on as it was - the line breaker and the converter stay on (report 2026-10-05:
## both dropped after the take-back). Only the roles in the cabin change (PlayerServer).
##
## The player is in the scene before the scenery and takes the selected vehicle on
## `scenery_loaded`, after the scenery has assigned its AI drivers. Handing the vehicle over uses
## that driver instead of creating AI for the player (the cause of the report).

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"
## Simulated time the AI drives before the player takes the vehicle back [s]
const AI_DRIVING_SECONDS:float = 3.0
## Simulated time the locomotive is watched after the take-back - the line breaker and the
## converter must stay on through it [s]
const TAKE_BACK_WATCH_SECONDS:float = 2.0

var _previous_game_dir:String = ""
var scenery:MaszynaSceneryNode
var player:MaszynaPlayer
var vehicle_rid:RID


func before_each():
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    player = load("res://addons/libmaszyna/player/player.tscn").instantiate()
    player.start_vehicle_id = "EP07-424"
    add_child(player)
    scenery = MaszynaSceneryNode.new()
    scenery.filename = SCENERY
    scenery.scenery_loaded.connect(player._on_scenery_loaded)
    add_child(scenery)
    # the scenery is announced once its vehicles are built and its drivers given their AI
    if not await wait_loaded(scenery.scenery_loaded, SCENERY):
        return
    vehicle_rid = VehicleServer.vehicle_get_rid_by_name("EP07-424")


func after_each():
    player.free()
    scenery.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func _main_switch_enabled() -> bool:
    return bool(VehicleServer.vehicle_dump_state(vehicle_rid).get("main_switch_enabled", false))


func _power_supply() -> RailVehiclePowerSupply:
    return RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply


func _converter_enabled() -> bool:
    return _power_supply().get_converter_enabled()


## The locomotive running: low voltage, pantograph at the wire, the line breaker and the converter
## on; false when a step did not come about (the test has failed there)
func _power_up() -> bool:
    var power_supply:RailVehiclePowerSupply = _power_supply()
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            vehicle_rid, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    VehicleServer.vehicle_send_command(vehicle_rid, "battery", true)
    # the battery's low voltage is there from the next step on
    if not await wait_simulated_until(power_supply.get_power24_available, TICK, "the battery's low voltage"):
        return false
    VehicleServer.vehicle_send_command(vehicle_rid, "security_acknowledge", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "security_acknowledge", false)
    VehicleServer.vehicle_send_command(vehicle_rid, "pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    # the fixture's EP07 has no model, so no arms to raise: its pantograph is at the wire the step it
    # goes up (RailVehicleServer::vehicle_collect_current()), its tank spawned above MinPress
    # (303e-ep.fiz)
    if not await wait_simulated_until(
            func() -> bool: return VehicleServer.vehicle_dump_state(vehicle_rid).get(
                    "current_collector/pantograph_first_voltage", 0.0) > 100.0,
            TICK, "the wire's voltage at the first pantograph"):
        return false
    # the ground relay only resets with a direction set (Mover.cpp:6038-6051)
    VehicleServer.vehicle_send_command(vehicle_rid, "direction_increase")
    VehicleServer.vehicle_send_command(vehicle_rid, "converter_fuse_reset")
    VehicleServer.vehicle_send_command(vehicle_rid, "fuse_reset")
    # the line breaker closes only CtrlDelay (SCDelay) after its last switching, counted from the
    # vehicle's spawn (Mover.cpp:3362)
    if not await wait_simulated_until(engine.get_main_switch_closable, master.step_delay + TICK,
            "a closable line breaker"):
        return false
    VehicleServer.vehicle_send_command(vehicle_rid, "main_switch", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "converter", true)
    # the converter starts ConverterStartDelay after it is switched on (Mover.cpp:2055-2067)
    return await wait_simulated_until(power_supply.get_converter_enabled,
            power_supply.cntrl_converter_start_delay + TICK, "the converter")


func test_handed_to_the_ai_and_taken_back_the_locomotive_runs_on() -> void:
    assert_true(VehicleServer.vehicle_is_simulation_ready(vehicle_rid), "EP07-424 should exist")
    assert_eq(PlayerServer.player_get_vehicle(), vehicle_rid, "the player drives EP07-424")
    var ai_driver:RID = DriverServer.vehicle_get_driver(vehicle_rid)
    assert_true(ai_driver.is_valid(), "the scenery's driver rides along")
    assert_ne(ai_driver, PlayerServer.player_get_person(), "and it is not the player")
    assert_false(DriverServer.driver_get_rids().has(PlayerServer.player_get_person()), "the player thinks for itself")
    assert_false(DriverServer.vehicle_is_control_active(vehicle_rid), "the AI touches nothing while the player drives")
    if not await _power_up():
        return
    assert_true(_main_switch_enabled(), "the line breaker is on")
    assert_true(_converter_enabled(), "the converter runs")

    PlayerServer.player_hand_over_vehicle()
    assert_true(DriverServer.vehicle_is_control_active(vehicle_rid), "the AI drives")
    await step(ticks(AI_DRIVING_SECONDS))
    assert_true(_main_switch_enabled(), "the line breaker stays on while the AI drives")
    assert_true(_converter_enabled(), "and so does the converter")

    PlayerServer.player_take_over_vehicle(vehicle_rid)
    assert_eq(VehicleServer.person_get_role(PlayerServer.player_get_person()),
            VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER, "the player drives again")
    assert_false(DriverServer.vehicle_is_control_active(vehicle_rid), "and the AI rides along")
    await step(ticks(TAKE_BACK_WATCH_SECONDS))
    assert_true(_main_switch_enabled(), "the line breaker stays on after the take-back")
    assert_true(_converter_enabled(), "and so does the converter")
