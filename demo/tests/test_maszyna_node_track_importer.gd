extends MaszynaGutTest

## maszyna_node_track_importer.gd - the ".scn track node" reader.

const TrackImporter = preload("res://addons/libmaszyna/scenery/maszyna_node_track_importer.gd")

## A "normal" track whose trackbed slot holds the format's "no texture" sentinel, followed by a
## marker token so the parser position after the node can be checked.
const NONE_TRACKBED_SOURCE:String = """
normal 6.092 1.435 0.15 25 25 0 flat vis
rail_screw_used1 4.55 none 0.6 0.9 0.9
-15992.16 0.603 -2902.416 0
0 0 0
0 0 0
-15997.885 0.63 -2900.332 0
0
velocity 80
endtrack
MARKER
"""
## krzyzowa2's krzyzowa_tor8end: its exit signal and its W4 both on event2 - FINDINGS.md
## 2026-09-29, the second overwrote the first and the goods train left past a signal at stop
const TWO_EVENTS_SOURCE:String = """
normal 50.0 1.435 0.25 25.0 20 0 flat vis
rail_screw_used1 4 tpbps-new2 0.2 0.5 1.1
-3038.84 0.195275 -4651.62 0.0
0.0 0.0 0.0
0.0 0.0 0.0
-2988.86 0.195275 -4652.84 0.0
0
event2 krzyzowa_p2_sem_info
event2 Krzyzowa#p_stopinfo
endtrack
"""
## A track type the wrapper does not build yet.
const RIVER_SOURCE:String = """
river 10.0 8.0 0.15 25 25 0 flat vis
water 4.0 none 0.0 0.0 0.0
0 0 0 0
0 0 0
0 0 0
10 0 0 0
0
endtrack
MARKER
"""
## td/drogi.scm's d1: a road with its surface and its shoulders
const ROAD_SOURCE:String = """
road 0 8 0.85 0 20 0 flat vis
asphalt1_solidwhite 8 asphaltdark1_side1 0.38 0.38 0.38
0 0 0 0
0 0 0
0 0 0
40 0 0 0
0
velocity 45
endtrack
MARKER
"""


func _import(source:String) -> Array:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(source.to_utf8_buffer())
    var track:MaszynaTrackData = TrackImporter.new().import(parser, MaszynaImporterContext.new())
    return [track, parser]


func test_none_trackbed_is_not_a_material_name() -> void:
    var result:Array = _import(NONE_TRACKBED_SOURCE)
    var track:MaszynaTrackData = result[0]

    assert_not_null(track)
    assert_eq(track.material1, "rail_screw_used1")
    assert_eq(track.material2, "", "\"none\" is the format's sentinel, not a texture")


func test_unsupported_track_type_is_skipped_whole() -> void:
    var result:Array = _import(RIVER_SOURCE)
    var parser:MaszynaParser = result[1]

    assert_null(result[0])
    assert_eq(parser.next_token(), "MARKER", "the rejected node must be consumed up to endtrack")


func test_a_road_is_read_as_a_road_track() -> void:
    var result:Array = _import(ROAD_SOURCE)
    var track:MaszynaTrackData = result[0]
    var parser:MaszynaParser = result[1]

    assert_not_null(track)
    assert_eq(track.kind, TrackServer.TRACK_ROAD)
    assert_eq(track.width, 8.0)
    assert_eq(track.material1, "asphalt1_solidwhite")
    assert_eq(track.material2, "asphaltdark1_side1")
    assert_eq(track.tex_height, 0.38)
    assert_eq(track.curve.p2, Vector3(40, 0, 0))
    assert_eq(track.speed_limit, 45.0)
    assert_eq(track.surface, TrackData.SURFACE_ASPHALT, "an asphalt texture is an asphalt road")
    assert_true(track.centre_line, "a solid white line on the texture is a centre line")
    assert_true(track.shoulder, "a second texture is the road's shoulder")
    assert_eq(parser.next_token(), "MARKER", "the node is read up to endtrack")


func test_a_slot_keeps_every_event_the_track_names_for_it() -> void:
    var track:MaszynaTrackData = _import(TWO_EVENTS_SOURCE)[0]

    assert_eq(track.events.get("event2", PackedStringArray()),
            PackedStringArray(["krzyzowa_p2_sem_info", "Krzyzowa#p_stopinfo"]),
            "the signal and the W4 on one slot are both the track's")


## A track's keywords are read in lower case, as the original reads every token (Track.cpp:810-928):
## "Event2" is the event2 slot
func test_a_keyword_with_capitals_is_its_slot() -> void:
    var track:MaszynaTrackData = _import(TWO_EVENTS_SOURCE.replace("event2 krzyzowa_p2", "Event2 krzyzowa_p2"))[0]

    assert_eq(track.events.get("event2", PackedStringArray()).size(), 2, "both events in event2")
