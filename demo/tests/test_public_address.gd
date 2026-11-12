extends MaszynaGutTest

## A train's PA system: an announcement goes from the driver's vehicle along the train's couplings
## (RailVehicleServer.vehicle_send_line_command()) to every vehicle with a PA system, which queues its
## chime, its own announcement and the stop's name, and plays them one by one while it has its low
## voltage (TDynamicObject::announce(), DynObj.cpp:4790-4813, 7965-8020)

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const ANNOUNCEMENTS_MMD:String = "res://tests/fixtures/test_announcements.mmd"
const LINE_COMMAND:StringName = &"line_test"
const BATTERY_VOLTAGE:float = 110.0
## A second of silence, enough for an announcement to be playing when asked
const MIX_RATE:int = 22050
## Enough frames for a built vehicle to take its controller
const SETTLE_FRAMES:int = 4
const STOP_FILE:String = "res://tests/fixtures/stop.wav"
const DESTINATION_FILE:String = "res://tests/fixtures/destination.wav"

var _received:Array = []


func _record(_p1:Variant, _p2:Variant, vehicle:RID) -> void:
    _received.append(vehicle)


## Three vehicles, the middle one turned round: front - rear|rear - front|front - rear
func test_a_line_command_goes_along_the_couplings_to_those_carrying_it() -> void:
    var first:RID = build_vehicle("LineFirst", SM42).get_rid()
    var second:RID = build_vehicle("LineSecond", SM42).get_rid()
    var third:RID = build_vehicle("LineThird", SM42).get_rid()
    await wait_idle_frames(SETTLE_FRAMES)
    RailVehicleServer.coupler_attach(first, RailVehicleController.COUPLER_END_REAR, second,
            RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLING_FLAG_COUPLER)
    RailVehicleServer.coupler_attach(second, RailVehicleController.COUPLER_END_FRONT, third,
            RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)
    for vehicle:RID in [second, third]:
        VehicleServer.vehicle_get_controller(vehicle).register_command(LINE_COMMAND, _record.bind(vehicle))
    watch_signals(RailVehicleServer)

    RailVehicleServer.vehicle_send_line_command(
            first, RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLING_FLAG_COUPLER, LINE_COMMAND)
    assert_eq(_received, [second, third], "along the line, whichever way each stands")
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_line_command_received",
            [third, first, RailVehicleController.COUPLER_END_FRONT, LINE_COMMAND])

    _received.clear()
    RailVehicleServer.vehicle_send_line_command(
            first, RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLING_FLAG_CONTROL, LINE_COMMAND)
    assert_eq(_received, [], "no control line: it stops at the first coupling")

    VehicleServer.vehicle_get_controller(second).unregister_command(LINE_COMMAND)
    RailVehicleServer.vehicle_send_line_command(
            first, RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLING_FLAG_COUPLER, LINE_COMMAND)
    assert_eq(_received, [third], "a vehicle without it passes it on")


func test_the_announcements_of_the_mmd_are_read() -> void:
    var context := MmdImportContext.new()
    context.base_dir = ANNOUNCEMENTS_MMD.get_base_dir()
    var announcements:Dictionary = MmdSoundSourceParser.parse_announcements(ANNOUNCEMENTS_MMD, context)

    assert_eq(announcements.get("near_stop"), "sip/stacja")
    assert_has(["sip/stacja_a", "sip/stacja_b"], announcements.get("stop"), "one of a random set")
    assert_eq(announcements.get("next_stop"), "sip/nastepna_stacja")
    assert_eq(announcements.get("chime"), "sip/jingiel")
    assert_false(announcements.has("destination"))
    assert_false(announcements.has("unknown"), "a key it does not know is read past")



## The PA's loudness follows how far the listener is (MaszynaVehicleSoundServer sets it), so its bank
## declares that parameter - a player takes no parameter its bank does not declare
func test_the_pa_bank_declares_the_listeners_distance() -> void:
    var train:VehicleController = build_vehicle("PublicAddressBankTest", SM42)
    var parent:Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var diagnostics:Array[Dictionary] = []
    MmdSoundBankInstancer.build_into(
            parent, train.get_rid(), ProjectSettings.globalize_path(ANNOUNCEMENTS_MMD), {}, {}, diagnostics)

    var player:SfxPlayer3D = parent.get_node("PublicAddressSfxPlayer3D") as SfxPlayer3D
    var declared:Array[StringName] = []
    for parameter:SfxParameter in player.bank.parameters:
        declared.append(parameter.name)
    assert_has(declared, ScenerySoundServer.DISTANCE_PARAMETER)

func _event(event_name:String) -> SfxEvent:
    var stream:AudioStreamWAV = AudioStreamWAV.new()
    stream.mix_rate = MIX_RATE
    stream.data = PackedByteArray()
    stream.data.resize(MIX_RATE)
    var clip:SfxClip = SfxClip.new()
    clip.stream = stream
    var event:SfxEvent = SfxEvent.new()
    event.name = StringName(event_name)
    var clips:Array[SfxClip] = [clip]
    event.clips = clips
    return event


func test_a_powered_vehicle_plays_the_chime_the_announcement_and_the_names() -> void:
    var model:RailVehicleController = MaszynaMoverRailVehicleController.new()
    model.add_component(build_power_supply(BATTERY_VOLTAGE))
    var train:VehicleController = build_vehicle("PublicAddressTest", model)
    var vehicle:RID = train.get_rid()
    var bank:SfxBank = SfxBank.new()
    var events:Array[SfxEvent] = [_event("chime"), _event("next_stop"), _event("destination")]
    bank.events = events
    var player:SfxPlayer3D = SfxPlayer3D.new()
    player.bank = bank
    add_child_autoqfree(player)
    MaszynaVehicleSoundServer.announcements_register(
            player.get_instance_id(), vehicle, _event, ScenerySoundServer.DISTANCE_PARAMETER)
    train.send_command("battery", true)
    await step(1)

    VehicleServer.vehicle_send_command(vehicle, MaszynaAIDriver.ANNOUNCE_COMMAND,
            MaszynaVehicleSoundServer.ANNOUNCEMENT_NEXT, PackedStringArray([STOP_FILE, DESTINATION_FILE]))
    await step(1)
    assert_true(player.is_playing(&"chime"), "the chime first")
    var names:Array[StringName] = []
    for event:SfxEvent in bank.events:
        names.append(event.name)
    assert_has(names, StringName(STOP_FILE), "the stop's name made")
    assert_has(names, StringName(DESTINATION_FILE), "and the destination's, after the next stop's")

    train.send_command("battery", false)
    await step(1)
    assert_false(player.is_playing(&"chime"), "no low voltage: silent")
