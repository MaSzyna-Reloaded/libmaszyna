extends MaszynaGutTest
## A cab is dozens of widgets asking the same vehicle for its state in one frame. The dump is
## composed from every component, so it is built once per physics step and handed out unchanged
## until the next one - nothing but a step can change what it says.

var _rid: RID = RID()
var _controller: VehicleController = null
var _radio: RailVehicleRadio = null


func before_each() -> void:
    _controller = build_vehicle("dump_cache_test")
    _radio = MoverRailVehicleRadio.new()
    _controller.add_component(_radio)
    _rid = _controller.get_rid()
    await wait_idle_frames(2)


func after_each() -> void:
    if _rid.is_valid():
        VehicleServer.vehicle_free(_rid)
        _rid = RID()
    _controller = null
    _radio = null


func test_two_reads_in_one_step_see_the_same_values() -> void:
    var first: Dictionary = VehicleServer.vehicle_dump_state(_rid)
    var second: Dictionary = VehicleServer.vehicle_dump_state(_rid)
    assert_eq(first, second, "the dump is composed once, not per reader")
    assert_true(first.has("velocity"), "and it is the real dump")


## The point of the cache: a change made between two reads of the same step is not visible until
## the step that actually applied it.
func test_a_change_is_not_visible_until_the_next_step() -> void:
    var before: int = int(VehicleServer.vehicle_dump_state(_rid).get("radio_channel", -1))
    _radio.channel_set(before + 1)
    assert_eq(
        int(VehicleServer.vehicle_dump_state(_rid).get("radio_channel", -1)),
        before,
        "still the dump this step was given"
    )
    VehicleServer.stepping_advance(0.016)
    assert_eq(
        int(VehicleServer.vehicle_dump_state(_rid).get("radio_channel", -1)),
        before + 1,
        "the step rebuilt it"
    )


## Regression: a cabin widget reads the state the moment it reports a manipulation, and the whole
## cab is driven that way. Keyed on the step alone the dump still held the values from before the
## command, so pressing a key played its sound and the operation only showed up on the next
## keypress (see FINDINGS.md, 2026-09-23).
func test_a_command_shows_in_the_dump_without_waiting_for_a_step() -> void:
    var before: int = int(VehicleServer.vehicle_dump_state(_rid).get("radio_channel", -1))
    VehicleServer.vehicle_send_command(_rid, "radio_channel_set", before + 1, null)
    assert_eq(
        int(VehicleServer.vehicle_dump_state(_rid).get("radio_channel", -1)),
        before + 1,
        "the dump follows the command, not the next step"
    )


func test_a_freed_vehicle_dumps_nothing() -> void:
    VehicleServer.vehicle_free(_rid)
    var empty: Dictionary = VehicleServer.vehicle_dump_state(_rid)
    _rid = RID()
    assert_eq(empty.size(), 0, "no handle, no dump")


## The public way in: a consumer names the kind, not the implementation, and gets the interface
## that kind promises - whatever the vehicle turns out to be built from.
func test_a_component_is_reached_by_its_kind() -> void:
    var heating: MoverRailVehicleHeating = MoverRailVehicleHeating.new()
    _controller.add_component(heating)
    await wait_idle_frames(2)

    var found: VehicleComponent = VehicleServer.vehicle_component_get(
        _rid, VehicleComponentType.COMPONENT_HEATING
    )
    assert_same(found, heating, "the vehicle answers with its heating")
    assert_true(found is RailVehicleHeating, "and it is the interface that kind promises")
    assert_null(
        VehicleServer.vehicle_component_get(_rid, VehicleComponentType.COMPONENT_DOORS),
        "a kind this vehicle has not got answers with nothing"
    )
