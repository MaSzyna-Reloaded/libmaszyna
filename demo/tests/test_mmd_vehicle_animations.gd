extends MaszynaGutTest

## The submodels of a vehicle model that move are named by its MMD - `animwheelprefix:`,
## `animpant*prefix:`, `animwiperprefix:` - numbered from 1 as many times as its `animations:` line
## declares (TDynamicObject::LoadMMediaFile, DynObj.cpp:5221-5873). The fixture's prefixes are not
## the commonest ones, as the 36WE's and the EN57's are not.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const DATA_PATH:String = "dynamic/test/animated_v1"
const MMD_DIR:String = "res://tests/fixtures/dynamic/test/animated_v1"
## The pantograph elements: lower arm 1, lower arm 2, upper arm 1, upper arm 2, slider
const FRONT_PANTOGRAPH:PackedStringArray = ["pantrd1_pant01", "", "pantrg1_pant01", "", "slizg_pant01"]

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func _mmd(file_name:String) -> String:
    return ProjectSettings.globalize_path(MMD_DIR.path_join(file_name + ".mmd"))


func _parameters(file_name:String) -> Dictionary:
    return MmdCabinInstancer.vehicle_parameters("animated", file_name, "")


func _appearance(file_name:String) -> RailVehicleAppearance:
    return MaszynaRailVehicle3DInstancer.read_structure(DATA_PATH, file_name, "", "animated").appearance


func test_pantograph_elements_are_named_by_the_prefixes_the_mmd_declares() -> void:
    var elements:Array[PackedStringArray] = MmdCabinInstancer.parse_pantograph_element_names(
            _mmd("animated"), _parameters("animated"))
    var expected:Array[PackedStringArray] = [
        PackedStringArray(["pantrd1_pant01"]), PackedStringArray(), PackedStringArray(["pantrg1_pant01"]),
        PackedStringArray(), PackedStringArray(["slizg_pant01"])]
    assert_eq(elements, expected, "one pantograph declared, its rd2/rg2 prefixes absent")


func test_the_model_moves_the_submodels_the_mmd_names() -> void:
    var appearance:RailVehicleAppearance = _appearance("animated")
    assert_eq(appearance.pantograph_front_arms, FRONT_PANTOGRAPH, "the front pantograph by its own prefixes")
    assert_eq(appearance.pantograph_rear_arms, PackedStringArray(),
            "the model's second pantograph is not one the MMD declares")
    assert_eq(appearance.powered_wheels, PackedStringArray(["wheel1", "wheel2"]), "the wheels by animwheelprefix:")
    assert_eq(appearance.wiper_arms.size(), 12, "four wipers declared, three elements each")
    assert_eq(appearance.wiper_arms[0], "wiper1_p1")
    assert_eq(appearance.wiper_arms[9], "wiper4_p1", "the fourth wiper after two the model has not got")


func test_counts_end_at_the_first_negative_number() -> void:
    var appearance:RailVehicleAppearance = _appearance("animated_short")
    assert_eq(appearance.powered_wheels, PackedStringArray(["wheel1", "wheel2"]), "the wheels before the end mark")
    assert_eq(appearance.pantograph_front_arms, PackedStringArray(), "no pantograph after it")
    assert_eq(appearance.wiper_arms, PackedStringArray(), "no wiper after it")


# DynObj.cpp:5361-5388 - with rolling wheels of their own diameter the axle arrangement, read from its
# second character, splits the axles: 1A1 makes wheel1 powered (the A) and wheel2 rear rolling
func test_rolling_wheels_follow_the_axle_arrangement() -> void:
    var appearance:RailVehicleAppearance = _appearance("rolling")
    assert_eq(appearance.powered_wheels, PackedStringArray(["wheel1"]))
    assert_eq(appearance.rear_rolling_wheels, PackedStringArray(["wheel2"]))
    assert_eq(appearance.front_rolling_wheels, PackedStringArray())


func test_wheels_of_one_diameter_are_all_powered() -> void:
    assert_eq(_appearance("animated").powered_wheels, PackedStringArray(["wheel1", "wheel2"]))


# DynObj.cpp:2539-2545 - the destination sign is the submodel with replaceable skin 4
func test_the_destination_sign_is_the_replaceable_skin_4() -> void:
    assert_eq(_appearance("animated").head_display_submodel, "tablica")


# DynObj.cpp:5721-5790 - the doors by animdoorprefix:, each with the two submodels below it a folding
# door turns (DynObj.cpp:592-622), the steps by animstepprefix:; what the model has not got is empty
func test_doors_and_steps_are_named_by_their_prefixes() -> void:
    var appearance:RailVehicleAppearance = _appearance("doors")
    assert_eq(appearance.doors, PackedStringArray(["drzwi1", "drzwi1_a", "drzwi1_b", "drzwi2", "", ""]))
    assert_eq(appearance.door_steps, PackedStringArray(["stopien1", ""]))


# DynObj.cpp:5702-5719 - pendulums 1 to 4 of the prefix the model has, with their amplitude; none
# while animations: declares no levers (DynObj.cpp:1121)
func test_pendulums_swing_only_with_levers_declared() -> void:
    var appearance:RailVehicleAppearance = _appearance("pendulum")
    assert_eq(appearance.pendulums, PackedStringArray(["drzwi1", "drzwi2"]))
    assert_eq(appearance.pendulum_amplitude, 15.0)
    assert_eq(_appearance("doors").pendulums, PackedStringArray())
