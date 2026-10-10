extends MaszynaGutTest

## Regression: SU46's head/end lights hang under an unnamed transform, which E3DParser renames to
## "banan". Light paths used to be built from the raw (empty) E3D name, so every light definition
## got an empty on/off path, the nodes instancer never matched them ("LightInfo not found for light")
## and no lamp could be switched on. The fabricated vehicle (demo/tests/fixtures/dynamic/test/
## synthetic_v1) hangs its lights under a transform too - a text model cannot leave one unnamed, as
## a child names its parent - so this checks the path of a nested light end to end.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

var _previous_game_dir:String
var vehicle:RailVehicle3D


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    if is_instance_valid(vehicle):
        vehicle.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_exterior_lights_resolve_and_switch() -> void:
    vehicle = await spawn_maszyna_vehicle("dynamic/test/synthetic_v1", "synthetic", "", "test_vehicle_lights")
    if not await wait_detailed(vehicle):
        return
    var model:RID = RailVehicleRenderingServer.vehicle_get_model(vehicle.get_rid())
    assert_true(model.is_valid(), "the exterior model should load")
    if not model.is_valid():
        return

    var lights:Dictionary = E3DRenderingServer.instance_get_model(model).lights
    assert_true(lights.has("headlamp11"), "headlamp11 should be a light of the model")
    assert_true(lights.has("endsignal12"), "endsignal12 should be a light of the model")

    E3DRenderingServer.instance_set_lights_state(model, {"headlamp11": true})
    var on_node:Node3D = vehicle.find_child("headlamp11_on", true, false) as Node3D
    assert_not_null(on_node, "the exterior should contain headlamp11_on")
    if on_node:
        assert_true(on_node.visible, "headlamp11_on should be shown when the lamp is on")
