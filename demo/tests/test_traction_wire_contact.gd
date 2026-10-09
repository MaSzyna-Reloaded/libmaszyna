extends MaszynaGutTest

## The geometry half of the pantograph: which span is overhead, and how far the collector reaches.
## Built from bare wires rather than from a scenery, so it says nothing about voltage - only about
## where contact is kept and where it is lost.
##
## These cases are real losses of contact reported on zwierzyniec_tlk: an EP08 dropping the line
## voltage while crossing wire junctions, and then dropping it at the exit of one switch whatever
## the speed. In that scenery 126 span ends have three neighbours each - four spans meeting over a
## switch - and only two ends anywhere have a gap wider than the joining tolerance, so the spans
## are not the problem; deciding which of them is overhead is.

## Half of a 1.4 m slider, which is what every electric locomotive of the datapack declares
## (CSW=1.4 in 134 of 135 .fiz files).
const SLIDER_HALF_WIDTH:float = 0.7
## DynObj.cpp:93 fWidthExtra - the guide horn beyond the slider.
const HORN_WIDTH:float = 0.381
const WIRE_HEIGHT:float = 5.8
const SPAN_LENGTH:float = 20.0

const UP:Vector3 = Vector3(0.0, 1.0, 0.0)
const FORWARD:Vector3 = Vector3(0.0, 0.0, 1.0)
const LEFT:Vector3 = Vector3(1.0, 0.0, 0.0)

var _wires:Array[RID] = []


func after_each() -> void:
    for wire:RID in _wires:
        TractionServer.wire_free(wire)
    _wires.clear()


func _add_wire(from:Vector3, to:Vector3) -> RID:
    var wire:RID = TractionServer.wire_create()
    _wires.append(wire)
    TractionServer.wire_set_params(wire, from, to, "test_power", 3000.0, 2000.0, 0.01)
    return wire


## A run of spans end to end along +Z, starting at the origin. Seven is the shortest run with two
## adjacent spans that are neither last nor second to last, and only such a span has its chain
## followed at all - which is why these cases work in the middle of the run and not at its ends.
func _add_chain(count:int, lateral:float = 0.0) -> Array[RID]:
    var chain:Array[RID] = []
    for index:int in count:
        chain.append(_add_wire(
                Vector3(lateral, WIRE_HEIGHT, index * SPAN_LENGTH),
                Vector3(lateral, WIRE_HEIGHT, (index + 1) * SPAN_LENGTH)))
    TractionServer.network_build()
    return chain


func _follow(from:RID, along:float) -> Dictionary:
    return TractionServer.wire_follow_above(
            from, Vector3(0.0, 0.0, along), UP, FORWARD, LEFT, SLIDER_HALF_WIDTH, HORN_WIDTH)


## Running off the end of a span is not a loss of contact - the next span is reached along the
## chain, in the same frame, the way vehicle_table::update_traction() does (DynObj.cpp:8742).
func test_a_pantograph_crossing_a_span_junction_keeps_contact() -> void:
    var chain:Array[RID] = _add_chain(7)

    var held:RID = chain[2]
    var lost_at:PackedFloat32Array = PackedFloat32Array()
    var offset:float = 2.0 * SPAN_LENGTH + 1.0
    while offset <= 4.0 * SPAN_LENGTH - 1.0:
        var wire:RID = _follow(held, offset)["rid"]
        if wire.is_valid():
            held = wire
        else:
            lost_at.append(offset)
        offset += 0.5

    assert_eq(lost_at.size(), 0, "contact should never be lost while crossing the junction")
    assert_eq(held, chain[3], "and the pantograph should end up on the span it crossed into")


## At the end of a section the chain stops being the whole story: the wire actually overhead may
## be one this span does not point to - the far side of a switch, or a parallel run. The original
## gives up on the chain for the last and second-to-last span and looks around instead
## (DynObj.cpp:8747), whether or not the span it is on would still do. Without this an EP08 lost
## the line at the exit of one switch on zwierzyniec_tlk, at any speed.
func test_the_chain_is_not_followed_at_the_end_of_a_section() -> void:
    var chain:Array[RID] = _add_chain(7)

    assert_false(
            RID(_follow(chain[0], 0.5 * SPAN_LENGTH)["rid"]).is_valid(),
            "the last span of a section sends the pantograph back to an area search")
    assert_false(
            RID(_follow(chain[1], 1.5 * SPAN_LENGTH)["rid"]).is_valid(),
            "and so does the second to last")
    assert_eq(
            _follow(chain[3], 3.5 * SPAN_LENGTH)["rid"], chain[3],
            "a span in the middle of a section is followed")


## The area search is what answers there, and it finds the span regardless of the chain.
func test_the_area_search_still_finds_a_span_at_a_section_end() -> void:
    var chain:Array[RID] = _add_chain(7)

    var found:Dictionary = TractionServer.wire_find_above_with_height(
            Vector3(0.0, 0.0, 0.5 * SPAN_LENGTH), UP, FORWARD, LEFT, SLIDER_HALF_WIDTH, HORN_WIDTH)
    assert_eq(found["rid"], chain[0], "the end span is still the wire overhead")


## The spans a scenery leaves a hand's width apart are not one wire. The original joins two ends
## only within 0.025 m on every axis (Traction.cpp:355); joining what is further apart sends the
## pantograph along a chain into a span the author never connected. In zwierzyniec_tlk a 0.25 m
## tolerance saw three candidate neighbours at 25 span ends where the original sees one.
func test_spans_are_joined_at_the_distance_the_original_joins_them() -> void:
    var chain:Array[RID] = _add_chain(7)
    assert_eq(
            _follow(chain[2], 3.0 * SPAN_LENGTH + 1.0)["rid"], chain[3],
            "ends laid on the same point are one wire")

    after_each()
    var apart_gap:float = 0.1
    for index:int in 7:
        var start:float = index * SPAN_LENGTH + (apart_gap if index > 2 else 0.0)
        _add_wire(
                Vector3(0.0, WIRE_HEIGHT, start),
                Vector3(0.0, WIRE_HEIGHT, (index + 1) * SPAN_LENGTH))
    TractionServer.network_build()
    assert_false(
            RID(_follow(_wires[2], 3.0 * SPAN_LENGTH + 1.0)["rid"]).is_valid(),
            "ends further apart are two wires, and the chain ends there")


## What following the chain buys over searching the area every frame: the pantograph stays on the
## wire it is under, so a lower wire crossing overhead - which over a switch is the diverging
## span - does not steal the contact.
func test_a_lower_crossing_wire_does_not_steal_the_contact() -> void:
    var chain:Array[RID] = _add_chain(7)
    _add_wire(
            Vector3(0.0, WIRE_HEIGHT - 0.2, 3.0 * SPAN_LENGTH),
            Vector3(0.0, WIRE_HEIGHT - 0.2, 4.0 * SPAN_LENGTH))
    TractionServer.network_build()

    var found:Dictionary = _follow(chain[3], 3.5 * SPAN_LENGTH)
    assert_eq(found["rid"], chain[3], "the span being followed keeps the contact")
    assert_almost_eq(float(found["height"]), WIRE_HEIGHT, 0.001, "at its own height")


## The horn catches a wire the slider alone would miss, and reports it as higher than a wire
## straight overhead, so a span properly above still wins (scene.cpp:105-112).
func test_a_wire_over_the_horn_is_still_caught_and_reads_higher() -> void:
    var overhang:float = 0.5 * HORN_WIDTH
    var wire:RID = _add_wire(
            Vector3(SLIDER_HALF_WIDTH + overhang, WIRE_HEIGHT, 0.0),
            Vector3(SLIDER_HALF_WIDTH + overhang, WIRE_HEIGHT, SPAN_LENGTH))
    TractionServer.network_build()

    var found:Dictionary = TractionServer.wire_find_above_with_height(
            Vector3(0.0, 0.0, 10.0), UP, FORWARD, LEFT, SLIDER_HALF_WIDTH, HORN_WIDTH)
    assert_eq(found["rid"], wire, "a wire on the horn is in reach")
    assert_gt(float(found["height"]), WIRE_HEIGHT, "and counts as higher than one on the slider")


func test_a_wire_beyond_the_horn_is_out_of_reach() -> void:
    _add_wire(
            Vector3(SLIDER_HALF_WIDTH + HORN_WIDTH + 0.1, WIRE_HEIGHT, 0.0),
            Vector3(SLIDER_HALF_WIDTH + HORN_WIDTH + 0.1, WIRE_HEIGHT, SPAN_LENGTH))
    TractionServer.network_build()

    var found:Dictionary = TractionServer.wire_find_above_with_height(
            Vector3(0.0, 0.0, 10.0), UP, FORWARD, LEFT, SLIDER_HALF_WIDTH, HORN_WIDTH)
    assert_false(RID(found["rid"]).is_valid(), "past the horn there is nothing to collect from")


## A span that shares its running with another is not trusted either: the sibling is not on the
## chain, so it can only be found by looking around (Traction.cpp:838-852).
func test_a_span_with_a_parallel_run_is_not_followed() -> void:
    var chain:Array[RID] = _add_chain(7)
    TractionServer.wire_set_parallel(chain[3], "some_other_span")
    TractionServer.network_build()

    assert_false(
            RID(_follow(chain[3], 3.5 * SPAN_LENGTH)["rid"]).is_valid(),
            "a span with a parallel run sends the pantograph back to an area search")
