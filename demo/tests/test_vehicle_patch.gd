extends MaszynaGutTest

## A game's patch of a vehicle's files (MaszynaVehiclePatch, MaszynaVehicleSystem.patch_set()) adds
## what they do not define - the wiper arms of its model, the controls of its cab's model, the wiper
## switch of its FIZ - and only that: what a file defines of its own stays its own.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const DATA_PATH:String = "dynamic/test/animated_v1"
const FILE_NAME:String = "patched"
## The fixture's model, of its vehicle and of its cab
const MODEL_PATH:String = "dynamic/test/animated_v1/animated.t3d"
const FIZ_PATH:String = "dynamic/test/animated_v1/patched.fiz"
const VALVE_FIZ_PATH:String = "dynamic/test/valves.fiz"
const CAB:int = 1

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    MaszynaVehicleSystem.patch_set(_patch())


func after_each() -> void:
    MaszynaVehicleSystem.patch_set(MaszynaVehiclePatch.new())
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _patch() -> MaszynaVehiclePatch:
    var model := MaszynaVehicleModelPatch.new()
    model.path = MODEL_PATH
    var arm := MaszynaWiperArmPatch.new()
    arm.submodel = "wiper1_p1"
    arm.pivot = Vector3(0.0, 0.0, 0.5)
    arm.axis = Vector3.RIGHT
    arm.turn = 63.0
    var nested_arm := MaszynaWiperArmPatch.new()
    nested_arm.submodel = "DRZWI1/drzwi1_a/drzwi1_b"
    nested_arm.turn = -40.0
    var missing_arm := MaszynaWiperArmPatch.new()
    missing_arm.submodel = "nadwozie/wiper2_p1"
    var handle := MaszynaCabControlPatch.new()
    handle.label = "reloaded_manualwiper1"
    handle.submodel = "nadwozie"
    handle.scale = 0.175
    handle.mouse_drag_signs = Vector2(0.0, -1.0)
    var defined := MaszynaCabControlPatch.new()
    defined.label = "converter_sw"
    defined.submodel = "tablica"
    model.elements = [arm, nested_arm, missing_arm, handle, defined]

    var fiz := MaszynaVehicleModelPatch.new()
    fiz.path = FIZ_PATH
    var wiper_list := MaszynaWiperListPatch.new()
    wiper_list.angle = 83.0
    var off := RailVehicleWiperListItem.new()
    var interval := RailVehicleWiperListItem.new()
    interval.wiper_mask = 3
    interval.transit_time = 1.0
    interval.period = 5.0
    wiper_list.positions = [off, interval]
    fiz.elements = [wiper_list]

    var valve_fiz := MaszynaVehicleModelPatch.new()
    valve_fiz.path = VALVE_FIZ_PATH
    var valves := MaszynaWiperValvesPatch.new()
    valves.slowest_sweep_time = 2.0
    valves.fastest_sweep_time = 0.5
    valves.sensitivity = 1.5
    valve_fiz.elements = [valves]

    var patch := MaszynaVehiclePatch.new()
    patch.models = [model, fiz, valve_fiz]
    return patch


func test_the_model_gets_the_wiper_arms_of_its_patch() -> void:
    var appearance:MaszynaRailVehicleAppearance = MaszynaRailVehicle3DInstancer.read_structure(
            DATA_PATH, FILE_NAME, "", FILE_NAME).appearance
    assert_eq(appearance.wiper_arms, PackedStringArray(
            ["wiper1_p1", "", "", "drzwi1_b", "", "", "", "", ""]),
            "each arm one submodel, by its path in any case; one the model has not got kept empty")
    assert_eq(appearance.wiper_pivots, PackedVector3Array([Vector3(0.0, 0.0, 0.5), Vector3.ZERO, Vector3.ZERO]))
    assert_eq(appearance.wiper_axes, PackedVector3Array([Vector3.RIGHT, Vector3.UP, Vector3.UP]))
    assert_eq(appearance.wiper_turns, PackedFloat64Array([63.0, -40.0, 90.0]))


func test_the_cab_gets_the_controls_its_mmd_lacks() -> void:
    var abs_mmd_path:String = MaszynaGameDirectory.get_game_dir().path_join(DATA_PATH).path_join(FILE_NAME + ".mmd")
    var random_choices:Dictionary = {}
    var definition:MmdCabinDefinition = MmdCabinInstancer.parse(abs_mmd_path, {}, CAB, random_choices)
    var labels:Array[String] = []
    for descriptor:MmdInstrumentDescriptor in definition.instruments:
        labels.append("%s %s" % [descriptor.label, descriptor.submodel_name])
    assert_eq(labels, ["converter_sw nadwozie", "reloaded_manualwiper1 nadwozie"],
            "the MMD's converter switch kept, the handle added")
    var handle:MmdInstrumentDescriptor = definition.instruments[1]
    assert_eq([handle.animation_type, handle.scale, handle.mouse_drag_signs], ["rot", 0.175, Vector2(0.0, -1.0)])


func test_a_cab_control_of_a_path_takes_the_submodel_of_its_exact_case() -> void:
    var model := Node3D.new()
    var cab := Node3D.new()
    cab.name = "kabina_b"
    model.add_child(cab)
    var valve := Node3D.new()
    valve.name = "Object12"
    cab.add_child(valve)
    var plate := Node3D.new()
    plate.name = "object12"
    cab.add_child(plate)
    var index:Dictionary = {}
    MmdCabinInstancer._index_submodels(model, index)
    assert_eq(index[MmdCabinInstancer.submodel_key("kabina_b/Object12")][0], valve)
    assert_eq(index[MmdCabinInstancer.submodel_key("kabina_b/object12")][0], plate)
    model.free()


func test_a_fiz_gets_the_valve_speeds_of_its_patch() -> void:
    var wipers:MaszynaMoverRailVehicleWipers = MaszynaVehicleSystem.fiz_get_patched_wipers(VALVE_FIZ_PATH)
    assert_not_null(wipers)
    assert_eq(wipers.valve_slowest_sweep_time, 2.0)
    assert_eq(wipers.valve_fastest_sweep_time, 0.5)
    assert_eq(wipers.valve_sensitivity, 1.5)
    assert_true(wipers.positions.is_empty(), "no switch: its wipers stay driven by hand")


func test_a_fiz_of_no_wiper_list_gets_the_switch_of_its_patch() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(
            MaszynaGameDirectory.get_game_dir().path_join(FIZ_PATH), MaszynaVehicleSystem.fiz_get_patched_wipers(FIZ_PATH))
    var wipers:RailVehicleWipers = null
    for component:VehicleComponent in description.get_components():
        if component is RailVehicleWipers:
            wipers = component
    assert_not_null(wipers)
    assert_eq(wipers.angle, 83.0)
    assert_eq(wipers.positions.size(), 2, "the patch's switch, not wipers driven by hand")
    assert_eq(wipers.positions[1].period, 5.0)
