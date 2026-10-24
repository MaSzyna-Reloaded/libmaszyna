extends MaszynaGutTest

## The ".scn" "lines", "line_strip" and "line_loop" nodes (lines_node::import(), scenenode.cpp:561-649):
## the colour divided by 255, the width capped at 30, a strip and a loop turned into pairs of
## points, an odd last point left out, and the points placed by the enclosing rotate and origin

const NodeImporter = preload("res://addons/libmaszyna/scenery/maszyna_node_importer.gd")

## The token the parse goes on with after the node
const NEXT:String = "next"


func _import(source:String, context:MaszynaImporterContext = MaszynaImporterContext.new()) -> MaszynaImporterContext:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize((source + " " + NEXT).to_utf8_buffer())
    NodeImporter.new().import(parser, context)
    assert_eq(parser.next_token(), NEXT, "the node is read to its endline")
    return context


func test_lines_are_pairs_of_points_with_their_colour_width_and_range() -> void:
    var context:MaszynaImporterContext = _import("500 10 none lines 51 102 255 45.0 1 2 3 4 5 6 7 8 9 10 11 12 endline")

    assert_eq(context.lines.size(), 1)
    var lines:LinesData = context.lines[0]
    assert_eq(lines.points, PackedVector3Array([
        Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9), Vector3(10, 11, 12)
    ]))
    assert_eq(lines.color, Color(0.2, 0.4, 1.0))
    assert_eq(lines.width, 30.0, "a width over 30 is 30")
    assert_eq(lines.range_min, 10.0)
    assert_eq(lines.range_max, 500.0)


func test_a_strip_is_its_points_joined_and_has_no_limit_when_its_range_is_negative() -> void:
    var context:MaszynaImporterContext = _import("-1 0 line0_1 line_strip 0 0 0 8 0 0 0 1 0 0 1 1 0 endline")

    var lines:LinesData = context.lines[0]
    assert_eq(lines.points, PackedVector3Array([
        Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0)
    ]))
    assert_eq(lines.range_max, 0.0, "no limit")


func test_a_loop_is_closed() -> void:
    var context:MaszynaImporterContext = _import("-1 0 none line_loop 0 0 0 1 0 0 0 1 0 0 1 1 0 endline")

    assert_eq(context.lines[0].points, PackedVector3Array([
        Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0), Vector3(1, 1, 0), Vector3(0, 0, 0)
    ]))


func test_the_odd_last_point_of_lines_is_left_out() -> void:
    var context:MaszynaImporterContext = _import("-1 0 none lines 0 0 0 1 0 0 0 1 0 0 2 0 0 endline")

    assert_eq(context.lines[0].points, PackedVector3Array([Vector3(0, 0, 0), Vector3(1, 0, 0)]))
    assert_push_error("Lines node specified odd number of vertices")


## basic_region::insert() (scene.cpp:1370-1390): rotated, then moved by the origin
func test_the_points_are_rotated_and_moved_by_the_enclosing_rotate_and_origin() -> void:
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    context.push_origin(Vector3(100, 0, 200))
    context.push_rotate(Vector3(0, deg_to_rad(90.0), 0))
    _import("-1 0 none lines 0 0 0 1 1 0 0 0 0 1 endline", context)

    var points:PackedVector3Array = context.lines[0].points
    assert_almost_eq(points[0], Vector3(100, 0, 199), Vector3.ONE * 0.0001)
    assert_almost_eq(points[1], Vector3(101, 0, 200), Vector3.ONE * 0.0001)


## simulationstateserializer.cpp:586-601: with a region file the lines are the file's
func test_with_a_region_file_the_lines_are_left_out() -> void:
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    context.binary_terrain = true
    _import("-1 0 none line_strip 0 0 0 1 0 0 0 1 0 0 endline", context)

    assert_eq(context.lines.size(), 0)
