extends MaszynaGutTest

const LuaImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_lua_importer.gd")
const FIXTURES:String = "res://tests/fixtures/lua"
const OUTPUT:StringName = &"lua_test_output"
## SimulationServer::MAX_FRAME_DELTA
const MAX_FRAME_DELTA:float = 0.25
## Simulated seconds an event runs past its delay at most: the frame it falls due in and the frame
## that runs it, each counting MAX_FRAME_DELTA at most
const EVENT_MARGIN:float = 2.0 * MAX_FRAME_DELTA
## maszyna.sim.after()'s delay in the timer's script [s]
const AFTER_DELAY:float = 0.05
## Enough frames for a queued event to have run, were it going to
const SETTLE_FRAMES:int = 5
## The track the player's vehicle stands on
const PLAYER_TRACK:String = "lua_player_test"
const PLAYER_TRACK_LENGTH:float = 200.0
## The power supply's battery [V] - the `battery` command the script sends belongs to it
const BATTERY_VOLTAGE:float = 110.0


class RecordingCabin extends ScenarioScriptCabinImplementation:
    var acts:Array = []

    func _act(cabin:RID, control_id:StringName, action:StringName, value:Variant) -> Variant:
        acts.append([cabin, control_id, action, value])
        return true


class RecordingDriver extends DriverImplementation:
    var commands:Array = []

    func _handle_command(_driver:RID, command:String, value1:float, value2:float, _position:Vector3) -> void:
        commands.append([command, value1, value2])


class RecordingSystem extends SignallingImplementation:
    var events:Array = []

    func _handle_event(_system:RID, event:StringName, arguments:Dictionary) -> void:
        events.append([event, arguments])


var _context:RID
var _output:RID


func before_each() -> void:
    _context = ScenarioScriptServer.context_create(FIXTURES)
    _output = ScenarioEventServer.memory_create()
    ScenarioEventServer.memory_set_name(_output, OUTPUT)


func after_each() -> void:
    ScenarioScriptServer.context_free(_context)
    ScenarioEventServer.memory_free(_output)


func test_a_file_runs_and_requires_a_lowercase_fallback_module_of_its_directory() -> void:
    assert_true(ScenarioScriptServer.context_run_file(_context, "hello.lua"))

    assert_eq(ScenarioEventServer.memory_get_text(_output), "hello")
    assert_eq(ScenarioEventServer.memory_get_value2(_output), 2.0)


func test_an_error_is_reported_and_fails_the_run() -> void:
    watch_signals(ScenarioScriptServer)

    assert_false(ScenarioScriptServer.context_run_file(_context, "syntax_error.lua"))
    assert_signal_emitted(ScenarioScriptServer, "script_error")
    assert_false(ScenarioScriptServer.context_run_file(_context, "missing.lua"))


func test_check_compiles_without_running() -> void:
    assert_eq(ScenarioScriptServer.context_check_source(_context, &"editor", "maszyna.memory.write(nil)"), "")
    var error:String = ScenarioScriptServer.context_check_source(_context, &"editor", "\nlocal = 1")
    assert_string_contains(error, "editor:2:")


func test_the_sandbox_leaves_out_what_reaches_past_the_simulation() -> void:
    var source:String = (
        "assert(io == nil and os == nil and debug == nil and package == nil)\n"
        + "assert(dofile == nil and loadfile == nil and string.dump == nil)\n"
        + "assert(load('return 1')() == 1)\n"
        + "assert(load('\\27Lua') == nil, 'no binary chunks')\n"
        + "local roll = math.random(1, 6)\n"
        + "assert(roll >= 1 and roll <= 6)"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"sandbox", source))


func test_files_outside_the_directory_are_refused() -> void:
    assert_false(ScenarioScriptServer.context_run_file(_context, "../test_scenario_script_server.gd"))
    assert_false(ScenarioScriptServer.context_apply_source(_context, &"escape", "require('..tests')"))


func test_an_endless_loop_fails_instead_of_hanging() -> void:
    assert_false(ScenarioScriptServer.context_apply_source(_context, &"loop", "while true do end"))
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"after_loop", "local x = 1"))


func test_an_event_the_script_created_runs_and_goes_with_the_context() -> void:
    var source:String = (
        "local output = maszyna.memory.find('lua_test_output')\n"
        + "local e = maszyna.event.create{ name = 'lua_test_event', run = function(event, activator)\n"
        + "    maszyna.memory.write(output, maszyna.event.name(event), activator == nil and 1 or 0, 0)\n"
        + "end }\n"
        + "maszyna.event.queue(e)"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"events", source))
    var event:RID = ScenarioEventServer.event_get_rid_by_name(&"lua_test_event")
    assert_true(event.is_valid())
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_text(_output) == "lua_test_event", EVENT_MARGIN,
            "the script's event run"):
        return

    assert_eq(ScenarioEventServer.memory_get_value1(_output), 1.0, "nothing activated it")
    ScenarioScriptServer.context_free(_context)
    assert_false(ScenarioEventServer.event_get_rid_by_name(&"lua_test_event").is_valid())
    _context = ScenarioScriptServer.context_create(FIXTURES)


func test_after_runs_once_on_the_simulation_clock() -> void:
    var source:String = (
        "local output = maszyna.memory.find('lua_test_output')\n"
        + "maszyna.sim.after(%s, function()\n" % AFTER_DELAY
        + "    local text, count = maszyna.memory.read(output)\n"
        + "    maszyna.memory.write(output, 'after', count + 1, 0)\n"
        + "end)"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"timer", source))
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_text(_output) == "after", AFTER_DELAY + EVENT_MARGIN,
            "the timer's function run"):
        return
    await wait_idle_frames(SETTLE_FRAMES)

    assert_eq(ScenarioEventServer.memory_get_value1(_output), 1.0)


func test_applying_a_unit_again_frees_what_it_made_before() -> void:
    assert_true(ScenarioScriptServer.context_apply_source(
            _context, &"unit", "maszyna.event.create{ name = 'lua_test_first' }"))
    assert_true(ScenarioScriptServer.context_apply_source(
            _context, &"other", "maszyna.event.create{ name = 'lua_test_other' }"))
    assert_true(ScenarioScriptServer.context_apply_source(
            _context, &"unit", "maszyna.event.create{ name = 'lua_test_second' }"))

    assert_false(ScenarioEventServer.event_get_rid_by_name(&"lua_test_first").is_valid())
    assert_true(ScenarioEventServer.event_get_rid_by_name(&"lua_test_second").is_valid())
    assert_true(ScenarioEventServer.event_get_rid_by_name(&"lua_test_other").is_valid(), "another unit's stays")


func test_a_subscription_runs_through_the_queue_until_cancelled() -> void:
    var track:RID = TrackServer.track_create()
    var section:RID = TrackServer.isolated_create()
    TrackServer.isolated_set_name(section, &"lua_test_section")
    TrackServer.isolated_add_track(section, track)
    var vehicle:RID = ScenarioEventServer.memory_create() # stands in for a vehicle
    var source:String = (
        "local output = maszyna.memory.find('lua_test_output')\n"
        + "local section = maszyna.track.isolated_find('lua_test_section')\n"
        + "subscription = maszyna.track.on_isolated_occupied(section, function(vehicle)\n"
        + "    local text, count = maszyna.memory.read(output)\n"
        + "    maszyna.memory.write(output, tostring(vehicle ~= nil), count + 1, 0)\n"
        + "end)"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"subscription", source))

    TrackServer.track_vehicle_entered(track, vehicle)
    assert_eq(ScenarioEventServer.memory_get_text(_output), "", "never inside the server that reported it")
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_text(_output) == "true", EVENT_MARGIN,
            "the subscription's function run"):
        return
    TrackServer.track_vehicle_left(track, vehicle)
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"cancel", "assert(maszyna.cancel(subscription))"))
    TrackServer.track_vehicle_entered(track, vehicle)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_eq(ScenarioEventServer.memory_get_value1(_output), 1.0, "a cancelled subscription is not called")
    TrackServer.track_vehicle_left(track, vehicle)
    TrackServer.isolated_free(section)
    TrackServer.track_free(track)
    ScenarioEventServer.memory_free(vehicle)


func test_a_vehicle_takes_commands_and_reports_them() -> void:
    var controller:VehicleController = build_vehicle(
            "LuaTestTrain", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    controller.add_component(build_power_supply(BATTERY_VOLTAGE))
    var recording:RecordingDriver = RecordingDriver.new()
    attach_driver_implementation(get_vehicle_driver(controller.get_rid()), recording)
    var source:String = (
        "local output = maszyna.memory.find('lua_test_output')\n"
        + "local v = maszyna.vehicle.find('LuaTestTrain')\n"
        + "assert(v == maszyna.vehicle.find_all('LuaTestTrain')[1])\n"
        + "assert(maszyna.vehicle.name(v) == 'LuaTestTrain')\n"
        + "maszyna.vehicle.on_command_received(v, function(command, p1)\n"
        + "    maszyna.memory.write(output, command, p1 and 1 or 0, 0)\n"
        + "end)\n"
        + "maszyna.vehicle.send_command(v, 'battery', true)\n"
        + "assert(maszyna.driver.send_command(v, 'SetVelocity', 40, 30))"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"vehicle", source))
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_text(_output) == "battery", EVENT_MARGIN,
            "the command reported to the script"):
        return

    assert_eq(ScenarioEventServer.memory_get_value1(_output), 1.0)
    assert_eq(recording.commands, [["SetVelocity", 40.0, 30.0]])


func test_a_signalling_system_gets_the_script_event() -> void:
    var system:RID = SignallingServer.system_create()
    var recording:RecordingSystem = RecordingSystem.new()
    SignallingServer.system_attach_implementation(system, recording)
    SignallingServer.system_set_name(system, &"lua_test_system")
    var source:String = (
        "local system = maszyna.signal.find_system('lua_test_system')\n"
        + "maszyna.signal.send_event(system, 'lights', { aspect = 'S1' })"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"signal", source))

    assert_eq(recording.events.size(), 1)
    assert_eq(recording.events[0][0], &"lights")
    assert_eq(recording.events[0][1]["aspect"], "S1")
    SignallingServer.system_free(system)


func test_the_cabs_are_reached_through_the_implementation() -> void:
    var cabin:RecordingCabin = RecordingCabin.new()
    ScenarioScriptServer.context_attach_cabin_implementation(_context, cabin)
    var controller:VehicleController = build_vehicle(
            "LuaTestCab", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_REAR)
    var source:String = (
        "local v = maszyna.vehicle.find('LuaTestCab')\n"
        + "local cabin = maszyna.cabin.driver_cabin(v)\n"
        + "assert(cabin == maszyna.cabin.rear_cabin(v))\n"
        + "assert(not (cabin == maszyna.cabin.front_cabin(v)))\n"
        + "assert(maszyna.cabin.machine_room(v) == nil)\n"
        + "assert(maszyna.cabin.act(cabin, 'main_switch', 'toggle') == true)"
    )
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"cabin", source))

    var rear_cabin:RID = RailVehicleServer.vehicle_get_rear_cabin(controller.get_rid())
    assert_eq(cabin.acts, [[rear_cabin, &"main_switch", &"toggle", null]])


## maszyna.player, maszyna.camera and maszyna.hud reach PlayerServer, PlayerCameraServer and
## HUDServer by the scripts' vehicle handles
func test_the_player_its_view_and_the_hud_are_reached_by_vehicle_handles() -> void:
    var track:RID = build_track(PLAYER_TRACK, PLAYER_TRACK_LENGTH)
    var node:RailVehicle3D = build_rail_vehicle("LuaPlayerTrain", PLAYER_TRACK, PLAYER_TRACK_LENGTH / 2.0)
    await wait_idle_frames(SETTLE_FRAMES)
    var source:String = (
        "local v = maszyna.vehicle.find('LuaPlayerTrain')\n"
        + "assert(maszyna.camera.mode() == 'free')\n"
        + "maszyna.player.take_over(v)\n"
        + "assert(maszyna.player.vehicle() == v)\n"
        + "assert(maszyna.camera.mode() == 'cabin')\n"
        + "maszyna.camera.toggle_cabin()\n"
        + "assert(maszyna.camera.mode() == 'free')\n"
        + "maszyna.camera.set_follow_view('bogie')\n"
        + "assert(maszyna.camera.follow_view() == 'bogie')\n"
        + "maszyna.camera.set_target(v)\n"
        + "maszyna.camera.set_mode('follow')\n"
        + "assert(maszyna.camera.target() == v and maszyna.camera.mode() == 'follow')\n"
        + "maszyna.camera.show_vehicle(v)\n"
        + "assert(maszyna.camera.mode() == 'free')\n"
        + "maszyna.hud.show('lua_test_panel')\n"
        + "assert(maszyna.hud.is_visible('lua_test_panel'))\n"
        + "maszyna.hud.hide('lua_test_panel')\n"
        + "maszyna.hud.open_card(v)\n"
        + "assert(maszyna.hud.card() == v)\n"
        + "maszyna.hud.close_card()\n"
        + "assert(maszyna.hud.card() == nil)\n"
        + "maszyna.player.leave()\n"
        + "assert(maszyna.player.vehicle() == nil)\n"
        + "maszyna.player.enter(v)\n"
        + "assert(maszyna.player.vehicle() == v)\n"
        + "maszyna.player.leave()"
    )

    assert_true(ScenarioScriptServer.context_apply_source(_context, &"player", source))

    PlayerCameraServer.camera_set_target(RID())
    PlayerCameraServer.camera_set_follow_view(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    free_rail_vehicle(node)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


func test_the_original_api_runs_an_onstart_event() -> void:
    ScenarioEventServer.memory_set_values(_output, "", 0.0, 5.0)
    assert_true(ScenarioScriptServer.context_run_file(_context, "legacy_events.lua"))
    if not await wait_simulated_until(
            func() -> bool: return ScenarioEventServer.memory_get_text(_output) == "legacy:lua_test_legacy_onstart",
            EVENT_MARGIN, "the onstart event run"):
        return

    assert_eq(ScenarioEventServer.memory_get_value1(_output), 1.0)
    assert_eq(ScenarioEventServer.memory_get_value2(_output), 0.0, "memcell_update writes every field")


func test_the_lua_keyword_records_the_script() -> void:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize("lua scenario.lua".to_utf8_buffer())
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    parser.register_handler("lua", func(p:MaszynaParser) -> Array: return LuaImporter.new().import(p, context))
    parser.parse()

    var expected:Array[String] = ["scenario.lua"]
    assert_eq(context.scripts, expected)
