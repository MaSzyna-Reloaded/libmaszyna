extends MaszynaGutTest

## A knob with positions (a brake valve, position_min..position_max over its normalized value) is
## dragged smoothly, stops in the notch of each whole position and leaves it only when jerked.

## FV4a: positions -2..6 over the value 0..1
const POSITION_MIN:float = -2.0
const POSITION_MAX:float = 6.0

var _knob:CabinKnob


func before_each() -> void:
    _knob = CabinKnob.new()
    _knob.position_min = POSITION_MIN
    _knob.position_max = POSITION_MAX
    add_child_autofree(_knob)


func _value_at(position:float) -> float:
    return (position - POSITION_MIN) / (POSITION_MAX - POSITION_MIN)


func _pixels_for(positions:float) -> float:
    return positions / (POSITION_MAX - POSITION_MIN) * CabinKnob.MOUSE_PIXELS_PER_RANGE


func test_drag_moves_smoothly_between_positions() -> void:
    _knob.value = _value_at(0.4)
    _knob.drag(_pixels_for(0.2))
    assert_almost_eq(_knob.value, _value_at(0.6), 0.0001)


func test_drag_stops_in_the_next_notch() -> void:
    _knob.value = _value_at(0.4)
    _knob.drag(_pixels_for(2.0))
    assert_almost_eq(_knob.value, _value_at(1.0), 0.0001)


func test_notch_holds_against_a_small_pull_and_gives_to_a_jerk() -> void:
    _knob.value = _value_at(1.0)
    _knob.drag(CabinKnob.DETENT_BREAKAWAY_PIXELS * 0.5)
    assert_almost_eq(_knob.value, _value_at(1.0), 0.0001)
    _knob.drag(CabinKnob.DETENT_BREAKAWAY_PIXELS * 0.5 + _pixels_for(0.4))
    assert_almost_eq(_knob.value, _value_at(1.4), 0.001)


func test_knob_without_positions_drags_smoothly_without_notches() -> void:
    _knob.position_max = _knob.position_min
    _knob.value = 0.0
    _knob.drag(CabinKnob.MOUSE_PIXELS_PER_RANGE * 0.5)
    assert_almost_eq(_knob.value, 0.5, 0.0001)
