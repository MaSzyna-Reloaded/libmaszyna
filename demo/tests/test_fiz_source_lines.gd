extends MaszynaGutTest

## Where the lines a FIZ was read of stand in its files: every section's header and every row of a
## table, in the file it is written in - an included one too, with the directive that brought it in
## and the (pN) it passed. Noted only when the project asks for it (SOURCE_LINES_SETTING), as a tool
## writing the files back does; the game reads them without it

const FIXTURES:String = "res://tests/fixtures/fiz_sources"
const MAIN_FILE:String = "source_map.fiz"
const INCLUDED_FILE:String = "wheels.inc"
const WHEEL_DIAMETER:float = 1.25


func before_each() -> void:
    ProjectSettings.set_setting(FizVehicleBuilder.SOURCE_LINES_SETTING, true)


func after_each() -> void:
    ProjectSettings.set_setting(FizVehicleBuilder.SOURCE_LINES_SETTING, FizVehicleBuilder.SOURCE_LINES_DEFAULT)


func _read_description() -> FizImportContext:
    return FizVehicleBuilder.read_description(ProjectSettings.globalize_path(FIXTURES.path_join(MAIN_FILE)))


## The file's bytes from `begin`, as many as `text` has
func _bytes_at(path:String, begin:int, text:String) -> String:
    return FileAccess.get_file_as_bytes(path).slice(begin, begin + text.length()).get_string_from_ascii()


func test_nothing_is_noted_by_default() -> void:
    assert_eq(ProjectSettings.property_get_revert(FizVehicleBuilder.SOURCE_LINES_SETTING), false)
    ProjectSettings.set_setting(FizVehicleBuilder.SOURCE_LINES_SETTING, FizVehicleBuilder.SOURCE_LINES_DEFAULT)
    var context:FizImportContext = _read_description()
    assert_eq(context.source_lines.size(), 0)
    assert_eq(context.includes.size(), 0)


func test_the_headers_are_in_the_order_read() -> void:
    var sections:Array[String] = []
    for line:FizSourceLine in _read_description().source_lines:
        if not line.row:
            sections.append(line.section)
    assert_eq(sections, ["Param.", "Wheels:", "LightsList:"] as Array[String])


func test_a_header_points_at_its_line_in_its_file() -> void:
    var main_path:String = ProjectSettings.globalize_path(FIXTURES.path_join(MAIN_FILE))
    var param:FizSourceLine = _read_description().source_lines[0]
    assert_eq(param.path, main_path)
    assert_null(param.include)
    assert_eq(_bytes_at(param.path, param.begin, "Param.M=74000"), "Param.M=74000")


func test_an_included_header_is_written_as_in_its_file_and_knows_its_directive() -> void:
    var main_path:String = ProjectSettings.globalize_path(FIXTURES.path_join(MAIN_FILE))
    var included_path:String = ProjectSettings.globalize_path(FIXTURES.path_join(INCLUDED_FILE))
    var context:FizImportContext = _read_description()
    var wheels:FizSourceLine = context.source_lines[1]
    assert_eq(wheels.path, included_path)
    assert_eq(_bytes_at(wheels.path, wheels.begin, "Wheels:D=(p1)"), "Wheels:D=(p1)")
    assert_not_null(wheels.include)
    assert_eq(wheels.include.path, main_path)
    assert_eq(wheels.include.included_path, included_path)
    assert_eq(wheels.include.parameters, ["1.25"] as Array[String])
    var directive:String = "include wheels.inc 1.25 end\r\n"
    assert_eq(wheels.include.end - wheels.include.begin, directive.length())
    assert_eq(_bytes_at(main_path, wheels.include.begin, directive), directive)
    # the source is only noted: the (p1) is still substituted
    var wheels_component:RailVehicleWheels = context.get_part("RailVehicleWheels") as RailVehicleWheels
    assert_almost_eq(wheels_component.powered_wheel_diameter, WHEEL_DIAMETER, 0.0001)


func test_every_include_followed_is_kept() -> void:
    var context:FizImportContext = _read_description()
    assert_eq(context.includes.size(), 1)
    assert_eq(context.includes[0], context.source_lines[1].include)


func test_the_rows_of_a_table_are_its_section_s() -> void:
    var rows:Array[FizSourceLine] = []
    for line:FizSourceLine in _read_description().source_lines:
        if line.row:
            rows.append(line)
    assert_eq(rows.size(), 2)
    assert_eq(rows[0].section, "LightsList:")
    assert_eq(_bytes_at(rows[0].path, rows[0].begin, "4\t34"), "4\t34")
    assert_eq(_bytes_at(rows[1].path, rows[1].begin, "17\t64"), "17\t64")
