extends MaszynaGutTest

## The radio ether (RadioServer): what a radio transmits every other radio within reach hears,
## whatever it is tuned to; a vehicle's radio is one for as long as the vehicle has it - it sends the
## cab's calls and Radio-Stop only when powered, and hears Radio-Stop; the scenery transmits from its
## own radio
## (SceneryVirtualRadio).

const LOCOMOTIVE_PATH:String = "res://tests/fixtures/dynamic/pkp/ep09_v1/104e-039.fiz"
const TRACK:String = "TestRadioServerTrack"
const TRACK_LENGTH:float = 200.0
const OFFSET:float = 50.0
const CHANNEL:int = 3
const REACH:float = 100.0
## Farther than any reach used here and than Radio-Stop's
const FAR:Vector3 = Vector3(5000.0, 0.0, 0.0)

var _heard:Array = []
var _radios:Array[RID] = []
var _vehicle:RailVehicle3D = null
var _vehicle_rid:RID
var _track:RID


func after_each() -> void:
    for radio:RID in _radios:
        RadioServer.radio_free(radio)
    _radios.clear()
    _heard.clear()
    if _vehicle:
        free_rail_vehicle(_vehicle)
        _vehicle = null
        TrackServer.track_free(_track)
        TrackServer.topology_rebuild()


## A radio standing at `position`, keeping what it hears
func _listening_radio(position:Vector3) -> RID:
    var radio:RID = RadioServer.radio_create()
    RadioServer.radio_set_position(radio, position)
    RadioServer.radio_set_receive_callback(radio,
            func(kind:int, channel:int, payload:Variant, _from:Vector3) -> void: _heard.append([kind, channel, payload]))
    _radios.append(radio)
    return radio


## The locomotive on a track, its battery on - driven or not
func _build_locomotive(driver:MaszynaVehicleData.DriverType) -> void:
    _track = build_track(TRACK, TRACK_LENGTH)
    var physics_node:VehiclePhysicsNode = build_vehicle_node("TestRadioServer",
            FizVehicleBuilder.build_description_at(LOCOMOTIVE_PATH), 0.0, driver)
    _vehicle = RailVehicle3D.new()
    _vehicle.start_track_name = TRACK
    _vehicle.start_track_offset = OFFSET
    _vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(_vehicle)
    _vehicle_rid = physics_node.get_vehicle_rid()
    await step(3)


func _power_radio() -> void:
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    VehicleServer.vehicle_send_command(_vehicle_rid, "radio", true)
    await step(2)


func _radio_stop_active() -> bool:
    return VehicleServer.vehicle_get_controller(_vehicle_rid).get_state()["radio_stop_active"]


func test_a_transmission_is_heard_within_its_reach_only() -> void:
    var sender:RID = _listening_radio(Vector3.ZERO)
    _listening_radio(Vector3(REACH * 0.5, 0.0, 0.0))
    _listening_radio(Vector3(REACH * 2.0, 0.0, 0.0))
    RadioServer.radio_transmit(sender, CHANNEL, REACH, RadioServer.KIND_CALL, RailVehicleRadio.RADIO_CALL1)
    assert_eq(_heard, [[RadioServer.KIND_CALL, CHANNEL, RailVehicleRadio.RADIO_CALL1]],
            "the near radio, not the far one, nor the sender itself")


func test_no_reach_is_heard_everywhere_whatever_the_channel() -> void:
    var sender:RID = _listening_radio(Vector3.ZERO)
    _listening_radio(FAR)
    RadioServer.radio_transmit(sender, CHANNEL, 0.0, RadioServer.KIND_CALL, RailVehicleRadio.RADIO_CALL3)
    assert_eq(_heard.size(), 1, "heard whatever the receiver is tuned to: it compares the channel itself")


func test_a_freed_radio_hears_nothing() -> void:
    var listener:RID = _listening_radio(Vector3.ZERO)
    RadioServer.radio_free(listener)
    _radios.erase(listener)
    SceneryVirtualRadio.new().transmit(Vector3.ZERO, CHANNEL, 0.0, RadioServer.KIND_CALL, RailVehicleRadio.RADIO_CALL1)
    assert_eq(_heard, [])


func test_the_scenery_radio_transmits_from_where_it_is_told() -> void:
    _listening_radio(FAR)
    var scenery_radio:SceneryVirtualRadio = SceneryVirtualRadio.new()
    scenery_radio.transmit(Vector3.ZERO, CHANNEL, REACH, RadioServer.KIND_CALL, RailVehicleRadio.RADIO_CALL1)
    assert_eq(_heard.size(), 0, "too far from where it was said")
    scenery_radio.transmit(FAR, CHANNEL, REACH, RadioServer.KIND_CALL, RailVehicleRadio.RADIO_CALL1)
    assert_eq(_heard.size(), 1)


func test_a_vehicle_calls_on_its_channel_only_while_its_radio_is_powered() -> void:
    await _build_locomotive(MaszynaVehicleData.DriverType.DRIVER_HEAD)
    _listening_radio(FAR)
    VehicleServer.vehicle_send_command(_vehicle_rid, "radio_call1", true)
    assert_eq(_heard, [], "an unpowered radio sends nothing")

    await _power_radio()
    VehicleServer.vehicle_send_command(_vehicle_rid, "radio_channel_set", CHANNEL)
    VehicleServer.vehicle_send_command(_vehicle_rid, "radio_call1", true)
    assert_eq(_heard, [[RadioServer.KIND_CALL, CHANNEL, RailVehicleRadio.RADIO_CALL1]])

    VehicleServer.vehicle_send_command(_vehicle_rid, "radio_channel_set", RailVehicleRadio.CHANNEL_NO_CALLS)
    VehicleServer.vehicle_send_command(_vehicle_rid, "radio_call3", true)
    assert_eq(_heard.size(), 1, "no calls on the channel without them")


func test_radio_stop_brakes_a_driven_vehicle_in_reach() -> void:
    await _build_locomotive(MaszynaVehicleData.DriverType.DRIVER_HEAD)
    await _power_radio()
    watch_signals(RailVehicleServer)
    var scenery_radio:SceneryVirtualRadio = SceneryVirtualRadio.new()
    scenery_radio.transmit(VehicleServer.vehicle_get_position(_vehicle_rid) + FAR, RadioServer.CHANNEL_ALL,
            RadioServer.RADIO_STOP_REACH, RadioServer.KIND_RADIO_STOP)
    assert_false(_radio_stop_active(), "beyond Radio-Stop's reach")
    scenery_radio.transmit(VehicleServer.vehicle_get_position(_vehicle_rid), RadioServer.CHANNEL_ALL,
            RadioServer.RADIO_STOP_REACH, RadioServer.KIND_RADIO_STOP)
    assert_true(_radio_stop_active())
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_emergency_signal_received", [_vehicle_rid])


## The original's condition, kept: the brakes need a crew in the vehicle to act (DynObj.cpp:7229)
func test_radio_stop_leaves_a_vehicle_nobody_drives() -> void:
    await _build_locomotive(MaszynaVehicleData.DriverType.DRIVER_NOBODY)
    await _power_radio()
    SceneryVirtualRadio.new().transmit(VehicleServer.vehicle_get_position(_vehicle_rid), RadioServer.CHANNEL_ALL,
            RadioServer.RADIO_STOP_REACH, RadioServer.KIND_RADIO_STOP)
    assert_false(_radio_stop_active())


func test_radio_stop_sent_from_a_vehicle_brakes_itself_too() -> void:
    await _build_locomotive(MaszynaVehicleData.DriverType.DRIVER_HEAD)
    await _power_radio()
    _listening_radio(VehicleServer.vehicle_get_position(_vehicle_rid))
    VehicleServer.vehicle_send_command(_vehicle_rid, "radio_stop", true)
    assert_true(_radio_stop_active(), "the sender is in its own reach (scene.cpp:1269)")
    assert_eq(_heard, [[RadioServer.KIND_RADIO_STOP, RadioServer.CHANNEL_ALL, null]])
