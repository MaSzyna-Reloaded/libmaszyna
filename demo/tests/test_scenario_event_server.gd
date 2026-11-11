extends MaszynaGutTest

const EventImporter = preload("res://addons/libmaszyna/scenery/maszyna_event_importer.gd")
const NodeImporter = preload("res://addons/libmaszyna/scenery/maszyna_node_importer.gd")
const IsolatedImporter = preload("res://addons/libmaszyna/scenery/maszyna_isolated_importer.gd")
const ConfigImporter = preload("res://addons/libmaszyna/scenery/maszyna_config_importer.gd")
## The scenery animation's turn [degrees] and its speed [degrees a simulated second]
const QUARTER_TURN:float = 90.0
const TURN_SPEED:float = 900.0
## Longer than any test runs, so the event stays queued
const NEVER:float = 3600.0
## An event another one queues - or one that queues itself again - runs a step later: a pass of
## the queue does not run what it queued
const CHAINED_EVENT_SECONDS:float = 2.0 * TICK
## Fast enough for one frame to pass the cap
const FAST_SPEED:float = 1000.0
## SimulationServer::MAX_FRAME_DELTA
const MAX_FRAME_DELTA:float = 0.25
## The delay of an event queued to run after two of no delay [s]
const LATE_DELAY:float = 0.2
## The width the scenery gives its tracks here [m]
const SCENERY_TRACK_WIDTH:float = 1.435
## `departuredelay`: an event of this delay [s] and this departure delay [s], queued by a train that
## departs this many seconds from now; run times compared within this [s]
const EVENT_DELAY:float = 5.0
const DEPARTURE_DELAY:float = -30.0
const UNTIL_DEPARTURE:float = 600.0
const RUN_TIME_EPSILON:float = 0.01
const SECONDS_PER_HOUR:float = 3600.0
const SECONDS_PER_MINUTE:float = 60.0
## A name defined more than once queues its definitions, which queue their own events: three passes
## of the queue, a step each
const JOINED_CHAIN_SECONDS:float = 4.0 * TICK
## A definition queued by itself at the start, by its negative delay [s]
const START_DELAY:float = 0.1
## How often a launcher firing once looks whether it is in reach (MaszynaScenarioEventServer
## FIRE_ONCE_CHECK_SECONDS)
const FIRE_ONCE_CHECK_SECONDS:float = 1.0
## How many runs show an event going on on every step a vehicle stays on its track
const EVERY_MOVE_RUNS:int = 3
## The length of a track built with a name of the scenery [m]
const NAMED_TRACK_LENGTH:float = 10.0
## A turntable `tt` turning about this pivot, half this long, between approach tracks along z; the
## sidings its ends meet at a quarter turn lie along x, this long (Track.cpp:2039-2051 lays it along z
## at no angle)
const TURNTABLE_PIVOT:Vector3 = Vector3(15.0, 0.0, 0.0)
const TURNTABLE_HALF_LENGTH:float = 5.0
const TURNTABLE_SIDING_LENGTH:float = 10.0
## The speed a joined turntable lets traffic on at [km/h] (Track.cpp:1816)
const TURNTABLE_JOINED_VELOCITY:float = 6.0


class RecordingAction extends ScenarioEventAction:
    var runs:Array[RID] = []
    var else_runs:Array[RID] = []
    var activators:Array[RID] = []

    func _run(event:RID, activator:RID) -> void:
        runs.append(event)
        activators.append(activator)

    func _run_else(event:RID, _activator:RID) -> void:
        else_runs.append(event)


class RequeueingAction extends ScenarioEventAction:
    var count:int = 0

    func _run(event:RID, _activator:RID) -> void:
        count += 1
        ScenarioEventServer.event_queue(event)


## The event system the test's memories, launchers and events are of, the original's
var _event_system:RID


func before_each() -> void:
    _event_system = ScenarioEventServer.system_create()
    ScenarioEventServer.system_attach_implementation(_event_system, MaszynaScenarioEventServer.implementation_get())


func after_each() -> void:
    ScenarioEventServer.system_free(_event_system)


func test_events_run_in_time_order_and_in_queue_order_on_equal_times() -> void:
    var action:RecordingAction = RecordingAction.new()
    var late:RID = _create_event(action, LATE_DELAY)
    var first:RID = _create_event(action, 0.0)
    var second:RID = _create_event(action, 0.0)

    ScenarioEventServer.event_queue(late)
    ScenarioEventServer.event_queue(first)
    ScenarioEventServer.event_queue(second)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 3, LATE_DELAY + TICK,
            "the three events run"):
        return

    var expected:Array[RID] = [first, second, late]
    assert_eq(action.runs, expected)
    _free_events([late, first, second])


func test_a_queued_event_is_not_queued_again_and_keeps_its_activator() -> void:
    var action:RecordingAction = RecordingAction.new()
    var event:RID = _create_event(action, 0.0)
    # any RID stands in for a vehicle - the server does not look into its activators
    var activator:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    var other_activator:RID = MaszynaScenarioEventServer.memory_create(_event_system)

    assert_true(ScenarioEventServer.event_queue(event, activator))
    assert_false(ScenarioEventServer.event_queue(event, other_activator), "a waiting event should refuse")
    assert_true(ScenarioEventServer.event_is_queued(event))
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, TICK,
            "the event run"):
        return

    var expected:Array[RID] = [activator]
    assert_eq(action.activators, expected)
    assert_false(ScenarioEventServer.event_is_queued(event))
    _free_events([event])
    MaszynaScenarioEventServer.memory_free(activator)
    MaszynaScenarioEventServer.memory_free(other_activator)


func test_an_event_queueing_itself_runs_once_a_step() -> void:
    var action:RequeueingAction = RequeueingAction.new()
    var event:RID = _create_event(action, 0.0)

    # the event server takes one pass of its queue a step of the clock
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.count > 0, TICK,
            "the event run"):
        return
    await step(1)
    var count:int = action.count
    await step(1)

    assert_eq(action.count, count + 1, "once a step: the pass does not run the event it queued")
    _free_events([event])


func test_the_condition_chooses_between_the_events_and_the_else_events() -> void:
    var action:RecordingAction = RecordingAction.new()
    var memory:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    var chosen:RID = _create_event(action, 0.0)
    var otherwise:RID = _create_event(action, 0.0)
    var multiple_action:MaszynaMultipleAction = MaszynaMultipleAction.new()
    var events:Array[RID] = [chosen]
    var else_events:Array[RID] = [otherwise]
    multiple_action.events = events
    multiple_action.else_events = else_events
    var multiple:RID = _create_event(multiple_action, 0.0)
    var condition:MaszynaEventCondition = MaszynaEventCondition.new()
    var memories:Array[RID] = [memory]
    condition.memories = memories
    condition.text = "go*"
    condition.mask = MaszynaScenarioEventServer.MEMORY_FIELD_TEXT
    ScenarioEventServer.event_attach_condition(multiple, condition)
    var activator:RID = MaszynaScenarioEventServer.memory_create(_event_system)

    MaszynaScenarioEventServer.memory_set_values(memory, "go_ahead", 0.0, 0.0)
    ScenarioEventServer.event_queue(multiple, activator)
    # the multiple runs on one pass of the queue, the event it queues on the next
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, CHAINED_EVENT_SECONDS,
            "the chosen event run"):
        return
    MaszynaScenarioEventServer.memory_set_values(memory, "stop", 0.0, 0.0)
    ScenarioEventServer.event_queue(multiple)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 2, CHAINED_EVENT_SECONDS,
            "the else event run"):
        return

    var expected:Array[RID] = [chosen, otherwise]
    assert_eq(action.runs, expected, "a text with * should be compared up to it")
    assert_eq(action.activators[0], activator, "multiple should hand its activator on")
    _free_events([chosen, otherwise, multiple])
    MaszynaScenarioEventServer.memory_free(memory)
    MaszynaScenarioEventServer.memory_free(activator)


func test_pause_stops_the_time_and_the_speed_scales_it() -> void:
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    ScenarioEventServer.event_queue(event)

    SimulationServer.simulation_pause()
    var paused_at:float = SimulationServer.simulation_get_time()
    await get_tree().process_frame
    await get_tree().process_frame
    assert_eq(SimulationServer.simulation_get_time(), paused_at, "the time should stand while paused")
    SimulationServer.simulation_unpause()

    SimulationServer.simulation_speed = FAST_SPEED
    var fast_from:float = SimulationServer.simulation_get_time()
    SimulationServer.simulation_advance(TICK)
    var advanced:float = SimulationServer.simulation_get_time() - fast_from
    assert_gt(advanced, 0.0, "the time should run at the speed")
    assert_lte(advanced, MAX_FRAME_DELTA * FAST_SPEED, "a frame counts at most MAX_FRAME_DELTA, sped up")
    SimulationServer.simulation_reset_speed()
    _free_events([event])


func test_a_launcher_fires_only_when_its_condition_passes() -> void:
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var memory:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    var condition:MaszynaEventCondition = MaszynaEventCondition.new()
    var memories:Array[RID] = [memory]
    condition.memories = memories
    condition.value1 = 1.0
    condition.mask = MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_attach_condition(launcher, condition)

    MaszynaScenarioEventServer.launcher_fire(launcher)
    assert_false(ScenarioEventServer.event_is_queued(event))
    MaszynaScenarioEventServer.memory_set_values(memory, "", 1.0, 0.0)
    MaszynaScenarioEventServer.launcher_fire(launcher)
    assert_true(ScenarioEventServer.event_is_queued(event))

    MaszynaScenarioEventServer.launcher_free(launcher)
    MaszynaScenarioEventServer.memory_free(memory)
    _free_events([event])


func test_a_radio_call_fires_the_launchers_listening_in_range() -> void:
    var near:RID = _create_event(RecordingAction.new(), NEVER)
    var far:RID = _create_event(RecordingAction.new(), NEVER)
    var other_call:RID = _create_event(RecordingAction.new(), NEVER)
    var launchers:Array[RID] = []
    for setup:Array in [[near, RailVehicleRadio.RADIO_CALL3, Vector3.ZERO], [far, RailVehicleRadio.RADIO_CALL3, Vector3(500, 0, 0)],
            [other_call, RailVehicleRadio.RADIO_CALL1, Vector3.ZERO]]:
        var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
        MaszynaScenarioEventServer.launcher_set_events(launcher, setup[0], RID())
        MaszynaScenarioEventServer.launcher_set_radio_call(launcher, setup[1])
        MaszynaScenarioEventServer.launcher_set_position(launcher, setup[2])
        MaszynaScenarioEventServer.launcher_set_radius(launcher, 100.0)
        launchers.append(launcher)

    var radio:SceneryVirtualRadio = SceneryVirtualRadio.new()
    radio.transmit(Vector3(10, 0, 0), 1, 0.0, RadioServer.KIND_CALL, RailVehicleRadio.RADIO_CALL3)

    assert_true(ScenarioEventServer.event_is_queued(near), "call 3 within 100 m")
    assert_false(ScenarioEventServer.event_is_queued(far), "out of range")
    assert_false(ScenarioEventServer.event_is_queued(other_call), "listens to call 1")
    for launcher:RID in launchers:
        MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([near, far, other_call])


func test_putvalues_cab_signal_reaches_the_security_system() -> void:
    var controller:VehicleController = build_vehicle("CabSignalTest", load("res://tests/fixtures/sm42_vehicle.tres"))
    var vehicle:RID = controller.get_rid()
    var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
    action.command = "CabSignal"
    var event:RID = _create_event(action, 0.0)
    watch_signals(VehicleServer)

    ScenarioEventServer.event_queue(event, vehicle)
    if not await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(event), TICK,
            "the CabSignal event run"):
        return

    assert_signal_emitted_with_parameters(
        VehicleServer, "vehicle_command_received", [vehicle, "security_cabsignal_trigger", null, null]
    )
    _free_events([event])


## Event.cpp:2431-2444 - `departuredelay`: queued by a train, the event runs that long from its
## departure by its timetable; queued by nothing, after its delay alone
func test_a_departure_delay_counts_from_the_departure_of_the_train() -> void:
    # the timetable is the trainset's: its run's departure from where it stands
    var track:RID = build_track("departure_delay", 100.0)
    var vehicle:RID = _create_whois_vehicle(track)
    var timetable:RID = StationServer.timetable_create()
    # a stop: arrived a minute ago, leaving in UNTIL_DEPARTURE seconds
    StationServer.timetable_add_entry(timetable, MaszynaStations.station_of("Departure"),
            SimulationServer.time_of_day - SECONDS_PER_MINUTE / SECONDS_PER_HOUR,
            SimulationServer.time_of_day + UNTIL_DEPARTURE / SECONDS_PER_HOUR)
    var run:RID = StationServer.run_create(timetable, RailVehicleServer.vehicle_get_trainset(vehicle))
    var event:RID = _create_event(RecordingAction.new(), EVENT_DELAY)
    MaszynaScenarioEventServer.event_set_departure_delay(event, DEPARTURE_DELAY)
    var now:float = SimulationServer.simulation_get_time()

    ScenarioEventServer.event_queue(event, vehicle)
    assert_almost_eq(ScenarioEventServer.event_get_run_time(event), now + EVENT_DELAY + UNTIL_DEPARTURE + DEPARTURE_DELAY,
            RUN_TIME_EPSILON)
    ScenarioEventServer.event_free(event)

    event = _create_event(RecordingAction.new(), EVENT_DELAY)
    MaszynaScenarioEventServer.event_set_departure_delay(event, DEPARTURE_DELAY)
    ScenarioEventServer.event_queue(event)
    assert_almost_eq(ScenarioEventServer.event_get_run_time(event), now + EVENT_DELAY, RUN_TIME_EPSILON,
            "no train, no departure")
    ScenarioEventServer.event_free(event)

    StationServer.run_free(run)
    StationServer.timetable_free(timetable)
    timetable = StationServer.timetable_create()
    StationServer.timetable_add_entry(timetable, MaszynaStations.station_of("Departure"),
            SimulationServer.time_of_day - 2.0 * UNTIL_DEPARTURE / SECONDS_PER_HOUR,
            SimulationServer.time_of_day - UNTIL_DEPARTURE / SECONDS_PER_HOUR)
    run = StationServer.run_create(timetable, RailVehicleServer.vehicle_get_trainset(vehicle))
    event = _create_event(RecordingAction.new(), EVENT_DELAY)
    MaszynaScenarioEventServer.event_set_departure_delay(event, DEPARTURE_DELAY)
    ScenarioEventServer.event_queue(event, vehicle)
    assert_almost_eq(ScenarioEventServer.event_get_run_time(event), now, RUN_TIME_EPSILON, "departed long ago: at once")
    ScenarioEventServer.event_free(event)
    StationServer.run_free(run)
    StationServer.timetable_free(timetable)
    VehicleServer.vehicle_free(vehicle)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


func test_a_launcher_fires_when_the_clock_shows_its_time() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_set_time_of_day(launcher, 10, 50)

    SimulationServer.time_of_day = 10.5
    assert_false(ScenarioEventServer.event_is_queued(event), "10:30 is not its time")
    SimulationServer.time_of_day = 10.0 + 50.5 / 60.0
    assert_true(ScenarioEventServer.event_is_queued(event), "10:50 is")

    MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([event])
    SimulationServer.time_of_day = clock


## 21 + 5/60 is 21.08333..., whose minutes truncated were 4 - a launcher on a scenario's start
## minute never fired
func test_a_launcher_fires_at_a_start_minute_the_clock_holds_inexactly() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_set_time_of_day(launcher, 21, 5)

    SimulationServer.time_of_day = 21.0 + 5.0 / 60.0
    assert_true(ScenarioEventServer.event_is_queued(event))

    MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([event])
    SimulationServer.time_of_day = clock


## The original looks at the condition all through the minute (Event.cpp:2293-2306)
func test_a_launcher_waits_within_its_minute_for_its_condition() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var memory:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    var condition:MaszynaEventCondition = MaszynaEventCondition.new()
    var memories:Array[RID] = [memory]
    condition.memories = memories
    condition.value1 = 1.0
    condition.mask = MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_attach_condition(launcher, condition)
    MaszynaScenarioEventServer.launcher_set_time_of_day(launcher, 10, 50)

    SimulationServer.time_of_day = 10.0 + 50.25 / 60.0
    assert_false(ScenarioEventServer.event_is_queued(event), "its condition does not pass yet")
    MaszynaScenarioEventServer.memory_set_values(memory, "", 1.0, 0.0)
    assert_true(ScenarioEventServer.event_is_queued(event), "it passes within the minute")

    MaszynaScenarioEventServer.launcher_free(launcher)
    MaszynaScenarioEventServer.memory_free(memory)
    _free_events([event])
    SimulationServer.time_of_day = clock


## Only a launcher of a negative radius is global; the rest fire near the camera (scene.cpp:126-139)
func test_a_timed_launcher_of_a_radius_fires_only_near_the_camera() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_set_radius(launcher, 10.0)
    MaszynaScenarioEventServer.launcher_set_position(launcher, Vector3(0.0, 0.0, 1.0e6))
    MaszynaScenarioEventServer.launcher_set_time_of_day(launcher, 10, 50)

    SimulationServer.time_of_day = 10.0 + 50.5 / 60.0
    assert_false(ScenarioEventServer.event_is_queued(event))
    # coming into its reach within the minute fires it: the original looks at it every frame
    # (scene.cpp:124-143)
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    MaszynaScenarioEventServer.launcher_set_position(launcher, camera.global_position)
    await step(1)
    assert_true(ScenarioEventServer.event_is_queued(event), "in reach within its minute")

    SceneryStreamingServer.streaming_set_camera(0)
    MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([event])
    SimulationServer.time_of_day = clock


func test_a_passenger_stop_is_named_as_the_timetable_names_it() -> void:
    var models:Array[ModelData] = []
    var root:MaszynaIncludeNode = await _build_scenery(
        "event w4_stopinfo putvalues 0 none 1 2 3 PassengerStopPoint:Jawor#2 -4 151 endevent", models
    )
    var event:RID = ScenarioEventServer.event_get_rid_by_name(&"w4_stopinfo")
    var action:MaszynaVehicleCommandAction = ScenarioEventServer.event_get_action(event)

    assert_eq(action.command, "PassengerStopPoint:Jawor", "unique only past its #")
    assert_true(MaszynaScenarioEventServer.event_is_passive(event), "read by the drivers ahead, never queued")
    root.free()


func test_a_departure_delay_is_read_from_the_scenery() -> void:
    var models:Array[ModelData] = []
    var root:MaszynaIncludeNode = await _build_scenery("event odjazd multiple 2 none endevent "
            + "event odjazd_signal multiple 0 none departuredelay %s endevent" % DEPARTURE_DELAY, models)

    assert_eq(MaszynaScenarioEventServer.event_get_departure_delay(ScenarioEventServer.event_get_rid_by_name(&"odjazd_signal")),
            DEPARTURE_DELAY)
    assert_true(is_nan(MaszynaScenarioEventServer.event_get_departure_delay(ScenarioEventServer.event_get_rid_by_name(&"odjazd"))),
            "none without the keyword")
    root.free()


func test_scenery_memcells_and_value_events() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 Cell1 memcell 0 0 0 Start 1 2 none endmemcell "
        + "node -1 0 cell2 memcell 0 0 0 Other 7 8 none endmemcell "
        + "event set_cell updatevalues 0 cell1 Go * 5 endevent "
        + "event add_cell addvalues 0 CELL1 _x 1 * endevent "
        + "event copy_cell copyvalues 0 cell2 cell1 2 endevent",
        []
    )
    # the node's name is read in lower case, as the original reads it (cParser::getTokens())
    var cell1:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"cell1")
    var cell2:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"cell2")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell1), "Start")

    if not await _run_event(&"set_cell"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell1), "Go", "the text keeps its case")
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(cell1), 1.0, "* should leave the value")
    assert_eq(MaszynaScenarioEventServer.memory_get_value2(cell1), 5.0)

    if not await _run_event(&"add_cell"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell1), "Go_x")
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(cell1), 2.0)
    assert_eq(MaszynaScenarioEventServer.memory_get_value2(cell1), 5.0)

    if not await _run_event(&"copy_cell"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell2), "Other", "mask 2 copies only value 1")
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(cell2), 2.0)
    assert_eq(MaszynaScenarioEventServer.memory_get_value2(cell2), 8.0)
    root.free()


func test_scenery_onstart_and_negative_delay_events_are_queued() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "event scenery_onstart updatevalues 0.0 none a 0 0 endevent "
        + "event at_start updatevalues -30 none b 0 0 endevent "
        + "event later updatevalues 30 none c 0 0 endevent",
        []
    )

    assert_true(ScenarioEventServer.event_is_queued(ScenarioEventServer.event_get_rid_by_name(&"scenery_onstart")))
    var at_start:RID = ScenarioEventServer.event_get_rid_by_name(&"at_start")
    assert_true(ScenarioEventServer.event_is_queued(at_start))
    assert_eq(ScenarioEventServer.event_get_delay(at_start), 30.0, "the delay itself is positive")
    assert_false(ScenarioEventServer.event_is_queued(ScenarioEventServer.event_get_rid_by_name(&"later")))
    root.free()


func test_scenery_lights_event_shows_the_aspect() -> void:
    var instance:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    instance.model = MaszynaE3DModel.new()
    add_child_autoqfree(instance)
    var signal_head:RID = SignallingServer.signal_head_create()
    instance.attach_signal_head(signal_head)
    SignallingServer.signal_head_set_name(signal_head, &"Sem_A")
    var aspects:Dictionary = {&"sem_ligh1": PackedFloat32Array([1.0])}
    SignallingServer.signal_head_set_kind(signal_head, MaszynaSignalHeadKindFactory.create_kind(aspects))
    var system:RID = SignallingServer.system_create()
    SignallingServer.system_attach_implementation(system, MaszynaSignallingImplementation.new())
    SignallingServer.system_add_signal_head(system, signal_head)
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = "Sem_A"
    var models:Array[ModelData] = [model_data]
    var root:MaszynaIncludeNode = await _build_scenery("event sem_a_sem_ligh1 lights 0 sem_a 1 endevent", models)

    if not await _run_event(&"sem_a_sem_ligh1"):
        return

    assert_eq(SignallingServer.signal_head_get_aspect(signal_head), &"sem_ligh1")
    assert_eq(SignallingServer.signal_head_get_light_state(signal_head, 0), SignallingServer.LIGHT_STATE_ON)
    root.free()
    SignallingServer.system_free(system)


func test_a_track_event_fires_once_per_entry_in_its_direction() -> void:
    var action:RecordingAction = RecordingAction.new()
    var to_end:RID = _create_event(action, 0.0)
    var to_start:RID = _create_event(action, 0.0)
    var track:RID = TrackServer.track_create()
    var other_track:RID = TrackServer.track_create()
    var vehicle:RID = MaszynaScenarioEventServer.memory_create(_event_system) # stands in for a vehicle
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENTALL2, to_end)
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENTALL1, to_start)

    TrackServer.track_report_vehicle_entered(track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    assert_true(ScenarioEventServer.event_is_queued(to_end))
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, TICK,
            "the track event run"):
        return
    TrackServer.track_report_vehicle_stopped(track, vehicle)
    TrackServer.track_report_vehicle_started(track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    assert_false(ScenarioEventServer.event_is_queued(to_end), "once per entry")
    TrackServer.track_report_vehicle_started(track, vehicle, TrackServer.TRAVEL_TOWARD_START)
    assert_true(ScenarioEventServer.event_is_queued(to_start), "the other direction once too")
    TrackServer.track_report_vehicle_left(track, vehicle)
    TrackServer.track_report_vehicle_entered(other_track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    TrackServer.track_report_vehicle_left(other_track, vehicle)
    TrackServer.track_report_vehicle_entered(track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    assert_true(ScenarioEventServer.event_is_queued(to_end), "a new entry fires again")

    _free_events([to_end, to_start])
    TrackServer.track_report_vehicle_left(track, vehicle)
    TrackServer.track_free(track)
    TrackServer.track_free(other_track)
    MaszynaScenarioEventServer.memory_free(vehicle)


## An event1/event2 of a delay of -1 or less goes on on every step a vehicle stays on its track,
## the crew one only for a vehicle whose cab is occupied (TrkFoll.cpp:249-260)
func test_a_track_event_of_a_negative_delay_goes_on_while_the_vehicle_is_on_the_track() -> void:
    var every_move:RecordingAction = RecordingAction.new()
    var crew:RecordingAction = RecordingAction.new()
    var all_event:RID = _create_event(every_move, 0.0)
    var crew_event:RID = _create_event(crew, 0.0)
    MaszynaScenarioEventServer.event_set_queued_on_every_move(all_event, true)
    MaszynaScenarioEventServer.event_set_queued_on_every_move(crew_event, true)
    var track:RID = TrackServer.track_create()
    var vehicle:RID = MaszynaScenarioEventServer.memory_create(_event_system) # stands in for a vehicle of no cab
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENTALL2, all_event)
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENT2, crew_event)

    TrackServer.track_report_vehicle_entered(track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    if not await wait_simulated_until(func() -> bool: return every_move.runs.size() >= EVERY_MOVE_RUNS,
            EVERY_MOVE_RUNS * CHAINED_EVENT_SECONDS + TICK, "the event run on the steps after"):
        return
    assert_eq(crew.runs.size(), 0, "no crew event for a vehicle whose cab nobody occupies")
    TrackServer.track_report_vehicle_left(track, vehicle)
    # what was queued before it left runs still
    await step(ticks(CHAINED_EVENT_SECONDS))
    var runs:int = every_move.runs.size()
    await step(ticks(CHAINED_EVENT_SECONDS))
    assert_eq(every_move.runs.size(), runs, "not off the track")

    _free_events([all_event, crew_event])
    TrackServer.track_free(track)
    MaszynaScenarioEventServer.memory_free(vehicle)


func test_a_standing_event_goes_on_while_the_vehicle_stands() -> void:
    var action:RecordingAction = RecordingAction.new()
    var standing:RID = _create_event(action, 0.0)
    var track:RID = TrackServer.track_create()
    var vehicle:RID = MaszynaScenarioEventServer.memory_create(_event_system) # stands in for a vehicle
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENTALL0, standing)
    TrackServer.track_report_vehicle_entered(track, vehicle)

    TrackServer.track_report_vehicle_stopped(track, vehicle)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() >= 2, CHAINED_EVENT_SECONDS,
            "the standing event run again"):
        return
    assert_gt(action.runs.size(), 1, "queued again after its run")
    TrackServer.track_report_vehicle_started(track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    if not await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(standing), TICK,
            "the standing event let go"):
        return
    var runs:int = action.runs.size()
    await get_tree().process_frame
    assert_eq(action.runs.size(), runs, "not once the vehicle moves")

    _free_events([standing])
    TrackServer.track_report_vehicle_left(track, vehicle)
    TrackServer.track_free(track)
    MaszynaScenarioEventServer.memory_free(vehicle)


func test_the_other_axles_standing_event_goes_on_too() -> void:
    var action:RecordingAction = RecordingAction.new()
    var standing:RID = _create_event(action, 0.0)
    var track:RID = TrackServer.track_create() # the vehicle's own track
    var other_track:RID = TrackServer.track_create() # and the other axle's
    var vehicle:RID = MaszynaScenarioEventServer.memory_create(_event_system) # stands in for a vehicle
    MaszynaScenarioEventServer.track_add_event(other_track, MaszynaScenarioEventServer.TRACK_EVENTALL0, standing)
    TrackServer.track_report_vehicle_entered(track, vehicle)

    TrackServer.track_report_vehicle_stopped(track, vehicle)
    TrackServer.track_report_vehicle_stopped(other_track, vehicle)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() >= 2, CHAINED_EVENT_SECONDS,
            "the other track's standing event run again"):
        return
    TrackServer.track_report_vehicle_started(track, vehicle, TrackServer.TRAVEL_TOWARD_END)
    if not await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(standing), TICK,
            "the standing event let go"):
        return
    var runs:int = action.runs.size()
    await get_tree().process_frame
    assert_eq(action.runs.size(), runs, "not once the vehicle moves")

    _free_events([standing])
    TrackServer.track_report_vehicle_left(track, vehicle)
    TrackServer.track_free(track)
    TrackServer.track_free(other_track)
    MaszynaScenarioEventServer.memory_free(vehicle)


func test_an_isolated_section_counts_axles_not_vehicles() -> void:
    var track:RID = TrackServer.track_create()
    var vehicle:RID = MaszynaScenarioEventServer.memory_create(_event_system) # stands in for a vehicle
    var section:RID = TrackServer.isolated_create()
    TrackServer.isolated_add_track(section, track)
    var entered:Array[RID] = []
    var record:Callable = func(_isolated:RID, axle_vehicle:RID) -> void: entered.append(axle_vehicle)
    TrackServer.isolated_axle_entered.connect(record)

    TrackServer.track_report_vehicle_entered(track, vehicle)
    assert_false(TrackServer.isolated_is_occupied(section), "listing the vehicle on the track counts no axle")
    TrackServer.track_axle_entered(track, vehicle)
    TrackServer.track_axle_entered(track, vehicle)
    TrackServer.track_axle_left(track, vehicle)
    assert_true(TrackServer.isolated_is_occupied(section), "one axle of two still on it")
    TrackServer.track_axle_left(track, vehicle)
    assert_false(TrackServer.isolated_is_occupied(section), "free once the second axle left")

    TrackServer.isolated_axle_entered.disconnect(record)
    assert_eq(entered, [vehicle, vehicle] as Array[RID], ":inc for each axle")
    TrackServer.track_report_vehicle_left(track, vehicle)
    TrackServer.isolated_free(section)
    TrackServer.track_free(track)
    MaszynaScenarioEventServer.memory_free(vehicle)


func test_scenery_animation_turns_the_submodel() -> void:
    var instance:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    var model:MaszynaE3DModel = MaszynaE3DModel.new()
    var arm:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    arm.resource_name = "Ramie01"
    arm.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    var submodels:Array[MaszynaE3DSubModel] = [arm]
    model.submodels = submodels
    instance.model = model
    add_child_autoqfree(instance)
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = "rog1"
    var models:Array[ModelData] = [model_data]
    var instances:Dictionary[String, RID] = {"rog1": instance.get_model_instance()}
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 moving 0 0 none endmemcell "
        + "event rog1on animation 0 rog1 rotate ramie01 0 0 %.0f %.0f endevent " % [QUARTER_TURN, TURN_SPEED]
        + "event rog1.ramie01:done updatevalues 0 c1 done * * endevent",
        models, instances
    )
    var arm_node:Node3D = instance.find_child("Ramie01", true, false)
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")

    if not await _run_event(&"rog1on"):
        return
    # the arm turns on the simulation's clock at its speed - the quarter turn in QUARTER_TURN / TURN_SPEED
    # simulated seconds, its :done event on the step it arrives
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(cell) == "done",
            QUARTER_TURN / TURN_SPEED + TICK, "the animation's arrival"):
        return

    assert_true(arm_node.basis.is_equal_approx(Basis(Vector3(0, 0, 1), deg_to_rad(QUARTER_TURN))), "turned by 90 degrees about z")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell), "done", "the :done event runs when it arrives")
    root.free()


func test_scenery_voltage_event_sets_the_power_source() -> void:
    var power_source:RID = TractionServer.power_source_create()
    TractionServer.power_source_set_params(power_source, "Pwr1", 3000.0, 0.0, 0.2, 1000.0, 1.0, 3, 60.0, false)
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var power_source_data:PowerSourceData = PowerSourceData.new()
    power_source_data.name = "Pwr1"
    var power_sources:Array[PowerSourceData] = [power_source_data]
    var context:MaszynaImporterContext = _parse("event keyctrl05 voltage 0.1 pwr1 2400 endevent")
    var tracks:Array[RID] = []
    var models:Array[ModelData] = []
    var model_rids:Array[RID] = []
    var isolated_sections:Array[MaszynaIsolatedData] = []
    await MaszynaEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, isolated_sections, context.tracks,
        tracks, models, model_rids, power_sources
    )

    if not await _run_event(&"keyctrl05"):
        return

    assert_eq(TractionServer.power_source_get_nominal_voltage(power_source), 2400.0)
    root.free()
    TractionServer.power_source_free(power_source)


func test_shift_and_a_digit_queue_the_keyctrl_event() -> void:
    var keyboard:MaszynaScenarioKeyboard = MaszynaScenarioKeyboard.new()
    add_child_autoqfree(keyboard)
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    ScenarioEventServer.event_set_name(event, &"keyctrl03")
    var press:InputEventAction = InputEventAction.new()
    press.action = &"scenario_keyctrl_3"
    press.pressed = true

    Input.parse_input_event(press)
    Input.flush_buffered_events()
    await get_tree().process_frame

    assert_true(ScenarioEventServer.event_is_queued(event))
    _free_events([event])


func test_scenery_isolated_section_fires_busy_and_marks_its_memory() -> void:
    var track:RID = TrackServer.track_create()
    var vehicle:RID = MaszynaScenarioEventServer.memory_create(_event_system) # stands in for a vehicle
    var tracks:Dictionary[String, RID] = {"t1": track}
    var models:Array[ModelData] = []
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 idle 0 0 none endmemcell "
        + "isolated s1 t1 endisolated "
        + "event s1:busy updatevalues 0 c1 busy * * endevent",
        models, {}, tracks
    )
    var section:RID = TrackServer.isolated_get_rid_by_name(&"s1")
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")
    var own_memory:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"s1")
    assert_true(own_memory.is_valid(), "a section has a memory of its name")

    TrackServer.track_axle_entered(track, vehicle)
    assert_true(TrackServer.isolated_is_occupied(section))
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(cell) == "busy", TICK,
            "the busy event run"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell), "busy")
    assert_eq(int(MaszynaScenarioEventServer.memory_get_value2(own_memory)) & 1, 1, "value 2 made odd")

    TrackServer.track_axle_left(track, vehicle)
    assert_false(TrackServer.isolated_is_occupied(section))
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_value2(own_memory) == 0.0, TICK,
            "the section's memory cleared"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_value2(own_memory), 0.0, "the low byte cleared")
    root.free()
    TrackServer.track_free(track)
    MaszynaScenarioEventServer.memory_free(vehicle)


func test_memcompareex_and_track_tests() -> void:
    var memory:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    MaszynaScenarioEventServer.memory_set_values(memory, "b", 5.0, 0.0)
    var memories:Array[RID] = [memory]
    var condition:MaszynaEventCondition = MaszynaEventCondition.new()
    condition.memories = memories
    condition.text = "a"
    condition.text_operator = MaszynaEventCondition.OPERATOR_GREATER
    condition.value1 = 3.0
    condition.value1_operator = MaszynaEventCondition.OPERATOR_LESS
    condition.mask = MaszynaScenarioEventServer.MEMORY_FIELD_TEXT | MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1
    var action:RecordingAction = RecordingAction.new()
    var event:RID = _create_event(action, 0.0)
    ScenarioEventServer.event_attach_condition(event, condition)

    condition.pass = MaszynaEventCondition.PASS_ANY
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, TICK,
            "the event run"):
        return
    condition.pass = MaszynaEventCondition.PASS_ALL
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.else_runs.size() == 1, TICK,
            "the else events run"):
        return
    assert_eq(action.runs.size(), 1, "any: \"b\" > \"a\" passes although 5 < 3 does not")
    assert_eq(action.else_runs.size(), 1, "all: 5 < 3 fails")

    var track:RID = TrackServer.track_create()
    var condition_tracks:Array[RID] = [track]
    condition.mask = 0
    condition.tracks = condition_tracks
    condition.track_test = MaszynaEventCondition.TRACK_TEST_OCCUPIED
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.else_runs.size() == 2, TICK,
            "the else events run for an empty track"):
        return
    TrackServer.track_report_vehicle_entered(track, memory)
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 2, TICK,
            "the events run for an occupied track"):
        return
    assert_eq(action.else_runs.size(), 2, "an empty track is not occupied")
    assert_eq(action.runs.size(), 2, "a track with a vehicle is")

    TrackServer.track_report_vehicle_left(track, memory)
    TrackServer.track_free(track)
    _free_events([event])
    MaszynaScenarioEventServer.memory_free(memory)


func test_the_queue_lists_the_queued_events_in_the_order_they_run() -> void:
    var action:RecordingAction = RecordingAction.new()
    var late:RID = _create_event(action, NEVER)
    var early:RID = _create_event(action, NEVER / 2.0)
    var never_queued:RID = _create_event(action, 0.0)
    ScenarioEventServer.event_queue(late)
    ScenarioEventServer.event_queue(early)

    var queued:Array[RID] = []
    for event:RID in ScenarioEventServer.queue_get_events():
        if event in [late, early, never_queued]:
            queued.append(event)
    var in_run_order:Array[RID] = [early, late]
    assert_eq(queued, in_run_order)
    ScenarioEventServer.event_free(early)
    assert_does_not_have(ScenarioEventServer.queue_get_events(), early, "a freed event is not queued")
    var rest:Array[RID] = [late, never_queued]
    _free_events(rest)


## reports#31: the dispatcher at Ozimek defines its check once per route; joined, the first one -
## the route set - runs as well as the later ones (Event.cpp:2323-2334, 2466-2490)
func test_a_name_defined_more_than_once_runs_every_definition_when_joined() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "config joinduplicatedevents yes endconfig "
        + "node -1 0 route memcell 0 0 0 Fo1 7 1 none endmemcell "
        + "node -1 0 exit memcell 0 0 0 none 0 0 none endmemcell "
        + "event check multiple 0 route leave condition memcompare Fo1 7 * endevent "
        + "event check multiple 0 route enter condition memcompare Fo1 6 * endevent "
        + "event leave updatevalues 0 exit out * * endevent "
        + "event enter updatevalues 0 exit in * * endevent "
        + "config joinduplicatedevents no endconfig",
        []
    )
    var exit:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"exit")

    ScenarioEventServer.event_queue(ScenarioEventServer.event_get_rid_by_name(&"check"))
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(exit) == "out",
            JOINED_CHAIN_SECONDS, "the first definition's route set"):
        return
    await step(2)
    assert_eq(MaszynaScenarioEventServer.memory_get_text(exit), "out", "the second definition's condition fails")
    root.free()


## The definitions run in their order: the second compares what the first has just written
func test_a_joined_definition_sees_what_the_one_before_it_wrote() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "config joinduplicatedevents yes endconfig "
        + "node -1 0 track memcell 0 0 0 free 0 0 none endmemcell "
        + "node -1 0 done memcell 0 0 0 none 0 0 none endmemcell "
        + "event reserve updatevalues 0 track taken 2 * condition memcompare free * 0 endevent "
        + "event reserve multiple 0 track mark condition memcompare taken 2 * endevent "
        + "event mark updatevalues 0 done yes * * endevent",
        []
    )
    var done:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"done")

    var track:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"track")

    ScenarioEventServer.event_queue(ScenarioEventServer.event_get_rid_by_name(&"reserve"))
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(done) == "yes",
            JOINED_CHAIN_SECONDS, "the reservation marked"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(track), "taken", "the first definition wrote it")
    root.free()


## Without joinduplicatedevents the first definition is disabled: a `multiple` runs without its
## memory comparison (Event.cpp:1218-1222), the later ones with theirs
func test_a_duplicated_multiple_runs_without_its_comparison_when_not_joined() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 cell memcell 0 0 0 a 0 0 none endmemcell "
        + "node -1 0 first memcell 0 0 0 none 0 0 none endmemcell "
        + "node -1 0 second memcell 0 0 0 none 0 0 none endmemcell "
        + "event check multiple 0 cell set_first condition memcompare zzz * * endevent "
        + "event check multiple 0 cell set_second condition memcompare a * * endevent "
        + "event set_first updatevalues 0 first ran * * endevent "
        + "event set_second updatevalues 0 second ran * * endevent",
        []
    )
    var first:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"first")
    var second:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"second")

    ScenarioEventServer.event_queue(ScenarioEventServer.event_get_rid_by_name(&"check"))
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(second) == "ran",
            JOINED_CHAIN_SECONDS, "the second definition run"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(first), "ran", "the comparison of the first is dropped")
    root.free()


## Without joinduplicatedevents any other first definition is never queued, so the name does
## nothing; an "onstart" one, queued before it was disabled, runs the later ones (Event.cpp:2402-2403,
## 2472-2480)
func test_a_duplicated_name_does_nothing_when_not_joined_but_its_onstart_runs_the_rest() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 first memcell 0 0 0 none 0 0 none endmemcell "
        + "node -1 0 second memcell 0 0 0 none 0 0 none endmemcell "
        + "event set_cells updatevalues 0 first ran * * endevent "
        + "event set_cells updatevalues 0 second ran * * endevent "
        + "event cells_onstart updatevalues 0 first started * * endevent "
        + "event cells_onstart updatevalues 0 second started * * endevent",
        []
    )
    var first:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"first")
    var second:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"second")

    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(second) == "started",
            JOINED_CHAIN_SECONDS, "the later onstart definition run"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(first), "none", "the disabled first does not run")

    ScenarioEventServer.event_queue(ScenarioEventServer.event_get_rid_by_name(&"set_cells"))
    await step(roundi(JOINED_CHAIN_SECONDS / TICK))
    assert_eq(MaszynaScenarioEventServer.memory_get_text(first), "none")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(second), "started", "the name does nothing")
    root.free()


## A duplicate of a magnet, a horn warning or line info is dropped, joined or not (Event.cpp:2304-2321)
func test_a_duplicate_of_a_dropped_name_is_left_out() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "config joinduplicatedevents yes endconfig "
        + "node -1 0 cell memcell 0 0 0 none 0 0 none endmemcell "
        + "event magnet_shp updatevalues 0 cell first * * endevent "
        + "event magnet_shp updatevalues 0 cell second * * endevent "
        + "event horn_warning updatevalues 0 cell third * * endevent "
        + "event horn_warning updatevalues 0 cell fourth * * endevent",
        []
    )
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"cell")

    if not await _run_event(&"magnet_shp"):
        return
    await step(2)
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell), "first")
    if not await _run_event(&"horn_warning"):
        return
    await step(2)
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell), "third")
    root.free()


## A later definition of a negative delay is queued by itself at the start and runs the ones after
## it (InitEvents(), Event.cpp:2493-2499)
func test_a_joined_definition_of_a_negative_delay_starts_the_ones_after_it() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "config joinduplicatedevents yes endconfig "
        + "node -1 0 first memcell 0 0 0 none 0 0 none endmemcell "
        + "node -1 0 second memcell 0 0 0 none 0 0 none endmemcell "
        + "node -1 0 third memcell 0 0 0 none 0 0 none endmemcell "
        + "event chain updatevalues %s first ran * * endevent " % NEVER
        + "event chain updatevalues -%s second ran * * endevent " % START_DELAY
        + "event chain updatevalues 0 third ran * * endevent",
        []
    )
    var first:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"first")
    var third:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"third")

    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(third) == "ran",
            START_DELAY + JOINED_CHAIN_SECONDS, "the definitions after the started one run"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(MaszynaScenarioEventServer.memory_get_rid_by_name(&"second")), "ran")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(first), "none", "the first is not started")
    root.free()


## joinduplicatedevents holds from where it is set for the rest of the load: an include parsed
## beside its file inherits it, and what follows the include gets what the include left
## (Globals.cpp:668-672)
func test_join_duplicated_events_holds_past_the_include_that_sets_it() -> void:
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    context.queue = WorkerTaskQueue.new()
    _parse("event before_include multiple 0 none endevent", context)
    context.objects.append(context.submit_include(
        _parse_include.bind("event inherited multiple 0 none endevent "
            + "config joinduplicatedevents yes endconfig event in_include multiple 0 none endevent"),
        "include.ctr", {}
    ))
    _parse("event after_include multiple 0 none endevent "
        + "config joinduplicatedevents no endconfig event after_config multiple 0 none endevent", context)
    context.merge_pending_includes()

    var joined:Dictionary[String, bool] = {}
    for event:MaszynaEventData in context.events:
        joined[event.name] = event.joined
    var expected:Dictionary[String, bool] = {
        "before_include": false, "inherited": false, "in_include": true, "after_include": true, "after_config": false,
    }
    assert_eq(joined, expected)
    assert_false(context.join_duplicated_events, "the file leaves what it set last")


func _create_event(action:ScenarioEventAction, delay:float) -> RID:
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.system_add_event(_event_system, event)
    ScenarioEventServer.event_set_delay(event, delay)
    ScenarioEventServer.event_attach_action(event, action)
    return event


func _free_events(events:Array[RID]) -> void:
    for event:RID in events:
        ScenarioEventServer.event_free(event)


## Parses the scenery text and builds it through MaszynaEventFactory; the returned include
## frees what was built when it is freed
func _build_scenery(
    text:String, models:Array[ModelData], model_instances:Dictionary[String, RID] = {},
    tracks:Dictionary[String, RID] = {}
) -> MaszynaIncludeNode:
    var context:MaszynaImporterContext = _parse(text)
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var model_rids:Array[RID] = []
    for model_data:MaszynaModelData in models:
        model_rids.append(model_instances.get(model_data.name, RID()))
    var scenery_tracks:Array[TrackData] = []
    var track_rids:Array[RID] = []
    for track_name:String in tracks:
        # the scenery names its tracks in TrackServer, where the events find them
        TrackServer.track_update(tracks[track_name], TrackServer.TRACK_NORMAL, track_name, SCENERY_TRACK_WIDTH)
        var track_data:MaszynaTrackData = MaszynaTrackData.new()
        track_data.name = track_name
        scenery_tracks.append(track_data)
        track_rids.append(tracks[track_name])
    var power_sources:Array[PowerSourceData] = []
    await MaszynaEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
        scenery_tracks, track_rids, models, model_rids, power_sources
    )
    return root


func _parse(text:String, context:MaszynaImporterContext = MaszynaImporterContext.new()) -> MaszynaImporterContext:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(text.to_utf8_buffer())
    parser.register_handler("event", func(p:MaszynaParser) -> Array: return EventImporter.new().import(p, context))
    parser.register_handler("node", func(p:MaszynaParser) -> Array: return NodeImporter.new().import(p, context))
    parser.register_handler("isolated", func(p:MaszynaParser) -> Array: return IsolatedImporter.new().import(p, context))
    parser.register_handler("config", func(p:MaszynaParser) -> Array: return ConfigImporter.new().import(p, context))
    parser.parse()
    return context


## An include's task (MaszynaImporterContext.submit_include()) parsing the text in the state it inherits
func _parse_include(
    _filename:String, _parameters:Dictionary, state:Dictionary, _queue:WorkerTaskQueue, text:String
) -> MaszynaImporterContext:
    return _parse(text, MaszynaImporterContext.from_state(state))


## Queues the scenery's event and waits for it to run - its delay, in simulated time; true when it ran
func _run_event(event_name:StringName) -> bool:
    var event:RID = ScenarioEventServer.event_get_rid_by_name(event_name)
    ScenarioEventServer.event_queue(event)
    return await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(event),
            ScenarioEventServer.event_get_delay(event) + TICK, "%s run" % event_name)


## A log handler keeping the lines it is handed
class RecordingLogHandler:
    extends GameLogHandler

    var lines:PackedStringArray = []

    func _handle(_logger_id:String, _level:GameLog.LogLevel, line:String) -> void:
        lines.append(line)


func test_an_event_aimed_at_what_does_not_exist_does_nothing() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Start 0 0 none endmemcell "
        + "event set_missing updatevalues 0 c1|nowhere Go * * endevent", []
    )

    assert_null(ScenarioEventServer.event_get_action(ScenarioEventServer.event_get_rid_by_name(&"set_missing")),
            "disabled, as init_targets() disables it")
    root.free()


func test_only_value_events_and_multiple_read_a_condition() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Start 0 0 none endmemcell "
        + "event set_cell updatevalues 0 c1 Go * * condition memcompare Start * * endevent "
        + "event log_cell logvalues 0 c1 condition memcompare Other * * endevent", []
    )

    assert_not_null(ScenarioEventServer.event_get_condition(ScenarioEventServer.event_get_rid_by_name(&"set_cell")))
    assert_null(ScenarioEventServer.event_get_condition(ScenarioEventServer.event_get_rid_by_name(&"log_cell")),
            "logvalues passes over its condition")
    root.free()


func test_copyvalues_needs_its_source_and_copies_all_fields_for_a_mask_that_is_no_number() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Start 1 2 none endmemcell "
        + "node -1 0 c2 memcell 0 0 0 Other 7 8 none endmemcell "
        + "event copy_missing copyvalues 0 c1 nowhere endevent "
        + "event copy_all copyvalues 0 c1 c2 all endevent", []
    )
    var cell1:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")

    assert_null(ScenarioEventServer.event_get_action(ScenarioEventServer.event_get_rid_by_name(&"copy_missing")))
    if not await _run_event(&"copy_all"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell1), "Other")
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(cell1), 7.0)
    assert_eq(MaszynaScenarioEventServer.memory_get_value2(cell1), 8.0)
    root.free()


func test_a_multiple_aimed_at_a_missing_cell_compares_none() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Start 0 0 none endmemcell "
        + "event check multiple 0 c1|nowhere mark condition memcompare Never * * endevent "
        + "event mark updatevalues 0 c1 Marked * * endevent", []
    )
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")

    if not await _run_event(&"check"):
        return
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(cell) == "Marked",
            CHAINED_EVENT_SECONDS, "the event of the multiple"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell), "Marked", "the comparison was dropped")
    root.free()


func test_a_multiple_queues_itself_again_only_with_a_delay_of_five_seconds() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "event short multiple 1 none short endevent "
        + "event long multiple 5 none long endevent", []
    )
    var short:RID = ScenarioEventServer.event_get_rid_by_name(&"short")
    var long:RID = ScenarioEventServer.event_get_rid_by_name(&"long")

    if not await _run_event(&"short"):
        return
    assert_false(ScenarioEventServer.event_is_queued(short), "a second is no sensible period (Event.cpp:1300)")
    ScenarioEventServer.event_queue(long)
    var first_run:float = ScenarioEventServer.event_get_run_time(long)
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.event_get_run_time(long) > first_run,
            ScenarioEventServer.event_get_delay(long) + TICK, "long run"):
        return
    assert_true(ScenarioEventServer.event_is_queued(long), "five seconds is: it ran and is queued again")
    root.free()


func test_a_condition_may_test_for_occupied_and_free_tracks_at_once() -> void:
    var track:RID = build_track("both", 10.0)
    var tracks:Dictionary[String, RID] = {"both": track}
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Start 0 0 none endmemcell "
        + "event both multiple 0 both mark condition trackoccupied trackfree endevent "
        + "event mark updatevalues 0 c1 Marked * * endevent", [], {}, tracks
    )
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")

    var condition:MaszynaEventCondition = ScenarioEventServer.event_get_condition(
            ScenarioEventServer.event_get_rid_by_name(&"both"))
    assert_eq(condition.track_test,
            MaszynaEventCondition.TRACK_TEST_OCCUPIED | MaszynaEventCondition.TRACK_TEST_FREE)
    if not await _run_event(&"both"):
        return
    await step(ticks(CHAINED_EVENT_SECONDS))
    assert_eq(MaszynaScenarioEventServer.memory_get_text(cell), "Start", "no track is occupied and free")
    root.free()
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


func test_logvalues_writes_its_cells_to_the_scenario_log() -> void:
    var handler:RecordingLogHandler = RecordingLogHandler.new()
    GameLog.register_handler("logvalues_test", handler)
    GameLog.assign_handler("scenario", "logvalues_test")
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Start 1 2 none endmemcell "
        + "event log_cell logvalues 0 c1 endevent", []
    )

    if await _run_event(&"log_cell"):
        assert_has(handler.lines, "Memcell c1: [Start] [1.00] [2.00]")
    GameLog.unassign_handler("scenario", "logvalues_test")
    GameLog.unregister_handler("logvalues_test")
    root.free()


func test_message_and_an_unknown_type_do_nothing() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "event say message 0 none hello endevent event odd nonsense 0 none endevent", []
    )

    assert_null(ScenarioEventServer.event_get_action(ScenarioEventServer.event_get_rid_by_name(&"say")))
    assert_null(ScenarioEventServer.event_get_action(ScenarioEventServer.event_get_rid_by_name(&"odd")))
    root.free()


func test_texture_sets_the_skin_with_the_cells_values() -> void:
    var instance:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    instance.model = MaszynaE3DModel.new()
    add_child_autoqfree(instance)
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = "board"
    var models:Array[ModelData] = [model_data]
    var instances:Dictionary[String, RID] = {"board": instance.get_model_instance()}
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Rudawa 5.4 0 none endmemcell "
        + "event show texture 0 board -2 make:scripts/board?do=(p1)&delay=(p2) c1 endevent", models, instances
    )

    if not await _run_event(&"show"):
        return
    var skins:PackedStringArray = MaszynaE3DRenderingServer.instance_get_skins(instance.get_model_instance())
    assert_eq(skins.size(), 2, "the second skin, -2 read as 2")
    assert_eq(skins[1], "make:scripts/board?do=Rudawa&delay=5", "the cell's text and its value as a whole number")
    root.free()


## A vehicle without a controller standing on its own track, named as the scenery names it - what
## a `whois` test queues its events by
func _create_whois_vehicle(track:RID) -> RID:
    var vehicle:RID = VehicleServer.vehicle_create()
    VehicleServer.vehicle_set_name(vehicle, "EP09-018")
    RailVehicleServer.vehicle_attach(vehicle)
    RailVehicleServer.vehicle_set_track(vehicle, track, 10.0, TrackServer.DIRECTION_NORMAL)
    return vehicle


## A driver aboard the vehicle, in a cab of its own, thinking with an implementation that does nothing
func _seat_driver(vehicle:RID) -> RID:
    var cabin:RID = VehicleServer.cabin_create()
    VehicleServer.vehicle_cabin_attach(vehicle, cabin)
    var person:RID = PersonServer.person_create()
    VehicleServer.cabin_person_enter(cabin, person, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    # a driver is a person thinking with an implementation (DriverServer.vehicle_get_driver())
    attach_driver_implementation(person, DriverImplementation.new())
    return person


func _run_event_by(event_name:StringName, activator:RID) -> bool:
    var event:RID = ScenarioEventServer.event_get_rid_by_name(event_name)
    ScenarioEventServer.event_queue(event, activator)
    return await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(event),
            ScenarioEventServer.event_get_delay(event) + TICK, "%s run" % event_name)


func test_whois_writes_the_vehicles_name_and_destination() -> void:
    var track:RID = build_track("whois", 100.0)
    var vehicle:RID = _create_whois_vehicle(track)
    var station:RID = MaszynaStations.station_of("Gdansk")
    RailVehicleServer.vehicle_set_destination(vehicle, station, track)
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 name memcell 0 0 0 nic 0 0 none endmemcell "
        + "node -1 0 dest memcell 0 0 0 nic 0 0 none endmemcell "
        + "event who_name whois 0 name 33 endevent "
        + "event who_dest whois 0 dest 15 endevent", []
    )
    var name_cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"name")
    var dest_cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"dest")

    if not await _run_event_by(&"who_name", vehicle) or not await _run_event_by(&"who_dest", vehicle):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(name_cell), "EP09-018", "+32: its name")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(dest_cell), "Gdansk:whois", "+8: its station and track")
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(dest_cell), 1.0, "facing its trainset's head")
    root.free()
    VehicleServer.vehicle_free(vehicle)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


func test_whois_writes_the_train_only_with_a_driver_and_moves_on_after_a_departure() -> void:
    var track:RID = build_track("whois_train", 100.0)
    var vehicle:RID = _create_whois_vehicle(track)
    var timetable:RID = StationServer.timetable_create()
    StationServer.timetable_set_train_name(timetable, "EIE1531")
    StationServer.timetable_add_entry(timetable, MaszynaStations.station_of("Alakowice"), StationServer.NO_TIME, 19.5)
    StationServer.timetable_add_entry(timetable, MaszynaStations.station_of("Baltyk Glowny"), StationServer.NO_TIME, 19.6)
    StationServer.timetable_add_entry(timetable, MaszynaStations.station_of("Baltyk Miasto"), 19.8, StationServer.NO_TIME)
    var run:RID = StationServer.run_create(timetable, RailVehicleServer.vehicle_get_trainset(vehicle))
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 train memcell 0 0 0 nic 0 0 none endmemcell "
        + "node -1 0 next memcell 0 0 0 nic 0 0 none endmemcell "
        + "event who_train whois 0 train 7 endevent "
        + "event who_next whois 0 next 47 endevent", []
    )
    var train_cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"train")
    var next_cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"next")

    if not await _run_event_by(&"who_train", vehicle):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(train_cell), "nic", "nothing without a driver")
    _seat_driver(vehicle)
    if not await _run_event_by(&"who_train", vehicle) or not await _run_event_by(&"who_next", vehicle):
        return
    # the train stands at its start, not left yet: Alakowice is its next station
    assert_eq(MaszynaScenarioEventServer.memory_get_text(train_cell), "EIE1531")
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(train_cell), 2.0, "two stations after it")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(next_cell), "Alakowice")
    StationServer.run_next_entry(run)
    if not await _run_event_by(&"who_train", vehicle) or not await _run_event_by(&"who_next", vehicle):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(train_cell), 1.0)
    assert_eq(MaszynaScenarioEventServer.memory_get_value2(train_cell), 0.0, "it passes Baltyk Glowny")
    assert_eq(MaszynaScenarioEventServer.memory_get_text(next_cell), "Baltyk Glowny")
    root.free()
    StationServer.run_free(run)
    StationServer.timetable_free(timetable)
    VehicleServer.vehicle_free(vehicle)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


## The dispatcher's queue of Bałtyk Główny (baltyk/mod/events_euro2012.ctr:2376-2402): the train
## registers itself with `whois`, and the next report finds it and gives the entry
func test_a_train_registered_by_whois_is_found_by_the_dispatchers_queue() -> void:
    var track:RID = build_track("whois_queue", 100.0)
    var vehicle:RID = _create_whois_vehicle(track)
    _seat_driver(vehicle)
    var timetable:RID = StationServer.timetable_create()
    StationServer.timetable_set_train_name(timetable, "EIE1531")
    StationServer.timetable_add_entry(timetable, MaszynaStations.station_of("Baltyk Glowny"), StationServer.NO_TIME, 19.6)
    var run:RID = StationServer.run_create(timetable, RailVehicleServer.vehicle_get_trainset(vehicle))
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 bzg_a_mem1 memcell 0 0 0 nic 0 0 none endmemcell "
        + "node -1 0 entry memcell 0 0 0 closed 0 0 none endmemcell "
        + "event bzg_a_mem1_wpisz whois 0 bzg_a_mem1 1 endevent "
        + "event bzg_a_zglos multiple 0 bzg_a_mem1 else bzg_a_1 condition memcompare nic * * endevent "
        + "event bzg_a_1 updatevalues 0 entry open * * endevent", []
    )
    var entry:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"entry")

    if not await _run_event_by(&"bzg_a_mem1_wpisz", vehicle) or not await _run_event(&"bzg_a_zglos"):
        return
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(entry) == "open",
            CHAINED_EVENT_SECONDS, "the entry"):
        return
    assert_eq(MaszynaScenarioEventServer.memory_get_text(entry), "open")
    root.free()
    StationServer.run_free(run)
    StationServer.timetable_free(timetable)
    VehicleServer.vehicle_free(vehicle)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


func test_an_addition_of_no_delay_runs_at_once_and_none_is_lost() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 count 0 0 none endmemcell "
        + "event count_up addvalues 0 c1 * 1 * endevent "
        + "event set_later updatevalues 0 c1 * 9 * endevent", []
    )
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")
    var count_up:RID = ScenarioEventServer.event_get_rid_by_name(&"count_up")
    var set_later:RID = ScenarioEventServer.event_get_rid_by_name(&"set_later")

    assert_true(MaszynaScenarioEventServer.event_add_to_query(count_up))
    assert_true(MaszynaScenarioEventServer.event_add_to_query(count_up))
    assert_eq(MaszynaScenarioEventServer.memory_get_value1(cell), 2.0, "both additions ran at once (Event.cpp:2388-2401)")
    assert_false(ScenarioEventServer.event_is_queued(count_up))
    MaszynaScenarioEventServer.event_add_to_query(set_later)
    assert_true(ScenarioEventServer.event_is_queued(set_later), "an update waits in the queue")
    root.free()


## A switch track named `switch_name`: straight along +X, its diverging curve to the side
func _build_switch(switch_name:String) -> RID:
    var common:TrackCurve = TrackCurve.new()
    common.p2 = Vector3(30.0, 0.0, 0.0)
    var diverging:TrackCurve = TrackCurve.new()
    diverging.p2 = Vector3(30.0, 0.0, 2.0)
    var track:RID = TrackServer.track_create()
    TrackServer.track_update_curves(track, common, diverging)
    TrackServer.track_update(track, TrackServer.TRACK_SWITCH, switch_name, SCENERY_TRACK_WIDTH)
    return track


func test_a_switch_event_sets_the_blade_speed_and_delay_and_sets_up_at_load() -> void:
    var track:RID = _build_switch("sw1")
    # built as it is: _build_scenery() makes its tracks plain ones
    var context:MaszynaImporterContext = _parse(
        "event sw1_init switch 0 sw1 0 -1 0.07 endevent "
        + "event sw1_throw switch 0 sw1 1 0.2 0.05 endevent"
    )
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var track_data:MaszynaTrackData = MaszynaTrackData.new()
    track_data.name = "sw1"
    var scenery_tracks:Array[TrackData] = [track_data]
    var track_rids:Array[RID] = [track]
    var models:Array[ModelData] = []
    var model_rids:Array[RID] = []
    var power_sources:Array[PowerSourceData] = []
    await MaszynaEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
        scenery_tracks, track_rids, models, model_rids, power_sources
    )
    var switch_rid:RID = TrackServer.track_get_switch(track)

    assert_almost_eq(TrackServer.switch_get_blade_delay(switch_rid), 0.07, 0.0001,
            "the common track with a delay is set up at load (Event.cpp:1834-1842)")
    if not await _run_event(&"sw1_throw"):
        return
    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_DIVERGING)
    assert_almost_eq(TrackServer.switch_get_blade_speed(switch_rid), 0.2, 0.0001)
    assert_almost_eq(TrackServer.switch_get_blade_delay(switch_rid), 0.05, 0.0001)
    root.free()
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


## ls_Dark from a `lights` event: the aspect keeps the light, the model's light follows the
## daylight (TAnimModel::LightSet(), AnimModel.cpp:664-670); the other light shows the aspect
func test_a_lights_event_sets_a_light_to_follow_the_daylight() -> void:
    var instance:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    instance.model = MaszynaE3DModel.new()
    add_child_autoqfree(instance)
    var signal_head:RID = SignallingServer.signal_head_create()
    instance.attach_signal_head(signal_head)
    SignallingServer.signal_head_set_name(signal_head, &"Lamp_A")
    var aspects:Dictionary = {&"dark": PackedFloat32Array([1.0, 3.0])}
    SignallingServer.signal_head_set_kind(signal_head, MaszynaSignalHeadKindFactory.create_kind(aspects))
    var system:RID = SignallingServer.system_create()
    SignallingServer.system_attach_implementation(system, MaszynaSignallingImplementation.new())
    SignallingServer.system_add_signal_head(system, signal_head)
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = "Lamp_A"
    var models:Array[ModelData] = [model_data]
    var instances:Dictionary[String, RID] = {"lamp_a": instance.get_model_instance()}
    var root:MaszynaIncludeNode = await _build_scenery("event lamp_a_dark lights 0 lamp_a 1 3 endevent", models, instances)

    if not await _run_event(&"lamp_a_dark"):
        return

    assert_eq(SignallingServer.signal_head_get_light_state(signal_head, 0), SignallingServer.LIGHT_STATE_ON)
    assert_eq(MaszynaE3DRenderingServer.instance_get_light_mode(instance.get_model_instance(), 1),
            MaszynaE3DRenderingServer.LIGHT_MODE_DARK)
    root.free()
    SignallingServer.system_free(system)


func test_shift_fires_the_first_event_of_a_launcher_without_a_second() -> void:
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())

    MaszynaScenarioEventServer.launcher_fire_shift(launcher)
    assert_true(ScenarioEventServer.event_is_queued(event), "launch_event(), scene.cpp:700-703")

    MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([event])


func test_an_interval_launcher_out_of_reach_does_not_fire() -> void:
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_set_radius(launcher, 10.0)
    MaszynaScenarioEventServer.launcher_set_position(launcher, Vector3(0.0, 0.0, 1.0e6))
    MaszynaScenarioEventServer.launcher_set_interval(launcher, TICK)

    await step(3)
    assert_false(ScenarioEventServer.event_is_queued(event), "an interval counts only in reach (scene.cpp:135-139)")

    MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([event])


func test_a_launcher_firing_once_fires_once_in_reach() -> void:
    var action:RecordingAction = RecordingAction.new()
    var event:RID = _create_event(action, 0.0)
    var launcher:RID = MaszynaScenarioEventServer.launcher_create(_event_system)
    MaszynaScenarioEventServer.launcher_set_events(launcher, event, RID())
    MaszynaScenarioEventServer.launcher_set_fire_once(launcher, true)

    await step(ticks(3.0 * FIRE_ONCE_CHECK_SECONDS))
    assert_eq(action.runs.size(), 1, "once, however long it stays in reach (EvLaunch.cpp:182-186)")

    MaszynaScenarioEventServer.launcher_free(launcher)
    _free_events([event])


func test_a_launcher_key_given_by_its_code() -> void:
    var context:MaszynaImporterContext = _parse(
        "node -1 0 capital eventlauncher 0 0 0 -1 65 0 none end "
        + "node -1 0 small eventlauncher 0 0 0 -1 98 0 none end"
    )
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var tracks:Array[TrackData] = []
    var track_rids:Array[RID] = []
    var models:Array[ModelData] = []
    var model_rids:Array[RID] = []
    var power_sources:Array[PowerSourceData] = []
    await MaszynaEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
        tracks, track_rids, models, model_rids, power_sources
    )
    var capital:RID = MaszynaScenarioEventServer.launcher_get_rid_by_name(&"capital")
    var small:RID = MaszynaScenarioEventServer.launcher_get_rid_by_name(&"small")

    assert_eq(MaszynaScenarioEventServer.launcher_get_key(capital), KEY_A)
    assert_true(MaszynaScenarioEventServer.launcher_get_key_needs_shift(capital), "a capital letter needs Shift")
    assert_eq(MaszynaScenarioEventServer.launcher_get_key(small), KEY_B)
    assert_false(MaszynaScenarioEventServer.launcher_get_key_needs_shift(small))
    root.free()


## eventfilter (TrkFoll.cpp:115-121, 131, 147): a trainset whose head has a driver fires the events
## of a move only going the way its head is set to go - a car nobody drives goes any way
func test_a_driven_trainset_fires_the_events_of_a_move_only_the_way_it_is_set_to_go() -> void:
    var action:RecordingAction = RecordingAction.new()
    var to_end:RID = _create_event(action, NEVER)
    var track:RID = build_track("eventfilter", 100.0)
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENTALL2, to_end)
    var driven:RailVehicle3D = build_rail_vehicle("Driven", "eventfilter", 20.0, MaszynaVehicleData.DriverType.DRIVER_HEAD)
    var loose:RailVehicle3D = build_rail_vehicle("Loose", "eventfilter", 60.0)
    await step(2)

    # standing, reported moving: not the way it is set to go
    TrackServer.track_report_vehicle_entered(track, driven.get_rid(), TrackServer.TRAVEL_TOWARD_END)
    assert_false(ScenarioEventServer.event_is_queued(to_end), "a driven trainset not going the way it is set")
    TrackServer.track_report_vehicle_entered(track, loose.get_rid(), TrackServer.TRAVEL_TOWARD_END)
    assert_true(ScenarioEventServer.event_is_queued(to_end), "a car nobody drives goes any way")

    _free_events([to_end])
    free_rail_vehicle(driven)
    free_rail_vehicle(loose)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


## A memory's command for a driver (TMemCell::CommandCheck(), StopCommandSent(), MemCell.cpp:52-99,
## 196-205): sent once, its `:sent` event queued then; a velocity is none; written again, it is one again
func test_a_memorys_command_is_sent_once_and_says_so() -> void:
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 Change_direction 0 0 none endmemcell "
        + "node -1 0 c2 memcell 0 0 0 SetVelocity 40 40 none endmemcell "
        + "event c1:sent updatevalues 5 c2 Sent * * endevent "
        + "event rearm updatevalues 0 c1 Change_direction * * endevent", []
    )
    var cell1:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")
    var cell2:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c2")
    var sent:RID = ScenarioEventServer.event_get_rid_by_name(&"c1:sent")

    assert_true(MaszynaScenarioEventServer.memory_has_command(cell1))
    assert_false(MaszynaScenarioEventServer.memory_has_command(cell2), "a velocity is read, not sent")
    MaszynaScenarioEventServer.memory_command_sent(cell1)
    assert_false(MaszynaScenarioEventServer.memory_has_command(cell1))
    assert_true(ScenarioEventServer.event_is_queued(sent), "its :sent event")
    if not await _run_event(&"rearm"):
        return
    assert_true(MaszynaScenarioEventServer.memory_has_command(cell1), "written again, a command again")
    root.free()


## A track of the scenery's name, laid straight from p1 to p2
func _build_named_track(
    track_name:String, p1:Vector3, p2:Vector3, kind:TrackServer.TrackType = TrackServer.TRACK_NORMAL
) -> RID:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    var track:RID = TrackServer.track_create()
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, kind, track_name, SCENERY_TRACK_WIDTH)
    return track


## The turntable `tt` between its approaches `in` and `out`, the sidings `left` and `right` at its
## quarter turn, and - with submodels - its model `tt` of them; the scenery of the text built over
## them, as loaded: the tracks connected first. Returns {root, tracks, instance}.
func _build_turntable_scenery(text:String, submodel_names:PackedStringArray) -> Dictionary:
    var along_z:Vector3 = Vector3(0.0, 0.0, TURNTABLE_HALF_LENGTH)
    var along_x:Vector3 = Vector3(TURNTABLE_HALF_LENGTH, 0.0, 0.0)
    var siding:float = TURNTABLE_SIDING_LENGTH / TURNTABLE_HALF_LENGTH
    var tracks:Dictionary[String, RID] = {
        "in": _build_named_track("in", TURNTABLE_PIVOT - along_z * (1.0 + siding), TURNTABLE_PIVOT - along_z),
        "tt": _build_named_track("tt", TURNTABLE_PIVOT - along_z, TURNTABLE_PIVOT + along_z, TrackServer.TRACK_TURN),
        "out": _build_named_track("out", TURNTABLE_PIVOT + along_z, TURNTABLE_PIVOT + along_z * (1.0 + siding)),
        "left": _build_named_track("left", TURNTABLE_PIVOT - along_x, TURNTABLE_PIVOT - along_x * (1.0 + siding)),
        "right": _build_named_track("right", TURNTABLE_PIVOT + along_x, TURNTABLE_PIVOT + along_x * (1.0 + siding)),
    }
    TrackServer.topology_rebuild()
    var models:Array[ModelData] = []
    var model_rids:Array[RID] = []
    var instance:MaszynaE3DModelInstance = null
    if submodel_names:
        instance = MaszynaE3DModelInstance.new()
        var model:MaszynaE3DModel = MaszynaE3DModel.new()
        var submodels:Array[MaszynaE3DSubModel] = []
        for submodel_name:String in submodel_names:
            var submodel:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
            submodel.resource_name = submodel_name
            submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
            submodels.append(submodel)
        model.submodels = submodels
        instance.model = model
        add_child_autoqfree(instance)
        var model_data:MaszynaModelData = MaszynaModelData.new()
        model_data.name = "tt"
        models.append(model_data)
        model_rids.append(instance.get_model_instance())
    var scenery_tracks:Array[TrackData] = []
    var track_rids:Array[RID] = []
    for track_name:String in tracks:
        var track_data:MaszynaTrackData = MaszynaTrackData.new()
        track_data.name = track_name
        track_data.kind = TrackServer.TRACK_TURN if track_name == "tt" else TrackServer.TRACK_NORMAL
        scenery_tracks.append(track_data)
        track_rids.append(tracks[track_name])
    var context:MaszynaImporterContext = _parse(text)
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var power_sources:Array[PowerSourceData] = []
    await MaszynaEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
        scenery_tracks, track_rids, models, model_rids, power_sources
    )
    return {"root": root, "tracks": tracks, "instance": instance}


func _free_turntable_scenery(scenery:Dictionary) -> void:
    (scenery.root as Node).free()
    for track:RID in (scenery.tracks as Dictionary).values():
        TrackServer.track_free(track)
    TrackServer.topology_rebuild()


## The tracks an end of the track meets
func _met(track:RID, end:TrackServer.EndpointIndex) -> Array[RID]:
    var met:Array[RID] = []
    for connection:TrackEndpointRef in TrackServer.track_get_endpoint_connections(track, end):
        met.append(connection.track_rid)
    return met


## The cze_obrotnica.inc sequence: `switch 1` unlocks the turntable, the model turns, its `:done`
## locks it with `switch 0`, and its `:joined` confirms it (Track.cpp:1787-1833, 2024-2061)
func test_a_turntable_turns_with_its_model_and_joins_the_tracks_it_meets() -> void:
    var scenery:Dictionary = await _build_turntable_scenery(
        "node -1 0 c1 memcell 0 0 0 none 0 0 none endmemcell "
        + "event tt_unlock switch 0 tt 1 endevent "
        + "event tt_turn animation 0 tt rotate dzwigar 0 0 %.0f %.0f endevent " % [QUARTER_TURN, TURN_SPEED]
        + "event tt.dzwigar:done switch 0 tt 0 endevent "
        + "event tt:joined updatevalues 0 c1 joined * * endevent",
        PackedStringArray(["dzwigar"])
    )
    var tracks:Dictionary = scenery.tracks
    var cell:RID = MaszynaScenarioEventServer.memory_get_rid_by_name(&"c1")
    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P1), [tracks["in"]] as Array[RID], "connected as it was loaded")

    if not await _run_event(&"tt_unlock"):
        return
    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P1), [] as Array[RID], "off its neighbours")
    assert_eq(_met(tracks["in"], TrackServer.CURVE1_P2), [] as Array[RID])
    assert_eq(TrackServer.track_get_velocity(tracks.tt), 0.0, "no traffic on it")

    if not await _run_event(&"tt_turn"):
        return
    if not await wait_simulated_until(func() -> bool: return MaszynaScenarioEventServer.memory_get_text(cell) == "joined",
            QUARTER_TURN / TURN_SPEED + CHAINED_EVENT_SECONDS + TICK, "the turntable joined"):
        return

    var ends:PackedVector3Array = TrackServer.track_get_endpoints(tracks.tt)
    assert_almost_eq(ends[0], TURNTABLE_PIVOT - Vector3(TURNTABLE_HALF_LENGTH, 0.0, 0.0), Vector3.ONE * 0.001,
            "turned a quarter with the submodel")
    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P1), [tracks.left] as Array[RID], "joined where it turned to")
    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P2), [tracks.right] as Array[RID])
    assert_eq(TrackServer.track_get_velocity(tracks.tt), TURNTABLE_JOINED_VELOCITY, "traffic let on")
    _free_turntable_scenery(scenery)


## The track follows the model's first animated submodel - in cze_obrotnica.inc the signal's arm,
## animated before the bridge - so turning another one leaves it where it was
## (TAnimModel::GetContainer(), AnimModel.cpp:486-489)
func test_a_turntable_follows_only_the_first_animated_submodel() -> void:
    var scenery:Dictionary = await _build_turntable_scenery(
        "event tt_unlock switch 0 tt 1 endevent "
        + "event tt_stop animation 0 tt rotate wz1 0 0 0 %.0f endevent " % TURN_SPEED
        + "event tt_turn animation 0 tt rotate dzwigar 0 0 %.0f %.0f endevent " % [QUARTER_TURN, TURN_SPEED]
        + "event tt.dzwigar:done switch 0 tt 0 endevent",
        PackedStringArray(["wz1", "dzwigar"])
    )
    var tracks:Dictionary = scenery.tracks

    if not await _run_event(&"tt_unlock"):
        return
    if not await _run_event(&"tt_turn"):
        return
    if not await wait_simulated_until(func() -> bool: return _met(tracks.tt, TrackServer.CURVE1_P1).size() > 0,
            QUARTER_TURN / TURN_SPEED + CHAINED_EVENT_SECONDS + TICK, "the turntable locked"):
        return

    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P1), [tracks["in"]] as Array[RID], "joined where it stood")
    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P2), [tracks.out] as Array[RID])
    _free_turntable_scenery(scenery)


## A turntable of no model is a derailer: left off its neighbours as it is loaded
## (path_table::InitTracks(), Track.cpp:3418-3423)
func test_a_turntable_of_no_model_is_left_off_its_neighbours() -> void:
    var scenery:Dictionary = await _build_turntable_scenery("", PackedStringArray())
    var tracks:Dictionary = scenery.tracks

    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P1), [] as Array[RID])
    assert_eq(_met(tracks.tt, TrackServer.CURVE1_P2), [] as Array[RID])
    _free_turntable_scenery(scenery)


## A track takes the events named after it and the slot only with bit 1 of `hiddenevents`, the last
## `config` of it read (TTrack::AssignEvents(), Track.cpp:968-980; Globals.cpp:674-677)
func test_hiddenevents_decides_whether_a_track_takes_the_events_named_after_it() -> void:
    var text:String = ("node -1 0 c1 memcell 0 0 0 none 0 0 none endmemcell "
            + "config hiddenevents 0 endconfig "
            + "event t1:eventall0 updatevalues 0 c1 seen * * endevent")
    assert_eq(_parse(text).hidden_events, 0, "the config's value")
    assert_eq(_parse(text + " config hiddenevents 1 endconfig").hidden_events, 1, "the last one read")
    for hidden:int in [0, MaszynaImporterContext.DEFAULT_HIDDEN_EVENTS]:
        var track:RID = _build_named_track("t1", Vector3.ZERO, Vector3(NAMED_TRACK_LENGTH, 0.0, 0.0))
        var context:MaszynaImporterContext = _parse(text)
        var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
        root.autoload = false
        add_child(root)
        var track_data:MaszynaTrackData = MaszynaTrackData.new()
        track_data.name = "t1"
        var scenery_tracks:Array[TrackData] = [track_data]
        var track_rids:Array[RID] = [track]
        var models:Array[ModelData] = []
        var model_rids:Array[RID] = []
        var power_sources:Array[PowerSourceData] = []
        var no_rids:Array[RID] = []
        var traction:Array[TractionData] = []
        await MaszynaEventFactory.build(
            root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
            scenery_tracks, track_rids, models, model_rids, power_sources, no_rids, traction, no_rids, hidden
        )
        assert_eq(MaszynaScenarioEventServer.track_get_events(track, MaszynaScenarioEventServer.TRACK_EVENTALL0).size(),
                0 if hidden == 0 else 1, "hiddenevents %d" % hidden)
        root.free()
        TrackServer.track_free(track)
    TrackServer.topology_rebuild()


## `queueevent <event> [vehicle]` queues the event of the name by the vehicle of the name, as the
## original's command does (user_command::queueevent, simulation.cpp:293-307)
func test_the_queueevent_command_queues_the_event_by_the_vehicle() -> void:
    var action:RecordingAction = RecordingAction.new()
    var event:RID = _create_event(action, 0.0)
    ScenarioEventServer.event_set_name(event, &"console_event")
    var vehicle:RID = VehicleServer.vehicle_create()
    VehicleServer.vehicle_set_id(vehicle, "sm42-001")

    MaszynaConsoleCommands.queue_event("Console_Event", "nobody")
    assert_false(ScenarioEventServer.event_is_queued(event), "no vehicle of that name")
    MaszynaConsoleCommands.queue_event("Console_Event", "SM42-001")
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, TICK, "the event run"):
        return
    assert_eq(action.activators[0], vehicle, "by the vehicle named")

    _free_events([event])
    VehicleServer.vehicle_free(vehicle)
