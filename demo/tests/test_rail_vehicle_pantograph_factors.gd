extends MaszynaGutTest

## A pantograph of a slider only is animated, as the original's (DynObj.cpp:5562-5566), and stands
## where the MMD's pantfactors: puts it: its place along the vehicle and its slider height, and on top
## of the vehicle's box with the arms of the type - here none in the FIZ, so AKP_4E's
## (DynObj.cpp:90-100, 5577-5633).

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const DATA_PATH:String = "dynamic/test/animated_v1"
const TOLERANCE:float = 0.001
## pantfactors: -4.0 4.0 0.6 0.08, corrected (DynObj.cpp:5580-5596): the places turned, the first
## height the second's
const FIRST_PLACE:float = 4.0
const SLIDER_HEIGHT:float = 0.08
## Dimensions: H= of pantslider.fiz [m]
const VEHICLE_HEIGHT:float = 4.3
## AKP_4E (DynObj.cpp:90-100, 71-78)
const LOWER_LENGTH:float = 1.22
const UPPER_LENGTH:float = 1.755
const HORIZONTAL:float = 0.535
const LOWER_REST_ANGLE_DEGREES:float = 2.8547285515689267247882521833308

var _previous_game_dir:String
var vehicle:RailVehicle3D


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    if is_instance_valid(vehicle):
        vehicle.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_the_factors_are_read_with_the_originals_corrections() -> void:
    var appearance:RailVehicleAppearance = MaszynaRailVehicle3DInstancer.read_structure(
            DATA_PATH, "pantslider", "", "pantslider").appearance
    assert_eq(appearance.pantograph_factors, PackedFloat64Array([FIRST_PLACE, -FIRST_PLACE, SLIDER_HEIGHT, SLIDER_HEIGHT]))
    assert_eq(appearance.pantograph_front_arms, PackedStringArray(["", "", "", "", "slizg_pant01"]),
            "a slider alone is a pantograph")


func test_a_slider_only_pantograph_stands_where_the_factors_put_it() -> void:
    vehicle = await spawn_maszyna_vehicle(DATA_PATH, "pantslider", "", "test_pantslider")
    if not await wait_detailed(vehicle):
        return
    _assert_stands_where_the_factors_put_it()


## The pantograph is the vehicle's, read off its model file: a vehicle not drawn - none is, while a
## scenery loads and the streaming has no camera - raises it all the same
func test_a_pantograph_of_a_vehicle_not_drawn_stands_where_the_factors_put_it() -> void:
    assert_false(SceneryStreamingServer.streaming_has_camera(), "no camera to draw the vehicle near")
    var not_drawn:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    not_drawn.data_path = DATA_PATH
    not_drawn.file_name = "pantslider"
    not_drawn.vehicle_id = "test_pantslider_not_drawn"
    vehicle = not_drawn
    add_child(not_drawn)
    await not_drawn.vehicle_built
    await wait_idle_frames(1)

    assert_false(RailVehicleRenderingServer.vehicle_get_model(vehicle.get_rid()).is_valid(), "the vehicle is not drawn")
    _assert_stands_where_the_factors_put_it()


func _assert_stands_where_the_factors_put_it() -> void:
    var position:Vector3 = RailVehicleServer.vehicle_get_pantograph_position(
            vehicle.get_rid(), RailVehicleEnginePowerSource.PANTOGRAPH_FIRST)
    var lower_rest:float = deg_to_rad(LOWER_REST_ANGLE_DEGREES)
    var upper_rest:float = acos((LOWER_LENGTH * cos(lower_rest) + HORIZONTAL) / UPPER_LENGTH)
    var raised:float = LOWER_LENGTH * sin(lower_rest) + UPPER_LENGTH * sin(upper_rest) + SLIDER_HEIGHT
    var along:float = (MaszynaRailVehicle3DInstancer.MASZYNA_VEHICLE_FRAME.basis * Vector3(0.0, 0.0, FIRST_PLACE)).z
    assert_almost_eq(position.z, along, TOLERANCE, "along the vehicle, as the model's frame turns it")
    assert_almost_eq(position.y, VEHICLE_HEIGHT - SLIDER_HEIGHT - raised, TOLERANCE, "on top of the vehicle's box")
