extends MaszynaGutTest

## End-to-end smoke test for the pantograph power layer, using the actual td.scn scenery and its
## real EP07-424 vehicle (node "EP07-424" in td.scn, dynamic/pkp/303e_v1) - not a synthetic
## VehicleController like test_traction_power_pantograph.gd. Named test_zzz_* (like the existing
## test_zzz_scenery_scene_smoke.gd/test_zzz_trainset_diagnostic.gd) so it runs last: it's slow
## (loads the whole scenery) and exists specifically to catch breaks in the RailVehicle3D
## geometry/wire-lookup path that a synthetic-controller test can't reach.

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"
## A wire's voltage, not the noise of none [V]
const MIN_WIRE_VOLTAGE:float = 100.0

var _previous_game_dir:String = ""
var scenery:MaszynaSceneryNode


func before_each():
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    scenery = MaszynaSceneryNode.new()
    scenery.filename = SCENERY
    add_child(scenery)
    # announced once its vehicles are built
    await wait_loaded(scenery.scenery_loaded, SCENERY)


func after_each():
    scenery.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_ep07_pantograph_draws_wire_voltage_from_td_scn() -> void:
    var vehicle:RID = VehicleServer.vehicle_get_rid_by_name("EP07-424")
    assert_true(VehicleServer.vehicle_is_simulation_ready(vehicle), "EP07-424 should be simulated once the scenery is loaded")
    if not VehicleServer.vehicle_is_simulation_ready(vehicle):
        return
    var controller:VehicleController = VehicleServer.vehicle_get_controller(vehicle)

    # Same startup sequence a real driver uses on a cold EP07: battery, then the (main) compressor
    # to actually build air pressure - PantPress mirrors ScndPipePress (Mover.cpp's
    # UpdatePantVolume, bPantKurek3 branch), so without this the reservoir never fills and the
    # pantograph would never see enough pressure to move at all, regardless of "pantograph" being
    # sent. No separate master pantograph-valve command - this vehicle's cabin has no such switch
    # (confirmed against its .mmd) and nothing in this wrapper sends one via keybind either. If this
    # ever needs a fourth command again, that's a real regression, not a missing test setup step -
    # see RailVehicleEnginePowerSource::pantograph()'s own comment for why it's otherwise
    # self-contained.
    controller.send_command("battery", true)
    await step(1)
    controller.send_command("compressor", true)
    await step(1)
    controller.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)

    # the fixture's EP07 has no pantograph arms in its model: the raised pantograph samples the wire
    # where it stands, on the step after the command (RailVehicleServer, "a model without the arms")
    await wait_simulated_until(func() -> bool:
            return controller.get_state().get("current_collector/pantograph_first_active", false) \
                    and controller.get_state().get("current_collector/pantograph_first_voltage", 0.0) > MIN_WIRE_VOLTAGE,
            TICK, "the pantograph at the wire")
    var active:bool = controller.get_state().get("current_collector/pantograph_first_active", false)
    var voltage:float = controller.get_state().get("current_collector/pantograph_first_voltage", 0.0)

    assert_true(active, "pantograph should report raised once battery+valves are on")
    assert_true(
            voltage > MIN_WIRE_VOLTAGE,
            "pantograph should read a real wire voltage once raised and placed over td.scn's electrified track, got %s" % voltage)
