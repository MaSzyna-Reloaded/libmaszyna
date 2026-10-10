extends MaszynaGutTest

const EventImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_event_importer.gd")
const NodeImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_importer.gd")
const IsolatedImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_isolated_importer.gd")
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
## `departuredelay`: an event of this delay [s] and this departure delay [s], queued by a train that
## departs this many seconds from now; run times compared within this [s]
const EVENT_DELAY:float = 5.0
const DEPARTURE_DELAY:float = -30.0
const UNTIL_DEPARTURE:float = 600.0
const RUN_TIME_EPSILON:float = 0.01


class RecordingAction extends ScenarioEventAction:
    var runs:Array[RID] = []
    var else_runs:Array[RID] = []
    var activators:Array[RID] = []

    func _run(event:RID, activator:RID) -> void:
        runs.append(event)
        activators.append(activator)

    func _run_else(event:RID, _activator:RID) -> void:
        else_runs.append(event)


## A driver whose train departs this many seconds from any time
class DepartingDriver extends DriverImplementation:
    var seconds:float = 0.0

    func _get_seconds_until_departure(_driver:RID, _hours:float) -> float:
        return seconds


class RequeueingAction extends ScenarioEventAction:
    var count:int = 0

    func _run(event:RID, _activator:RID) -> void:
        count += 1
        ScenarioEventServer.event_queue(event)


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
    var activator:RID = ScenarioEventServer.memory_create()
    var other_activator:RID = ScenarioEventServer.memory_create()

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
    ScenarioEventServer.memory_free(activator)
    ScenarioEventServer.memory_free(other_activator)


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
    var memory:RID = ScenarioEventServer.memory_create()
    var chosen:RID = _create_event(action, 0.0)
    var otherwise:RID = _create_event(action, 0.0)
    var multiple_action:MaszynaLegacyMultipleAction = MaszynaLegacyMultipleAction.new()
    var events:Array[RID] = [chosen]
    var else_events:Array[RID] = [otherwise]
    multiple_action.events = events
    multiple_action.else_events = else_events
    var multiple:RID = _create_event(multiple_action, 0.0)
    var condition:MaszynaLegacyEventCondition = MaszynaLegacyEventCondition.new()
    var memories:Array[RID] = [memory]
    condition.memories = memories
    condition.text = "go*"
    condition.mask = ScenarioEventServer.MEMORY_FIELD_TEXT
    ScenarioEventServer.event_attach_condition(multiple, condition)
    var activator:RID = ScenarioEventServer.memory_create()

    ScenarioEventServer.memory_set_values(memory, "go_ahead", 0.0, 0.0)
    ScenarioEventServer.event_queue(multiple, activator)
    # the multiple runs on one pass of the queue, the event it queues on the next
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, CHAINED_EVENT_SECONDS,
            "the chosen event run"):
        return
    ScenarioEventServer.memory_set_values(memory, "stop", 0.0, 0.0)
    ScenarioEventServer.event_queue(multiple)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 2, CHAINED_EVENT_SECONDS,
            "the else event run"):
        return

    var expected:Array[RID] = [chosen, otherwise]
    assert_eq(action.runs, expected, "a text with * should be compared up to it")
    assert_eq(action.activators[0], activator, "multiple should hand its activator on")
    _free_events([chosen, otherwise, multiple])
    ScenarioEventServer.memory_free(memory)
    ScenarioEventServer.memory_free(activator)


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
    var memory:RID = ScenarioEventServer.memory_create()
    var condition:MaszynaLegacyEventCondition = MaszynaLegacyEventCondition.new()
    var memories:Array[RID] = [memory]
    condition.memories = memories
    condition.value1 = 1.0
    condition.mask = ScenarioEventServer.MEMORY_FIELD_VALUE1
    var launcher:RID = ScenarioEventServer.launcher_create()
    ScenarioEventServer.launcher_set_events(launcher, event, RID())
    ScenarioEventServer.launcher_attach_condition(launcher, condition)

    ScenarioEventServer.launcher_fire(launcher)
    assert_false(ScenarioEventServer.event_is_queued(event))
    ScenarioEventServer.memory_set_values(memory, "", 1.0, 0.0)
    ScenarioEventServer.launcher_fire(launcher)
    assert_true(ScenarioEventServer.event_is_queued(event))

    ScenarioEventServer.launcher_free(launcher)
    ScenarioEventServer.memory_free(memory)
    _free_events([event])


func test_a_radio_call_fires_the_launchers_listening_in_range() -> void:
    var near:RID = _create_event(RecordingAction.new(), NEVER)
    var far:RID = _create_event(RecordingAction.new(), NEVER)
    var other_call:RID = _create_event(RecordingAction.new(), NEVER)
    var launchers:Array[RID] = []
    for setup:Array in [[near, RailVehicleRadio.RADIO_CALL3, Vector3.ZERO], [far, RailVehicleRadio.RADIO_CALL3, Vector3(500, 0, 0)],
            [other_call, RailVehicleRadio.RADIO_CALL1, Vector3.ZERO]]:
        var launcher:RID = ScenarioEventServer.launcher_create()
        ScenarioEventServer.launcher_set_events(launcher, setup[0], RID())
        ScenarioEventServer.launcher_set_radio_call(launcher, setup[1])
        ScenarioEventServer.launcher_set_position(launcher, setup[2])
        ScenarioEventServer.launcher_set_radius(launcher, 100.0)
        launchers.append(launcher)

    RailVehicleServer.vehicle_radio_called.emit(RID(), RailVehicleRadio.RADIO_CALL3, Vector3(10, 0, 0))

    assert_true(ScenarioEventServer.event_is_queued(near), "call 3 within 100 m")
    assert_false(ScenarioEventServer.event_is_queued(far), "out of range")
    assert_false(ScenarioEventServer.event_is_queued(other_call), "listens to call 1")
    for launcher:RID in launchers:
        ScenarioEventServer.launcher_free(launcher)
    _free_events([near, far, other_call])


func test_putvalues_cab_signal_reaches_the_security_system() -> void:
    var controller:VehicleController = build_vehicle("CabSignalTest", load("res://tests/fixtures/sm42_vehicle.tres"))
    var vehicle:RID = controller.get_rid()
    var action:MaszynaLegacyVehicleCommandAction = MaszynaLegacyVehicleCommandAction.new()
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
    var vehicle:RID = build_vehicle(
            "DepartureDelayTest", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD).get_rid()
    var departing:DepartingDriver = DepartingDriver.new()
    departing.seconds = UNTIL_DEPARTURE
    attach_driver_implementation(get_vehicle_driver(vehicle), departing)
    var event:RID = _create_event(RecordingAction.new(), EVENT_DELAY)
    ScenarioEventServer.event_set_departure_delay(event, DEPARTURE_DELAY)
    var now:float = SimulationServer.simulation_get_time()

    ScenarioEventServer.event_queue(event, vehicle)
    assert_almost_eq(ScenarioEventServer.event_get_run_time(event), now + EVENT_DELAY + UNTIL_DEPARTURE + DEPARTURE_DELAY,
            RUN_TIME_EPSILON)
    ScenarioEventServer.event_free(event)

    event = _create_event(RecordingAction.new(), EVENT_DELAY)
    ScenarioEventServer.event_set_departure_delay(event, DEPARTURE_DELAY)
    ScenarioEventServer.event_queue(event)
    assert_almost_eq(ScenarioEventServer.event_get_run_time(event), now + EVENT_DELAY, RUN_TIME_EPSILON,
            "no train, no departure")
    ScenarioEventServer.event_free(event)

    event = _create_event(RecordingAction.new(), EVENT_DELAY)
    ScenarioEventServer.event_set_departure_delay(event, DEPARTURE_DELAY)
    departing.seconds = -UNTIL_DEPARTURE
    ScenarioEventServer.event_queue(event, vehicle)
    assert_almost_eq(ScenarioEventServer.event_get_run_time(event), now, RUN_TIME_EPSILON, "departed long ago: at once")
    ScenarioEventServer.event_free(event)


func test_a_launcher_fires_when_the_clock_shows_its_time() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = ScenarioEventServer.launcher_create()
    ScenarioEventServer.launcher_set_events(launcher, event, RID())
    ScenarioEventServer.launcher_set_time_of_day(launcher, 10, 50)

    SimulationServer.time_of_day = 10.5
    assert_false(ScenarioEventServer.event_is_queued(event), "10:30 is not its time")
    SimulationServer.time_of_day = 10.0 + 50.5 / 60.0
    assert_true(ScenarioEventServer.event_is_queued(event), "10:50 is")

    ScenarioEventServer.launcher_free(launcher)
    _free_events([event])
    SimulationServer.time_of_day = clock


## 21 + 5/60 is 21.08333..., whose minutes truncated were 4 - a launcher on a scenario's start
## minute never fired
func test_a_launcher_fires_at_a_start_minute_the_clock_holds_inexactly() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = ScenarioEventServer.launcher_create()
    ScenarioEventServer.launcher_set_events(launcher, event, RID())
    ScenarioEventServer.launcher_set_time_of_day(launcher, 21, 5)

    SimulationServer.time_of_day = 21.0 + 5.0 / 60.0
    assert_true(ScenarioEventServer.event_is_queued(event))

    ScenarioEventServer.launcher_free(launcher)
    _free_events([event])
    SimulationServer.time_of_day = clock


## The original looks at the condition all through the minute (Event.cpp:2293-2306)
func test_a_launcher_waits_within_its_minute_for_its_condition() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var memory:RID = ScenarioEventServer.memory_create()
    var condition:MaszynaLegacyEventCondition = MaszynaLegacyEventCondition.new()
    var memories:Array[RID] = [memory]
    condition.memories = memories
    condition.value1 = 1.0
    condition.mask = ScenarioEventServer.MEMORY_FIELD_VALUE1
    var launcher:RID = ScenarioEventServer.launcher_create()
    ScenarioEventServer.launcher_set_events(launcher, event, RID())
    ScenarioEventServer.launcher_attach_condition(launcher, condition)
    ScenarioEventServer.launcher_set_time_of_day(launcher, 10, 50)

    SimulationServer.time_of_day = 10.0 + 50.25 / 60.0
    assert_false(ScenarioEventServer.event_is_queued(event), "its condition does not pass yet")
    ScenarioEventServer.memory_set_values(memory, "", 1.0, 0.0)
    assert_true(ScenarioEventServer.event_is_queued(event), "it passes within the minute")

    ScenarioEventServer.launcher_free(launcher)
    ScenarioEventServer.memory_free(memory)
    _free_events([event])
    SimulationServer.time_of_day = clock


## Only a launcher of a negative radius is global; the rest fire near the camera (scene.cpp:126-139)
func test_a_timed_launcher_of_a_radius_fires_only_near_the_camera() -> void:
    var clock:float = SimulationServer.time_of_day
    var event:RID = _create_event(RecordingAction.new(), NEVER)
    var launcher:RID = ScenarioEventServer.launcher_create()
    ScenarioEventServer.launcher_set_events(launcher, event, RID())
    ScenarioEventServer.launcher_set_radius(launcher, 10.0)
    ScenarioEventServer.launcher_set_position(launcher, Vector3(0.0, 0.0, 1.0e6))
    ScenarioEventServer.launcher_set_time_of_day(launcher, 10, 50)

    SimulationServer.time_of_day = 10.0 + 50.5 / 60.0
    assert_false(ScenarioEventServer.event_is_queued(event))

    ScenarioEventServer.launcher_free(launcher)
    _free_events([event])
    SimulationServer.time_of_day = clock


func test_a_passenger_stop_is_named_as_the_timetable_names_it() -> void:
    var models:Array[MaszynaModelData] = []
    var root:MaszynaIncludeNode = await _build_scenery(
        "event w4_stopinfo putvalues 0 none 1 2 3 PassengerStopPoint:Jawor#2 -4 151 endevent", models
    )
    var event:RID = ScenarioEventServer.event_get_rid_by_name(&"w4_stopinfo")
    var action:MaszynaLegacyVehicleCommandAction = ScenarioEventServer.event_get_action(event)

    assert_eq(action.command, "PassengerStopPoint:Jawor", "unique only past its #")
    assert_true(ScenarioEventServer.event_is_passive(event), "read by the drivers ahead, never queued")
    root.free()


func test_a_departure_delay_is_read_from_the_scenery() -> void:
    var models:Array[MaszynaModelData] = []
    var root:MaszynaIncludeNode = await _build_scenery("event odjazd multiple 2 none endevent "
            + "event odjazd_signal multiple 0 none departuredelay %s endevent" % DEPARTURE_DELAY, models)

    assert_eq(ScenarioEventServer.event_get_departure_delay(ScenarioEventServer.event_get_rid_by_name(&"odjazd_signal")),
            DEPARTURE_DELAY)
    assert_true(is_nan(ScenarioEventServer.event_get_departure_delay(ScenarioEventServer.event_get_rid_by_name(&"odjazd"))),
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
    var cell1:RID = ScenarioEventServer.memory_get_rid_by_name(&"Cell1")
    var cell2:RID = ScenarioEventServer.memory_get_rid_by_name(&"cell2")
    assert_eq(ScenarioEventServer.memory_get_text(cell1), "Start")

    if not await _run_event(&"set_cell"):
        return
    assert_eq(ScenarioEventServer.memory_get_text(cell1), "Go", "the text keeps its case")
    assert_eq(ScenarioEventServer.memory_get_value1(cell1), 1.0, "* should leave the value")
    assert_eq(ScenarioEventServer.memory_get_value2(cell1), 5.0)

    if not await _run_event(&"add_cell"):
        return
    assert_eq(ScenarioEventServer.memory_get_text(cell1), "Go_x")
    assert_eq(ScenarioEventServer.memory_get_value1(cell1), 2.0)
    assert_eq(ScenarioEventServer.memory_get_value2(cell1), 5.0)

    if not await _run_event(&"copy_cell"):
        return
    assert_eq(ScenarioEventServer.memory_get_text(cell2), "Other", "mask 2 copies only value 1")
    assert_eq(ScenarioEventServer.memory_get_value1(cell2), 2.0)
    assert_eq(ScenarioEventServer.memory_get_value2(cell2), 8.0)
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
    var instance:E3DModelInstance = E3DModelInstance.new()
    instance.model = E3DModel.new()
    add_child_autoqfree(instance)
    var signal_head:RID = SignallingServer.signal_head_create(instance.get_e3d_instance())
    SignallingServer.signal_head_set_name(signal_head, &"Sem_A")
    var aspects:Dictionary = {&"sem_ligh1": PackedFloat32Array([1.0])}
    SignallingServer.signal_head_set_kind(signal_head, MaszynaLegacySignalHeadKindFactory.create_kind(aspects))
    var system:RID = SignallingServer.system_create()
    SignallingServer.system_attach_implementation(system, MaszynaLegacySignallingImplementation.new())
    SignallingServer.system_add_signal_head(system, signal_head)
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = "Sem_A"
    var models:Array[MaszynaModelData] = [model_data]
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
    var track:RID = ScenarioEventServer.memory_create() # stands in for a track
    var other_track:RID = ScenarioEventServer.memory_create()
    var vehicle:RID = ScenarioEventServer.memory_create() # and for a vehicle
    ScenarioEventServer.track_add_event(track, ScenarioEventServer.TRACK_EVENTALL2, to_end)
    ScenarioEventServer.track_add_event(track, ScenarioEventServer.TRACK_EVENTALL1, to_start)

    RailVehicleServer.vehicle_heading_to_track_end.emit(vehicle, track)
    assert_true(ScenarioEventServer.event_is_queued(to_end))
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, TICK,
            "the track event run"):
        return
    RailVehicleServer.vehicle_stopped_on_track.emit(vehicle, track)
    RailVehicleServer.vehicle_heading_to_track_end.emit(vehicle, track)
    assert_false(ScenarioEventServer.event_is_queued(to_end), "once per entry")
    RailVehicleServer.vehicle_heading_to_track_start.emit(vehicle, track)
    assert_true(ScenarioEventServer.event_is_queued(to_start), "the other direction once too")
    RailVehicleServer.vehicle_heading_to_track_end.emit(vehicle, other_track)
    RailVehicleServer.vehicle_heading_to_track_end.emit(vehicle, track)
    assert_true(ScenarioEventServer.event_is_queued(to_end), "a new entry fires again")

    ScenarioEventServer.track_clear_events(track)
    _free_events([to_end, to_start])
    for rid:RID in [track, other_track, vehicle]:
        ScenarioEventServer.memory_free(rid)


func test_a_standing_event_goes_on_while_the_vehicle_stands() -> void:
    var action:RecordingAction = RecordingAction.new()
    var standing:RID = _create_event(action, 0.0)
    var track:RID = ScenarioEventServer.memory_create()
    var vehicle:RID = ScenarioEventServer.memory_create()
    ScenarioEventServer.track_add_event(track, ScenarioEventServer.TRACK_EVENTALL0, standing)

    RailVehicleServer.vehicle_stopped_on_track.emit(vehicle, track)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() >= 2, CHAINED_EVENT_SECONDS,
            "the standing event run again"):
        return
    assert_gt(action.runs.size(), 1, "queued again after its run")
    RailVehicleServer.vehicle_heading_to_track_end.emit(vehicle, track)
    if not await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(standing), TICK,
            "the standing event let go"):
        return
    var runs:int = action.runs.size()
    await get_tree().process_frame
    assert_eq(action.runs.size(), runs, "not once the vehicle moves")

    ScenarioEventServer.track_clear_events(track)
    _free_events([standing])
    ScenarioEventServer.memory_free(track)
    ScenarioEventServer.memory_free(vehicle)


func test_scenery_animation_turns_the_submodel() -> void:
    var instance:E3DModelInstance = E3DModelInstance.new()
    var model:E3DModel = E3DModel.new()
    var arm:E3DSubModel = E3DSubModel.new()
    arm.resource_name = "Ramie01"
    arm.submodel_type = E3DSubModel.SUBMODEL_TRANSFORM
    var submodels:Array[E3DSubModel] = [arm]
    model.submodels = submodels
    instance.model = model
    add_child_autoqfree(instance)
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = "rog1"
    var models:Array[MaszynaModelData] = [model_data]
    var instances:Dictionary[String, RID] = {"rog1": instance.get_e3d_instance()}
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 moving 0 0 none endmemcell "
        + "event rog1on animation 0 rog1 rotate ramie01 0 0 %.0f %.0f endevent " % [QUARTER_TURN, TURN_SPEED]
        + "event rog1.ramie01:done updatevalues 0 c1 done * * endevent",
        models, instances
    )
    var arm_node:Node3D = instance.find_child("Ramie01", true, false)
    var cell:RID = ScenarioEventServer.memory_get_rid_by_name(&"c1")

    if not await _run_event(&"rog1on"):
        return
    # the arm turns on the simulation's clock at its speed - the quarter turn in QUARTER_TURN / TURN_SPEED
    # simulated seconds, its :done event on the step it arrives
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_text(cell) == "done",
            QUARTER_TURN / TURN_SPEED + TICK, "the animation's arrival"):
        return

    assert_true(arm_node.basis.is_equal_approx(Basis(Vector3(0, 0, 1), deg_to_rad(QUARTER_TURN))), "turned by 90 degrees about z")
    assert_eq(ScenarioEventServer.memory_get_text(cell), "done", "the :done event runs when it arrives")
    root.free()


func test_scenery_voltage_event_sets_the_power_source() -> void:
    var power_source:RID = TractionServer.power_source_create()
    TractionServer.power_source_set_params(power_source, "Pwr1", 3000.0, 0.0, 0.2, 1000.0, 1.0, 3, 60.0, false)
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var power_source_data:MaszynaPowerSourceData = MaszynaPowerSourceData.new()
    power_source_data.name = "Pwr1"
    var power_sources:Array[MaszynaPowerSourceData] = [power_source_data]
    var context:MaszynaImporterContext = _parse("event keyctrl05 voltage 0.1 pwr1 2400 endevent")
    var tracks:Array[RID] = []
    var models:Array[MaszynaModelData] = []
    var model_rids:Array[RID] = []
    var isolated_sections:Array[MaszynaIsolatedData] = []
    await MaszynaLegacyEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, isolated_sections, context.tracks,
        tracks, models, model_rids, power_sources
    )

    if not await _run_event(&"keyctrl05"):
        return

    assert_eq(TractionServer.power_source_get_nominal_voltage(power_source), 2400.0)
    root.free()
    TractionServer.power_source_free(power_source)


func test_shift_and_a_digit_queue_the_keyctrl_event() -> void:
    var keyboard:ScenarioKeyboard = ScenarioKeyboard.new()
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
    var vehicle:RID = ScenarioEventServer.memory_create() # stands in for a vehicle
    var tracks:Dictionary[String, RID] = {"t1": track}
    var models:Array[MaszynaModelData] = []
    var root:MaszynaIncludeNode = await _build_scenery(
        "node -1 0 c1 memcell 0 0 0 idle 0 0 none endmemcell "
        + "isolated s1 t1 endisolated "
        + "event s1:busy updatevalues 0 c1 busy * * endevent",
        models, {}, tracks
    )
    var section:RID = TrackServer.isolated_get_rid_by_name(&"s1")
    var cell:RID = ScenarioEventServer.memory_get_rid_by_name(&"c1")
    var own_memory:RID = ScenarioEventServer.memory_get_rid_by_name(&"s1")
    assert_true(own_memory.is_valid(), "a section has a memory of its name")

    TrackServer.track_vehicle_entered(track, vehicle)
    assert_true(TrackServer.isolated_is_occupied(section))
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_text(cell) == "busy", TICK,
            "the busy event run"):
        return
    assert_eq(ScenarioEventServer.memory_get_text(cell), "busy")
    assert_eq(int(ScenarioEventServer.memory_get_value2(own_memory)) & 1, 1, "value 2 made odd")

    TrackServer.track_vehicle_left(track, vehicle)
    assert_false(TrackServer.isolated_is_occupied(section))
    if not await wait_simulated_until(func() -> bool: return ScenarioEventServer.memory_get_value2(own_memory) == 0.0, TICK,
            "the section's memory cleared"):
        return
    assert_eq(ScenarioEventServer.memory_get_value2(own_memory), 0.0, "the low byte cleared")
    root.free()
    TrackServer.track_free(track)
    ScenarioEventServer.memory_free(vehicle)


func test_memcompareex_and_track_tests() -> void:
    var memory:RID = ScenarioEventServer.memory_create()
    ScenarioEventServer.memory_set_values(memory, "b", 5.0, 0.0)
    var memories:Array[RID] = [memory]
    var condition:MaszynaLegacyEventCondition = MaszynaLegacyEventCondition.new()
    condition.memories = memories
    condition.text = "a"
    condition.text_operator = MaszynaLegacyEventCondition.OPERATOR_GREATER
    condition.value1 = 3.0
    condition.value1_operator = MaszynaLegacyEventCondition.OPERATOR_LESS
    condition.mask = ScenarioEventServer.MEMORY_FIELD_TEXT | ScenarioEventServer.MEMORY_FIELD_VALUE1
    var action:RecordingAction = RecordingAction.new()
    var event:RID = _create_event(action, 0.0)
    ScenarioEventServer.event_attach_condition(event, condition)

    condition.pass = MaszynaLegacyEventCondition.PASS_ANY
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 1, TICK,
            "the event run"):
        return
    condition.pass = MaszynaLegacyEventCondition.PASS_ALL
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
    condition.track_test = MaszynaLegacyEventCondition.TRACK_TEST_OCCUPIED
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.else_runs.size() == 2, TICK,
            "the else events run for an empty track"):
        return
    TrackServer.track_vehicle_entered(track, memory)
    ScenarioEventServer.event_queue(event)
    if not await wait_simulated_until(func() -> bool: return action.runs.size() == 2, TICK,
            "the events run for an occupied track"):
        return
    assert_eq(action.else_runs.size(), 2, "an empty track is not occupied")
    assert_eq(action.runs.size(), 2, "a track with a vehicle is")

    TrackServer.track_vehicle_left(track, memory)
    TrackServer.track_free(track)
    _free_events([event])
    ScenarioEventServer.memory_free(memory)


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


func _create_event(action:ScenarioEventAction, delay:float) -> RID:
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.event_set_delay(event, delay)
    ScenarioEventServer.event_attach_action(event, action)
    return event


func _free_events(events:Array[RID]) -> void:
    for event:RID in events:
        ScenarioEventServer.event_free(event)


## Parses the scenery text and builds it through MaszynaLegacyEventFactory; the returned include
## frees what was built when it is freed
func _build_scenery(
    text:String, models:Array[MaszynaModelData], model_instances:Dictionary[String, RID] = {},
    tracks:Dictionary[String, RID] = {}
) -> MaszynaIncludeNode:
    var context:MaszynaImporterContext = _parse(text)
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var model_rids:Array[RID] = []
    for model_data:MaszynaModelData in models:
        model_rids.append(model_instances.get(model_data.name, RID()))
    var scenery_tracks:Array[MaszynaTrackData] = []
    var track_rids:Array[RID] = []
    for track_name:String in tracks:
        var track_data:MaszynaTrackData = MaszynaTrackData.new()
        track_data.track_name = track_name
        scenery_tracks.append(track_data)
        track_rids.append(tracks[track_name])
    var power_sources:Array[MaszynaPowerSourceData] = []
    await MaszynaLegacyEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
        scenery_tracks, track_rids, models, model_rids, power_sources
    )
    return root


func _parse(text:String) -> MaszynaImporterContext:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(text.to_utf8_buffer())
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    parser.register_handler("event", func(p:MaszynaParser) -> Array: return EventImporter.new().import(p, context))
    parser.register_handler("node", func(p:MaszynaParser) -> Array: return NodeImporter.new().import(p, context))
    parser.register_handler("isolated", func(p:MaszynaParser) -> Array: return IsolatedImporter.new().import(p, context))
    parser.parse()
    return context


## Queues the scenery's event and waits for it to run - its delay, in simulated time; true when it ran
func _run_event(event_name:StringName) -> bool:
    var event:RID = ScenarioEventServer.event_get_rid_by_name(event_name)
    ScenarioEventServer.event_queue(event)
    return await wait_simulated_until(func() -> bool: return not ScenarioEventServer.event_is_queued(event),
            ScenarioEventServer.event_get_delay(event) + TICK, "%s run" % event_name)
