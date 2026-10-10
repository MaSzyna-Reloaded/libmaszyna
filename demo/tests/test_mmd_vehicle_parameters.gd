extends MaszynaGutTest

## A vehicle's MMD is read as "include <TypeName>.mmd <name> <TypeName> <skin> end"
## (DynObj.cpp:5260): its (p1) is the vehicle's name, (p2) its type, (p3) its skin - and its
## `attachments:` (DynObj.cpp:5384) name their models with them.

const FIXTURE_DIR:String = "res://tests/fixtures/mmd_parameters"


func _parameters() -> Dictionary:
    return MmdCabinInstancer.vehicle_parameters("NAME-1", "Vehicle", "skin-a")


func test_type_name_is_the_body_model_of_an_mmd_that_only_includes():
    var path:String = ProjectSettings.globalize_path(FIXTURE_DIR.path_join("vehicle.mmd"))
    assert_eq(MmdCabinInstancer.parse_body_model(path, _parameters()), "Vehicle")


func test_attachments_are_named_by_the_parameters_and_a_random_set_is_one_of_them():
    var path:String = ProjectSettings.globalize_path(FIXTURE_DIR.path_join("vehicle.mmd"))
    var expected:PackedStringArray = PackedStringArray(["parts/skin-a", "extra_NAME-1", "random"])
    assert_eq(MmdCabinInstancer.parse_attachments(path, _parameters()), expected)


func test_an_mmd_that_names_p1_belongs_to_its_vehicle():
    assert_true(MmdCabinInstancer.names_vehicle(
            ProjectSettings.globalize_path(FIXTURE_DIR.path_join("vehicle.mmd"))))
    assert_false(MmdCabinInstancer.names_vehicle(
            ProjectSettings.globalize_path(FIXTURE_DIR.path_join("plain.mmd"))))


func test_an_mmd_without_attachments_has_none():
    var path:String = ProjectSettings.globalize_path(FIXTURE_DIR.path_join("plain.mmd"))
    assert_eq(MmdCabinInstancer.parse_attachments(path, _parameters()), PackedStringArray())
