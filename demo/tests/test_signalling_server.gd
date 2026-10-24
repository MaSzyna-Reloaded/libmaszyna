extends MaszynaGutTest

const EventImporter = preload("res://addons/libmaszyna/scenery/maszyna_event_importer.gd")
## Where a listed signal head's model stands
const SIGNAL_HEAD_POSITION: Vector3 = Vector3(5.0, 0.0, 0.0)


class RecordingImplementation extends SignallingImplementation:
    var added: Array[RID] = []
    var events: Array[StringName] = []

    func _signal_head_added(_system: RID, signal_head: RID) -> void:
        added.append(signal_head)

    func _handle_event(system: RID, event: StringName, _arguments: Dictionary) -> void:
        events.append(event)
        for signal_head: RID in SignallingServer.system_get_signal_heads(system):
            SignallingServer.signal_head_light_enable(signal_head, 0)


## Records what the server hands an interlocking implementation, as [call, system, element, ...]
class InterlockingRecording extends SignallingImplementation:
    var calls: Array = []

    func _handle_source_event(system: RID, source: RID, event: StringName, arguments: Dictionary) -> void:
        calls.append([&"source_event", system, source, event, arguments])

    func _switch_added(system: RID, switch_name: StringName) -> void:
        calls.append([&"switch_added", system, switch_name])

    func _switch_removed(system: RID, switch_name: StringName) -> void:
        calls.append([&"switch_removed", system, switch_name])

    func _switch_created(system: RID, switch_name: StringName) -> void:
        calls.append([&"switch_created", system, switch_name])

    func _switch_freed(system: RID, switch_name: StringName) -> void:
        calls.append([&"switch_freed", system, switch_name])

    func _switch_movement_finished(system: RID, switch_name: StringName, active_track: int) -> void:
        calls.append([&"switch_movement_finished", system, switch_name, active_track])

    func _system_linked(system: RID, other: RID) -> void:
        calls.append([&"system_linked", system, other])

    func _system_unlinked(system: RID, other: RID) -> void:
        calls.append([&"system_unlinked", system, other])

    func _route_added(system: RID, route: RID) -> void:
        calls.append([&"route_added", system, route])

    func _route_set_requested(system: RID, route: RID) -> void:
        calls.append([&"route_set_requested", system, route])

    func _route_cancel_requested(system: RID, route: RID) -> void:
        calls.append([&"route_cancel_requested", system, route])

    func _route_release_requested(system: RID, route: RID) -> void:
        calls.append([&"route_release_requested", system, route])

    func _route_emergency_release_requested(system: RID, route: RID) -> void:
        calls.append([&"route_emergency_release_requested", system, route])


## How long a switch takes to move at most [s]
const SWITCH_MOVEMENT_TIMEOUT: float = 5.0
## The switch's track width [m]
const TRACK_WIDTH: float = 1.435


func test_a_sensor_impulse_reaches_its_system_with_the_polarity() -> void:
    var system: RID = SignallingServer.system_create()
    var recording: InterlockingRecording = InterlockingRecording.new()
    SignallingServer.system_attach_implementation(system, recording)
    var source: RID = SignallingServer.source_create()
    SignallingServer.system_add_source(system, source)

    SignallingServer.source_send_event(source, &"impulse", {&"polarity": SignallingServer.SOURCE_POLARITY_NEGATIVE})

    assert_eq(recording.calls, [[&"source_event", system, source, &"impulse",
            {&"polarity": SignallingServer.SOURCE_POLARITY_NEGATIVE}]])
    SignallingServer.source_free(source)
    SignallingServer.system_free(system)


func test_an_isolated_section_reports_through_its_source() -> void:
    var system: RID = SignallingServer.system_create()
    var recording: InterlockingRecording = InterlockingRecording.new()
    SignallingServer.system_attach_implementation(system, recording)
    var track: RID = TrackServer.track_create()
    var section: RID = TrackServer.isolated_create()
    TrackServer.isolated_set_name(section, &"it1")
    TrackServer.isolated_add_track(section, track)
    var source: RID = SignallingServer.source_create()
    SignallingServer.source_set_isolated(source, &"it1")
    var vehicle: RID = SignallingServer.source_create() # stands in for a vehicle

    # no system yet: nothing to report to
    TrackServer.track_axle_entered(track, vehicle)
    TrackServer.track_axle_left(track, vehicle)
    SignallingServer.system_add_source(system, source)
    TrackServer.track_axle_entered(track, vehicle)
    TrackServer.track_axle_entered(track, vehicle)
    TrackServer.track_axle_left(track, vehicle)
    TrackServer.track_axle_left(track, vehicle)

    assert_eq(SignallingServer.source_get_isolated(source), &"it1")
    assert_eq(recording.calls, [
        [&"source_event", system, source, &"occupied", {}],
        [&"source_event", system, source, &"freed", {}],
    ], "the first axle on, the last one off")
    TrackServer.isolated_free(section)
    TrackServer.track_free(track)
    SignallingServer.source_free(source)
    SignallingServer.source_free(vehicle)
    SignallingServer.system_free(system)


func test_a_source_joined_to_an_occupied_section_hears_it_at_once() -> void:
    var system: RID = SignallingServer.system_create()
    var recording: InterlockingRecording = InterlockingRecording.new()
    SignallingServer.system_attach_implementation(system, recording)
    var track: RID = TrackServer.track_create()
    var section: RID = TrackServer.isolated_create()
    TrackServer.isolated_set_name(section, &"it2")
    TrackServer.isolated_add_track(section, track)
    var vehicle: RID = SignallingServer.source_create() # stands in for a vehicle
    TrackServer.track_axle_entered(track, vehicle)
    var source: RID = SignallingServer.source_create()
    SignallingServer.system_add_source(system, source)

    SignallingServer.source_set_isolated(source, &"it2")

    assert_eq(recording.calls, [[&"source_event", system, source, &"occupied", {}]])
    TrackServer.track_axle_left(track, vehicle)
    TrackServer.isolated_free(section)
    TrackServer.track_free(track)
    SignallingServer.source_free(source)
    SignallingServer.source_free(vehicle)
    SignallingServer.system_free(system)


func test_a_switch_of_a_system_reports_where_it_came_to_lie() -> void:
    var switch_track: RID = _create_switch("test_switch_moved")
    var system: RID = SignallingServer.system_create()
    SignallingServer.system_add_switch(system, &"test_switch_moved")
    var recording: InterlockingRecording = InterlockingRecording.new()

    SignallingServer.system_attach_implementation(system, recording)
    TrackServer.switch_set_active_track(
            TrackServer.switch_get_rid_by_name("test_switch_moved"), TrackServer.TRACK_DIVERGING)
    await wait_until(func() -> bool: return recording.calls.size() == 2, SWITCH_MOVEMENT_TIMEOUT)

    assert_eq(SignallingServer.switch_get_system(&"test_switch_moved"), system)
    assert_eq(SignallingServer.system_get_switches(system), PackedStringArray(["test_switch_moved"]))
    assert_eq(recording.calls, [
        [&"switch_added", system, &"test_switch_moved"],
        [&"switch_movement_finished", system, &"test_switch_moved", TrackServer.TRACK_DIVERGING],
    ], "a late implementation learns the switches held, then their movements")
    SignallingServer.system_free(system)
    assert_false(SignallingServer.switch_get_system(&"test_switch_moved").is_valid(),
            "a freed system lets its switches go")
    TrackServer.track_free(switch_track)


func test_a_switch_named_before_its_track_follows_the_track_coming_and_going() -> void:
    var system: RID = SignallingServer.system_create()
    var recording: InterlockingRecording = InterlockingRecording.new()
    SignallingServer.system_attach_implementation(system, recording)
    SignallingServer.system_add_switch(system, &"test_switch_later")

    var switch_track: RID = _create_switch("test_switch_later")
    TrackServer.track_update(switch_track, TrackServer.TRACK_NORMAL, "test_switch_later", TRACK_WIDTH)
    TrackServer.track_update(switch_track, TrackServer.TRACK_SWITCH, "test_switch_later", TRACK_WIDTH)
    TrackServer.track_free(switch_track)

    assert_eq(recording.calls, [
        [&"switch_added", system, &"test_switch_later"],
        [&"switch_created", system, &"test_switch_later"],
        [&"switch_freed", system, &"test_switch_later"],
        [&"switch_created", system, &"test_switch_later"],
        [&"switch_freed", system, &"test_switch_later"],
    ], "the system follows its switch by name through the track's types and its end")
    assert_eq(SignallingServer.system_get_switches(system), PackedStringArray(["test_switch_later"]),
            "the name stays; there is no switch to hold")
    SignallingServer.system_free(system)


func test_a_link_between_systems_goes_both_ways_and_goes_with_a_system() -> void:
    var system: RID = SignallingServer.system_create()
    var other: RID = SignallingServer.system_create()
    var recording: InterlockingRecording = InterlockingRecording.new()
    var other_recording: InterlockingRecording = InterlockingRecording.new()
    SignallingServer.system_attach_implementation(system, recording)
    SignallingServer.system_attach_implementation(other, other_recording)

    SignallingServer.system_link(system, other)

    assert_eq(SignallingServer.system_get_linked_systems(system), [other] as Array[RID])
    assert_eq(SignallingServer.system_get_linked_systems(other), [system] as Array[RID])
    assert_eq(recording.calls, [[&"system_linked", system, other]])
    assert_eq(other_recording.calls, [[&"system_linked", other, system]])
    SignallingServer.system_free(system)
    assert_eq(SignallingServer.system_get_linked_systems(other), [] as Array[RID])
    assert_eq(other_recording.calls.back(), [&"system_unlinked", other, system])
    SignallingServer.system_free(other)


func test_a_route_keeps_its_definition() -> void:
    var route: RID = SignallingServer.route_create()
    var other: RID = SignallingServer.route_create()
    var section: RID = SignallingServer.source_create()
    var overlap: RID = SignallingServer.source_create()
    var approach: RID = SignallingServer.source_create()
    var sections: Array[RID] = [section]
    var overlap_sections: Array[RID] = [overlap]
    var approach_sections: Array[RID] = [approach]

    SignallingServer.route_set_name(route, &"test_route_a1")
    SignallingServer.route_set_kind(route, SignallingServer.ROUTE_KIND_SHUNTING)
    SignallingServer.route_set_sections(route, sections)
    SignallingServer.route_set_overlap_sections(route, overlap_sections)
    SignallingServer.route_set_approach_sections(route, approach_sections)
    SignallingServer.route_set_switch_position(route, &"test_route_switch", TrackServer.TRACK_DIVERGING)
    SignallingServer.route_add_conflict(route, other)

    assert_eq(SignallingServer.route_get_rid_by_name(&"test_route_a1"), route)
    assert_eq(SignallingServer.route_get_kind(route), SignallingServer.ROUTE_KIND_SHUNTING)
    assert_eq(SignallingServer.route_get_sections(route), sections)
    assert_eq(SignallingServer.route_get_overlap_sections(route), overlap_sections)
    assert_eq(SignallingServer.route_get_approach_sections(route), approach_sections)
    assert_eq(SignallingServer.route_get_switch_positions(route), {&"test_route_switch": TrackServer.TRACK_DIVERGING})
    assert_eq(SignallingServer.route_get_conflicts(other), [route] as Array[RID], "a conflict goes both ways")
    SignallingServer.source_free(section)
    assert_eq(SignallingServer.route_get_sections(route), [] as Array[RID], "a freed source leaves the route")
    SignallingServer.route_free(other)
    assert_eq(SignallingServer.route_get_conflicts(route), [] as Array[RID], "a freed route leaves the conflicts")
    SignallingServer.route_free(route)
    SignallingServer.source_free(overlap)
    SignallingServer.source_free(approach)


func test_route_requests_go_to_the_implementation_of_its_system() -> void:
    var system: RID = SignallingServer.system_create()
    var route: RID = SignallingServer.route_create()
    SignallingServer.system_add_route(system, route)
    var recording: InterlockingRecording = InterlockingRecording.new()

    SignallingServer.system_attach_implementation(system, recording)
    SignallingServer.route_request_set(route)
    SignallingServer.route_request_cancel(route)
    SignallingServer.route_request_release(route)
    SignallingServer.route_request_emergency_release(route)

    assert_eq(SignallingServer.route_get_system(route), system)
    assert_eq(recording.calls, [
        [&"route_added", system, route],
        [&"route_set_requested", system, route],
        [&"route_cancel_requested", system, route],
        [&"route_release_requested", system, route],
        [&"route_emergency_release_requested", system, route],
    ])
    SignallingServer.route_free(route)
    SignallingServer.system_free(system)


func test_a_locked_route_holds_its_sections_until_released_one_by_one() -> void:
    var route: RID = SignallingServer.route_create()
    var first: RID = SignallingServer.source_create()
    var second: RID = SignallingServer.source_create()
    var overlap: RID = SignallingServer.source_create()
    var sections: Array[RID] = [first, second]
    var overlap_sections: Array[RID] = [overlap]
    SignallingServer.route_set_sections(route, sections)
    SignallingServer.route_set_overlap_sections(route, overlap_sections)
    watch_signals(SignallingServer)

    SignallingServer.route_set_state(route, SignallingServer.ROUTE_STATE_LOCKED)
    SignallingServer.route_release_section(route, first)

    assert_eq(SignallingServer.route_get_state(route), SignallingServer.ROUTE_STATE_LOCKED)
    assert_signal_emitted_with_parameters(SignallingServer, "route_state_changed",
            [route, SignallingServer.ROUTE_STATE_LOCKED])
    assert_signal_emitted_with_parameters(SignallingServer, "route_section_released", [route, first])
    assert_eq(SignallingServer.route_get_locked_sections(route), [second, overlap] as Array[RID])
    SignallingServer.route_set_state(route, SignallingServer.ROUTE_STATE_IDLE)
    assert_eq(SignallingServer.route_get_locked_sections(route), [] as Array[RID], "an idle route holds nothing")
    SignallingServer.route_free(route)
    for source: RID in [first, second, overlap]:
        SignallingServer.source_free(source)


func test_signal_head_node_registers_the_model_under_its_name() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    var signal_head_node: SignalHeadNode = _create_signal_head_node(&"test_registered", model)

    var signal_head: RID = SignallingServer.signal_head_get_rid_by_name(&"test_registered")
    assert_true(signal_head.is_valid(), "the model's signal head should be registered under the node's name")
    assert_eq(signal_head_node.get_signal_head(), signal_head)


func test_enabling_a_light_shows_its_submodel_and_reports_the_change() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    var signal_head_node: SignalHeadNode = _create_signal_head_node(&"test_enable", model)
    watch_signals(signal_head_node)

    signal_head_node.enable_light(0)

    assert_true(model.get_node(NodePath("light_on00")).visible, "light_on00 should be shown")
    assert_false(model.get_node(NodePath("light_off00")).visible, "light_off00 should be hidden")
    assert_eq(signal_head_node.get_light_state(0), SignallingServer.LIGHT_STATE_ON)
    assert_signal_emitted_with_parameters(signal_head_node, "light_state_changed", [0, SignallingServer.LIGHT_STATE_ON])


func test_a_blinking_light_goes_on_and_off() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    var signal_head_node: SignalHeadNode = _create_signal_head_node(&"test_blink", model)
    var light_on: Node3D = model.get_node(NodePath("light_on00"))

    signal_head_node.blink_light(0, 0.05, 0.05, 0.0)

    var seen_on: bool = false
    var seen_off: bool = false
    for i: int in range(30):
        await wait_idle_frames(1)
        seen_on = seen_on or light_on.visible
        seen_off = seen_off or not light_on.visible
        if seen_on and seen_off:
            break
    assert_eq(signal_head_node.get_light_state(0), SignallingServer.LIGHT_STATE_BLINKING)
    assert_true(seen_on and seen_off, "a blinking light should be seen both on and off")


func test_a_system_implementation_receives_its_signal_heads_and_events() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    _create_signal_head_node(&"test_system", model)
    var signal_head: RID = SignallingServer.signal_head_get_rid_by_name(&"test_system")
    var implementation: RecordingImplementation = RecordingImplementation.new()
    var system: RID = SignallingServer.system_create()
    SignallingServer.system_add_signal_head(system, signal_head)

    SignallingServer.system_attach_implementation(system, implementation)
    SignallingServer.system_send_event(system, &"proceed", {})

    assert_eq(implementation.added, [signal_head] as Array[RID], "an attached implementation learns the signal heads held")
    assert_eq(implementation.events, [&"proceed"] as Array[StringName])
    assert_eq(SignallingServer.signal_head_get_light_state(signal_head, 0), SignallingServer.LIGHT_STATE_ON)
    SignallingServer.system_free(system)


func test_freeing_the_model_removes_its_signal_head_from_the_system() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    _create_signal_head_node(&"test_freed", model)
    var system: RID = SignallingServer.system_create()
    SignallingServer.system_add_signal_head(system, SignallingServer.signal_head_get_rid_by_name(&"test_freed"))

    remove_child(model)
    model.free()

    assert_eq(SignallingServer.system_get_signal_heads(system).size(), 0, "the signal head should leave its system")
    assert_false(SignallingServer.signal_head_get_rid_by_name(&"test_freed").is_valid())
    SignallingServer.system_free(system)


func test_a_reloaded_model_is_registered_again_and_picked_up_by_its_system_node() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    var signal_head_node: SignalHeadNode = _create_signal_head_node(&"test_reload", model)
    var system_node: SignallingSystemNode = SignallingSystemNode.new()
    system_node.signal_head_names = PackedStringArray(["test_reload"])
    add_child_autoqfree(system_node)
    var before: RID = signal_head_node.get_signal_head()

    model.reload()

    var after: RID = SignallingServer.signal_head_get_rid_by_name(&"test_reload")
    assert_true(after.is_valid(), "the reloaded model should be registered again")
    assert_ne(after, before, "the new instance should have a new signal head")
    assert_eq(signal_head_node.get_signal_head(), after)
    assert_eq(SignallingServer.signal_head_get_system(after), system_node.get_system())


func test_a_signal_head_node_attaches_to_a_signal_head_registered_later() -> void:
    var signal_head_node: SignalHeadNode = SignalHeadNode.new()
    signal_head_node.signal_head_name = &"test_late"
    add_child_autoqfree(signal_head_node)
    assert_false(signal_head_node.get_signal_head().is_valid())

    var model: MaszynaE3DModelInstance = _create_model_instance()
    var signal_head: RID = SignallingServer.signal_head_create()
    model.attach_signal_head(signal_head)
    SignallingServer.signal_head_set_name(signal_head, &"test_late")

    assert_eq(signal_head_node.get_signal_head(), signal_head)


func test_legacy_kind_turns_lights_events_into_aspects() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    _create_signal_head_node(&"test_legacy", model)
    var signal_head: RID = SignallingServer.signal_head_get_rid_by_name(&"test_legacy")
    var aspects: Dictionary = {
        &"sem_ligh1": PackedFloat32Array([1.35, 1.0]),
        &"sem_ligh2": PackedFloat32Array([-1.0, 0.0]),
    }
    SignallingServer.signal_head_set_kind(signal_head, MaszynaSignalHeadKindFactory.create_kind(aspects))
    var system: RID = SignallingServer.system_create()
    SignallingServer.system_attach_implementation(system, MaszynaSignallingImplementation.new())
    SignallingServer.system_add_signal_head(system, signal_head)

    SignallingServer.system_send_event(system, &"lights", {"signal_head": signal_head, "aspect": &"sem_ligh1"})
    assert_eq(SignallingServer.signal_head_get_light_state(signal_head, 0), SignallingServer.LIGHT_STATE_BLINKING)
    assert_eq(SignallingServer.signal_head_get_light_state(signal_head, 1), SignallingServer.LIGHT_STATE_ON)

    SignallingServer.system_send_event(system, &"lights", {"signal_head": signal_head, "aspect": &"sem_ligh2"})
    assert_eq(
        SignallingServer.signal_head_get_light_state(signal_head, 0),
        SignallingServer.LIGHT_STATE_BLINKING,
        "-1 should leave the light as it is"
    )
    assert_eq(SignallingServer.signal_head_get_light_state(signal_head, 1), SignallingServer.LIGHT_STATE_OFF)
    assert_eq(SignallingServer.signal_head_get_aspect(signal_head), &"sem_ligh2")
    SignallingServer.system_free(system)


func test_lights_events_of_an_include_give_its_copies_one_kind() -> void:
    var parser: MaszynaParser = MaszynaParser.new()
    parser.initialize((
        "event Sem_A_sem_ligh1 lights 0.0 sem_a 0 1 0 endevent "
        + "event sem_a_sem_ligh3o lights 0.0 sem_a|none 2 0 0 endevent "
        + "event sem_b_sem_ligh1 lights 0.0 sem_b 0 1 0 endevent "
        + "event sem_b_sem_ligh3o lights 0.0 sem_b 2 0 0 endevent "
        + "event sem_a_info multiple 0 none sem_a_sem_ligh1 endevent"
    ).to_utf8_buffer())
    var context: MaszynaImporterContext = MaszynaImporterContext.new()
    parser.register_handler("event", func(p: MaszynaParser) -> Array: return EventImporter.new().import(p, context))
    parser.parse()
    var models: Array[ModelData] = [
        _create_model_data("sem_a", PackedFloat32Array([0, 0, 1])),
        _create_model_data("sem_b", PackedFloat32Array()),
        _create_model_data("lamp", PackedFloat32Array([3])),
        _create_model_data("house", PackedFloat32Array()),
    ]

    MaszynaSceneryProvider.assign_signal_head_kinds(models, context.events)

    assert_eq(context.events.size(), 5, "every event should be kept, only lights events make aspects")
    var kind: SignalHeadKind = models[0].signal_head_kind
    assert_eq(kind.get_aspect_names(), PackedStringArray(["sem_ligh1", "sem_ligh3o"]))
    assert_eq(kind.get_aspect(&"sem_ligh3o").lights, PackedInt32Array([SignalAspect.LIGHT_BLINK, 0, 0]))
    assert_same(models[1].signal_head_kind, kind, "the copies of one include should share their kind")
    assert_eq(models[2].signal_head_kind, MaszynaSceneryProvider.GENERIC_SIGNAL_HEAD_KIND, "a lit model no event reaches")
    assert_null(models[3].signal_head_kind, "a model neither lit nor aimed at is no signal head")


func test_the_light_count_comes_from_the_model() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance()
    var signal_head_node: SignalHeadNode = _create_signal_head_node(&"test_light_count", model)

    assert_eq(SignallingServer.signal_head_get_light_count(signal_head_node.get_signal_head()), 2)
    assert_eq(signal_head_node.light_count, 2)


func test_an_aspect_of_the_kind_lights_its_lights() -> void:
    var kind: SignalHeadKind = SignalHeadKind.new()
    var aspects: Dictionary[StringName, SignalAspect] = {
        &"stop": _create_aspect(PackedInt32Array([SignalAspect.LIGHT_OFF, SignalAspect.LIGHT_ON])),
        &"proceed": _create_aspect(PackedInt32Array([SignalAspect.LIGHT_BLINK, SignalAspect.LIGHT_OFF])),
    }
    kind.aspects = aspects
    var signal_head_node: SignalHeadNode = SignalHeadNode.new()
    signal_head_node.signal_head_name = &"test_aspect"
    signal_head_node.kind = kind
    signal_head_node.set(&"aspect", &"stop")
    signal_head_node.model = _create_model_instance()
    add_child_autoqfree(signal_head_node)
    var signal_head: RID = signal_head_node.get_signal_head()
    assert_eq(SignallingServer.signal_head_get_aspects(signal_head), PackedStringArray(["stop", "proceed"]))
    assert_eq(SignallingServer.signal_head_get_aspect(signal_head), &"stop", "the scene's aspect should be shown")
    assert_eq(signal_head_node.get(&"light_1_state"), SignallingServer.LIGHT_STATE_ON)
    watch_signals(signal_head_node)

    SignallingServer.signal_head_set_aspect(signal_head, &"proceed")

    assert_eq(signal_head_node.get(&"aspect"), &"proceed", "the aspect property should read the server")
    assert_eq(signal_head_node.get_light_state(0), SignallingServer.LIGHT_STATE_BLINKING)
    assert_eq(signal_head_node.get_light_state(1), SignallingServer.LIGHT_STATE_OFF)
    assert_signal_emitted_with_parameters(signal_head_node, "aspect_changed", [&"proceed"])


func _create_aspect(lights: PackedInt32Array) -> SignalAspect:
    var aspect: SignalAspect = SignalAspect.new()
    aspect.lights = lights
    return aspect


func _create_model_data(model_name: String, lights: PackedFloat32Array) -> MaszynaModelData:
    var model_data: MaszynaModelData = MaszynaModelData.new()
    model_data.name = model_name
    model_data.lights = lights
    return model_data


func test_signal_heads_are_listed_with_the_instance_they_light() -> void:
    var model: MaszynaE3DModelInstance = _create_model_instance(SIGNAL_HEAD_POSITION)
    _create_signal_head_node(&"test_listed", model)

    var signal_head: RID = SignallingServer.signal_head_get_rid_by_name(&"test_listed")
    assert_has(SignallingServer.signal_head_get_rids(), signal_head)
    var instance: RID = MaszynaE3DRenderingServer.signal_head_get_instance(signal_head)
    assert_true(instance.is_valid(), "the signal head should give the instance it was made of")
    assert_eq(MaszynaE3DRenderingServer.instance_get_transform(instance).origin, SIGNAL_HEAD_POSITION)


## The model stands at position from the start - a NODES instance does not follow a node moved later
func _create_model_instance(position: Vector3 = Vector3.ZERO) -> MaszynaE3DModelInstance:
    var instance: MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    instance.position = position
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var submodels: Array[MaszynaE3DSubModel] = []
    for light_name: String in ["00", "01"]:
        var light_on: MaszynaE3DSubModel = _create_transform_submodel("light_on" + light_name, false)
        var light_off: MaszynaE3DSubModel = _create_transform_submodel("light_off" + light_name, true)
        var definition: MaszynaE3DModelLightDefinition = MaszynaE3DModelLightDefinition.new()
        definition.on_submodel_path = NodePath(light_on.resource_name)
        definition.off_submodel_path = NodePath(light_off.resource_name)
        model.register_light(light_name, definition)
        submodels.append(light_on)
        submodels.append(light_off)
    model.submodels = submodels
    instance.model = model
    add_child_autoqfree(instance)
    return instance


func _create_signal_head_node(signal_head_name: StringName, model: MaszynaE3DModelInstance) -> SignalHeadNode:
    var signal_head_node: SignalHeadNode = SignalHeadNode.new()
    signal_head_node.signal_head_name = signal_head_name
    signal_head_node.model = model
    add_child_autoqfree(signal_head_node)
    return signal_head_node


func _create_transform_submodel(submodel_name: String, visible: bool) -> MaszynaE3DSubModel:
    var submodel: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    submodel.resource_name = submodel_name
    submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    submodel.visible = visible
    return submodel


func _create_switch(switch_name: String) -> RID:
    var straight: TrackCurve = TrackCurve.new()
    straight.p2 = Vector3(10.0, 0.0, 0.0)
    var diverging: TrackCurve = TrackCurve.new()
    diverging.p2 = Vector3(10.0, 0.0, 10.0)
    var switch_track: RID = TrackServer.track_create()
    TrackServer.track_update_curves(switch_track, straight, diverging)
    TrackServer.track_update(switch_track, TrackServer.TRACK_SWITCH, switch_name, TRACK_WIDTH)
    return switch_track
