extends MaszynaGutTest

## How loud a scenery sound is with the camera at a distance (MaszynaScenerySounds._loudness()): a
## sound placed by its range by OpenAL's inverse distance, clamped - rolloff 1.75 from a sixteenth of
## the range (audiorenderer.cpp:178-183, 311) - and any sound faded out from its range to 1.25 of it
## (audiorenderer.cpp:184-199)

const RANGE:float = 160.0
## The inverse distance at the range: 1 / (1 + 1.75 * 15)
const GAIN_AT_RANGE:float = 1.0 / 27.25
const TOLERANCE:float = 0.001


func _gain(range_max:float, distance:float) -> float:
    var loudness:SfxParameterModulation = MaszynaScenerySounds._loudness(range_max)
    return loudness.curve.sample(clampf(distance, loudness.min_domain, loudness.max_domain))


func test_a_placed_sound_falls_off_by_the_inverse_distance_and_fades_past_its_range() -> void:
    assert_almost_eq(_gain(RANGE, 0.0), 1.0, TOLERANCE, "full within a sixteenth of the range")
    assert_almost_eq(_gain(RANGE, RANGE / 16.0), 1.0, TOLERANCE)
    assert_almost_eq(_gain(RANGE, RANGE), GAIN_AT_RANGE, TOLERANCE, "at the range")
    assert_almost_eq(_gain(RANGE, RANGE * 1.25), 0.0, TOLERANCE, "silent at 1.25 of the range")
    assert_almost_eq(_gain(RANGE, RANGE * 2.0), 0.0, TOLERANCE, "and past it")


func test_an_ambient_sound_is_flat_within_its_range_and_fades_past_it() -> void:
    assert_almost_eq(_gain(-RANGE, 0.0), 1.0, TOLERANCE)
    assert_almost_eq(_gain(-RANGE, RANGE), 1.0, TOLERANCE, "on the listener: no distance falloff")
    assert_almost_eq(_gain(-RANGE, RANGE * 1.25), 0.0, TOLERANCE, "silent at 1.25 of the range")
