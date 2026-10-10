extends MaszynaGutTest

## A cab is read from its own cab<N>definition: wherever the MMD puts it, until the next
## cab0definition: (TTrain::InitializeCab(), Train.cpp:10504-10517, 10700-10704) - 133 MMDs of the
## game data declare cab 2 before cab 1, or cab 0 between them.

const MMD:String = "res://tests/fixtures/dynamic/test/animated_v1/cabs_reversed.mmd"
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const DATA_PATH:String = "dynamic/test/animated_v1"
const OFF_SUBMODEL_MMD:String = "res://tests/fixtures/dynamic/test/animated_v1/off_submodel.mmd"
const CABLIGHT_MMD:String = "res://tests/fixtures/dynamic/test/animated_v1/cablight.mmd"


func _labels(cab:int) -> Array[String]:
    var definition:MmdCabinDefinition = MmdCabinInstancer.parse(
            ProjectSettings.globalize_path(MMD), MmdCabinInstancer.vehicle_parameters("cabs", "cabs_reversed", ""), cab, {})
    var labels:Array[String] = []
    for instrument:MmdInstrumentDescriptor in definition.instruments:
        labels.append(instrument.label)
    return labels


func test_cab_2_declared_first_has_its_controls() -> void:
    var labels:Array[String] = _labels(2)
    assert_true("battery_sw" in labels, "cab 2's own control")
    assert_true("converter_sw" in labels, "and cab 1's, which follows it before cab0definition:")


func test_cab_1_reads_from_its_own_definition() -> void:
    var labels:Array[String] = _labels(1)
    assert_true("converter_sw" in labels)
    assert_false("battery_sw" in labels, "cab 2's section comes before cab 1's")


## A cablight: inside a cab is no control: the original passes over the label alone, and the
## controls after it are read (EP07 4e-ep-tv-g.mmd lost its i-radio and timetable screen to it)
func test_cablight_inside_a_cab_does_not_swallow_the_next_controls() -> void:
    var definition:MmdCabinDefinition = MmdCabinInstancer.parse(
            ProjectSettings.globalize_path(CABLIGHT_MMD), MmdCabinInstancer.vehicle_parameters("cabs", "cablight", ""), 1, {})
    var labels:Array[String] = []
    for instrument:MmdInstrumentDescriptor in definition.instruments:
        labels.append(instrument.label)
    assert_true("converter_sw" in labels)
    assert_eq(definition.bounds_min, Vector3(-1.0, 1.5, 4.0), "the bounds after cablight:'s colours (TCab::Load)")
    assert_eq(definition.interior_light, Color(0.5, 0.25, 1.0), "internaldata:'s cablight: base colour")


## TGauge::Load (Gauge.cpp:186-197): a control's submodel by its name, else by its name with "_off"
func test_a_control_binds_its_off_submodel() -> void:
    var previous_game_dir:String = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    var definition:MmdCabinDefinition = MmdCabinInstancer.parse(
            ProjectSettings.globalize_path(OFF_SUBMODEL_MMD),
            MmdCabinInstancer.vehicle_parameters("cabs", "off_submodel", ""), 1, {})
    var root:Node3D = add_child_autofree(Node3D.new())
    var diagnostics:Array[Dictionary] = []
    MmdCabinInstancer.build_into(root, definition, RID(), DATA_PATH, "", diagnostics)
    await wait_idle_frames(2)
    var buttons:Array[Node] = root.find_children("*", "CabinButton", true, false)
    UserSettings.save_maszyna_game_dir(previous_game_dir)
    assert_eq(buttons.size(), 1)
    var mesh_path:NodePath = buttons[0].get("mesh_path")
    assert_true(not mesh_path.is_empty(), "bound to kierunek_przod_off")
    assert_eq(String(buttons[0].get_node(mesh_path).name).to_lower(), "kierunek_przod_off")
