extends MaszynaGutTest

## A vehicle's name belongs to the server that owns its handle: whoever knows a vehicle only by
## name - a scenery event, the console, a `.scn` command - asks the server for the handle, and
## everything that already holds the vehicle never comes through here at all. TrackServer has the
## same pair for tracks, for the same reason.

var _controller:VehicleController = null


func before_each() -> void:
    _controller = build_vehicle("name_registry_test")
    await wait_idle_frames(2)


func after_each() -> void:
    _controller = null


func test_a_vehicle_is_found_by_the_name_the_scenery_gave_it() -> void:
    var rid:RID = _controller.get_rid()
    assert_eq(
            VehicleServer.vehicle_get_name(rid), "name_registry_test",
            "the server knows what the scenery called this vehicle")
    assert_eq(
            VehicleServer.vehicle_get_rid_by_name("name_registry_test"), rid,
            "and hands back its handle when asked by that name")


func test_a_name_nothing_carries_answers_with_no_handle() -> void:
    assert_false(
            RID(VehicleServer.vehicle_get_rid_by_name("no_such_train")).is_valid(),
            "a name no vehicle carries is not a handle")


func test_a_freed_vehicle_takes_its_name_with_it() -> void:
    var rid:RID = _controller.get_rid()
    VehicleServer.vehicle_free(rid)
    assert_false(
            RID(VehicleServer.vehicle_get_rid_by_name("name_registry_test")).is_valid(),
            "the name of a vehicle that is gone leads nowhere")


## Names.h:27-40 basic_table::insert - a repeated name goes to the later vehicle, and neither loses
## anything but the lookup: both keep their handle and their commands.
func test_a_repeated_name_goes_to_the_later_vehicle_and_both_take_commands() -> void:
    var later:VehicleController = build_vehicle("name_registry_test")
    await wait_idle_frames(2)
    assert_eq(
            VehicleServer.vehicle_get_rid_by_name("name_registry_test"), later.get_rid(),
            "the name leads to the vehicle that took it last")
    for vehicle:VehicleController in [_controller, later]:
        assert_true(_takes_a_command(vehicle), "a vehicle sharing its name still takes commands by its handle")


func test_freeing_the_earlier_of_two_same_named_vehicles_keeps_the_name_on_the_later() -> void:
    var later:VehicleController = build_vehicle("name_registry_test")
    await wait_idle_frames(2)
    VehicleServer.vehicle_free(_controller.get_rid())
    assert_eq(
            VehicleServer.vehicle_get_rid_by_name("name_registry_test"), later.get_rid(),
            "the earlier vehicle does not take the later one's name with it")


## Names.h:29 - "" and "none" are no names: nothing is found by them, and the vehicle still works.
func test_a_vehicle_named_nothing_or_none_is_not_looked_up_but_takes_commands() -> void:
    for name:String in ["", "none"]:
        var vehicle:VehicleController = build_vehicle(name)
        await wait_idle_frames(2)
        assert_false(
                RID(VehicleServer.vehicle_get_rid_by_name(name)).is_valid(),
                "\"%s\" is not a name to look a vehicle up by" % name)
        assert_true(_takes_a_command(vehicle), "a vehicle without a name still takes commands by its handle")


## Whether a command sent by the vehicle's handle changes its state - the radio channel, which
## needs no power configured
func _takes_a_command(vehicle:VehicleController) -> bool:
    vehicle.add_component(MoverRailVehicleRadio.new())
    var before:int = int(VehicleServer.vehicle_dump_state(vehicle.get_rid()).get("radio_channel", -1))
    VehicleServer.vehicle_send_command(vehicle.get_rid(), "radio_channel_set", before + 1)
    return int(VehicleServer.vehicle_dump_state(vehicle.get_rid()).get("radio_channel", -1)) == before + 1
