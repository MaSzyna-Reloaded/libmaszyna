extends MaszynaGutTest

## CabinSystem is the one place a cab talks to the vehicle servers from, and every element of a cab
## asks it within one frame. It keeps no dump of its own: VehicleServer builds one per step and
## per command, and that one must not outlive a command - a control reports its manipulation and
## reads the result in the very same frame (see `FINDINGS.md`, 2026-09-23).

var _controller:VehicleController = null
var _vehicle:RID


func before_each() -> void:
    _controller = build_vehicle("cabin_state_test")
    _controller.add_component(MoverRailVehicleRadio.new())
    _vehicle = _controller.get_rid()
    await wait_idle_frames(2)


func after_each() -> void:
    _controller = null


func test_two_elements_asking_in_one_frame_are_given_the_same_dump() -> void:
    var first:Dictionary = CabinSystem.vehicle_state(_vehicle)
    var second:Dictionary = CabinSystem.vehicle_state(_vehicle)
    assert_eq(first, second, "the dump is built once a step, not once an element")
    assert_true(first.has("velocity"), "and it is the real dump")


func test_a_command_shows_through_without_waiting_for_the_next_frame() -> void:
    var before:int = int(CabinSystem.vehicle_state(_vehicle).get("radio_channel", -1))
    _controller.send_command("radio_channel_set", before + 1)
    assert_eq(
            int(CabinSystem.vehicle_state(_vehicle).get("radio_channel", -1)),
            before + 1,
            "a control reads the result of its own manipulation in the frame it made it")


func test_a_vehicle_that_left_is_not_still_being_described() -> void:
    assert_true(CabinSystem.vehicle_state(_vehicle).has("velocity"))
    VehicleServer.vehicle_free(_vehicle)
    assert_eq(
            CabinSystem.vehicle_state(_vehicle).size(), 0,
            "the dump of a vehicle that is gone is not handed out for the rest of the frame")
