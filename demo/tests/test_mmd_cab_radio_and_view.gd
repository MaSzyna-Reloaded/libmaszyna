extends MaszynaGutTest

## What the cab definition reads for the radio and the driver's view: internaldata:'s radiostop:
## (Train.cpp:10340), driverNangle: yaw then pitch in degrees, driverNpos: and the seat
## driverNsitpos:, which is the position unless given (Train.cpp:10527-10560); cab 1 with
## `cab1model: none` is the vehicle's model (Train.cpp:10611-10615).

const MMD:String = "res://tests/fixtures/dynamic/test/animated_v1/radio_view.mmd"


func _definition(cab:int) -> MmdCabinDefinition:
    return MmdCabinInstancer.parse(
            ProjectSettings.globalize_path(MMD), MmdCabinInstancer.vehicle_parameters("radio", "radio_view", ""), cab, {})


func test_the_radio_stop_alarm_is_read() -> void:
    var sound:MmdSoundSourceDefinition = _definition(1).radio_stop_sound
    assert_not_null(sound)
    assert_eq(sound.sound_main.get_basename(), "test_loop")


func test_the_driver_view_of_a_cab() -> void:
    var definition:MmdCabinDefinition = _definition(1)
    assert_eq(definition.driver_angle, Vector2(165.0, -10.0), "yaw, pitch")
    assert_eq(definition.driver_pos, Vector3(0.0, 2.5, 5.0))
    assert_eq(definition.driver_sitpos, Vector3(0.3, 2.2, 5.1), "the seat")


func test_the_seat_is_the_position_without_sitpos() -> void:
    var definition:MmdCabinDefinition = _definition(2)
    assert_eq(definition.driver_sitpos, Vector3(0.0, 2.5, -5.0))


func test_a_variable_gauge_keeps_its_end_value_and_scale() -> void:
    var descriptor:MmdInstrumentDescriptor = _definition(1).instruments[0]
    assert_eq(descriptor.label, "tachometern")
    assert_eq(descriptor.end_value, 140.0)
    assert_almost_eq(descriptor.end_scale, 0.004457, 0.0000001)


# Train.cpp:10611-10615 - cab 1 without a model is the vehicle's model; another cab is none
func test_cab_1_without_a_model_is_the_vehicle_model() -> void:
    assert_eq(_definition(1).model_relpath, "animated")
    assert_eq(_definition(2).model_relpath, "")


# Train.cpp:12158-12166 - brakes: names the car and its pressure before the gauge's shape
func test_a_gauge_of_a_car_keeps_its_numbers() -> void:
    var descriptor:MmdInstrumentDescriptor = _definition(1).instruments[1]
    assert_eq(descriptor.label, "brakes")
    assert_eq(descriptor.leading_numbers, PackedInt32Array([2, 1]))
    assert_eq(descriptor.submodel_name, "ws-cyl2")
    assert_eq(descriptor.animation_type, "rot")
