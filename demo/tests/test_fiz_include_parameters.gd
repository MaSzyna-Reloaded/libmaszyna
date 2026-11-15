extends MaszynaGutTest

## A file an include directive brings in, read on its own as the directive reads it
## (FizVehicleBuilder.read_description() with the directive's parameters): its (pN) are the
## parameters given - one not given reads as "none", as cParser::substituteParameters has it, and
## a file read with none keeps them as written - and it takes as many parameters as its highest
## (pN) in its data, a line commented out not counted

const FIXTURES:String = "res://tests/fixtures/fiz_parameters"
const INCLUDED_FILE:String = "parameters.inc"
## A file of no (pN) of its own, including one that has them
const FILE_OF_NO_PARAMETERS:String = "res://tests/fixtures/fiz_sources/source_map.fiz"
## parameters.inc's highest (pN): (p3), the (p4) of a line commented out not counted
const PARAMETER_COUNT:int = 3
const MASS:float = 52000.0
const AXLES:String = "Bo'Bo'"
const WHEEL_DIAMETER:float = 1.1
const PARAMETERS:Dictionary = {"p1": "52000", "p2": "Bo'Bo'", "p3": "1.1"}
## p2 not given
const PARAMETERS_SHORT:Dictionary = {"p1": "52000", "p3": "1.1"}


func _read(parameters:Dictionary) -> FizImportContext:
    return FizVehicleBuilder.read_description(
            ProjectSettings.globalize_path(FIXTURES.path_join(INCLUDED_FILE)), parameters)


func _wheels(context:FizImportContext) -> RailVehicleWheels:
    return context.get_part("RailVehicleWheels") as RailVehicleWheels


func test_the_parameters_given_are_its_values() -> void:
    var context:FizImportContext = _read(PARAMETERS)

    assert_almost_eq(context.controller.mass, MASS, 0.001)
    assert_almost_eq(_wheels(context).powered_wheel_diameter, WHEEL_DIAMETER, 0.0001)
    assert_eq(_wheels(context).axle_arrangement, AXLES)


func test_a_parameter_not_given_reads_as_none() -> void:
    var context:FizImportContext = _read(PARAMETERS_SHORT)

    assert_eq(_wheels(context).axle_arrangement, "none")
    assert_almost_eq(_wheels(context).powered_wheel_diameter, WHEEL_DIAMETER, 0.0001, "the one after it given")


func test_read_with_no_parameters_it_keeps_them_as_written() -> void:
    assert_eq(_wheels(_read({})).axle_arrangement, "(p2)")


func test_it_takes_as_many_parameters_as_its_highest() -> void:
    assert_eq(_read(PARAMETERS).parameter_count, PARAMETER_COUNT)
    assert_eq(_read({}).parameter_count, PARAMETER_COUNT, "the same read with none")


func test_a_file_of_no_parameters_takes_none_whatever_it_includes() -> void:
    assert_eq(FizVehicleBuilder.read_description(ProjectSettings.globalize_path(FILE_OF_NO_PARAMETERS)).parameter_count, 0)
