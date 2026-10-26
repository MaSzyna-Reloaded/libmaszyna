@tool
extends RefCounted
class_name MaszynaEventFactory

## Builds ScenarioEventServer's events, memories and launchers out of a scenery's data - the
## original's `firstinit` (simulationstateserializer.cpp:360-379). The original's encoding (the `*`
## of a field, `else`, a negative delay, HHMM) is read here once; the server gets delays, actions
## and conditions. Names are matched in lower case, as the original reads them (its parser lowers
## every token it is not told to keep, parser.h:71).
##
## Built: `updatevalues`, `addvalues`, `copyvalues`, `multiple`, `lights`, `switch`, `trackvel`,
## `voltage`, `animation` (rotate, translate), `sound`, `putvalues` and `getvalues` (the commands a
## vehicle acts on); conditions `memcompare`, `memcompareex`,
## `probability`, `trackoccupied`, `trackfree`; a track's `event0/1/2`, `eventall0/1/2` and the
## events named `<track>:<slot>` (Track.cpp:970-983); the isolated sections with their own memory and
## `<section>:busy/:free/:inc/:dec` events. The other types get an event without an action (TODO.md).
## A name defined more than once runs its definitions as the scenery's `joinduplicatedevents`
## says (MaszynaEventData.joined; see where the names defined more than once are set up).

## "Leave this field as it is" (Event.cpp:491-503)
const FIELD_KEPT:String = "*"
## Splits a `multiple` event's list (Event.cpp:1266-1278)
const ELSE:String = "else"
## An event with this in its name is queued when the scenery starts (Event.cpp:2341-2345)
const ONSTART:String = "onstart"
## `copyvalues` copies all three fields unless given a mask (Event.cpp:898)
const ALL_FIELDS:int = MaszynaScenarioEventServer.MEMORY_FIELD_TEXT | MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1 | MaszynaScenarioEventServer.MEMORY_FIELD_VALUE2
## `<text> <value1> <value2>`
const MEMCOMPARE_FIELDS:int = 3
## The commands a driver reads on the tracks ahead rather than being sent them: a `putvalues`
## with one of these (or a stop point) and a `getvalues` whose memory holds one of the others at
## the start are passive - never run, only read (Event.cpp:577-597, 715-754). `CabSignal` stays active
## unlike the original: a vehicle the player drives has no driver to read the magnet (TODO.md)
const PASSIVE_PUT_COMMANDS:Array[String] = [
    "SetVelocity", "RoadVelocity", "SectionVelocity", "ShuntVelocity", "OutsideStation",
]
const PASSENGER_STOP_POINT:String = "PassengerStopPoint:"
## What follows it in a stop's name only makes the name unique
const STOP_POINT_UNIQUE:String = "#"
const PASSIVE_GET_COMMANDS:Array[String] = ["SetVelocity", "ShuntVelocity", "SetProximityVelocity"]
## A duplicate of a name like these is dropped whatever joinduplicatedevents says - the original's
## "temporary" exceptions for W4 line info, horn warnings and SHP magnets
## (event_manager::insert(), Event.cpp:2304-2321)
const DROPPED_DUPLICATE_PREFIX:String = "#"
const DROPPED_DUPLICATE_SUFFIXES:Array[String] = ["lineinfo:", "_warning", "_shp"]
## A launcher's HHMM (EvLaunch.cpp:139-140)
const HHMM_HOUR:int = 100
## A launcher firing the first time it is in range (EvLaunch.cpp:182-186) - not ported (TODO.md)
const FIRE_ONCE_IN_RANGE:float = -10000.0
## A track's event slots by the key the `.scn` gives them (Track.cpp:815-858, 952-954)
const TRACK_EVENTS:Dictionary[String, MaszynaScenarioEventServer.TrackEvent] = {
    "event0": MaszynaScenarioEventServer.TRACK_EVENT0,
    "event1": MaszynaScenarioEventServer.TRACK_EVENT1,
    "event2": MaszynaScenarioEventServer.TRACK_EVENT2,
    "eventall0": MaszynaScenarioEventServer.TRACK_EVENTALL0,
    "eventall1": MaszynaScenarioEventServer.TRACK_EVENTALL1,
    "eventall2": MaszynaScenarioEventServer.TRACK_EVENTALL2,
}
## An isolated section's events by the suffix of their name (Track.cpp:110-116)
const ISOLATED_EVENTS:Dictionary[String, MaszynaScenarioEventServer.IsolatedEvent] = {
    ":busy": MaszynaScenarioEventServer.ISOLATED_BUSY,
    ":free": MaszynaScenarioEventServer.ISOLATED_FREE,
    ":inc": MaszynaScenarioEventServer.ISOLATED_INC,
    ":dec": MaszynaScenarioEventServer.ISOLATED_DEC,
}
## What a section's own memory is marked with, and when
const ISOLATED_MARKERS:Dictionary[MaszynaMemoryAction.Mode, MaszynaScenarioEventServer.IsolatedEvent] = {
    MaszynaMemoryAction.MODE_ISOLATED_BUSY: MaszynaScenarioEventServer.ISOLATED_BUSY,
    MaszynaMemoryAction.MODE_ISOLATED_FREE: MaszynaScenarioEventServer.ISOLATED_FREE,
}
## `memcompareex` (comparison.h:56-75); an unknown one is the original's legacy default
const COMPARISON_PASSES:Dictionary[String, MaszynaEventCondition.Pass] = {
    "all": MaszynaEventCondition.PASS_ALL,
    "any": MaszynaEventCondition.PASS_ANY,
    "none": MaszynaEventCondition.PASS_NONE,
}
const COMPARISON_OPERATORS:Dictionary[String, MaszynaEventCondition.Operator] = {
    "==": MaszynaEventCondition.OPERATOR_EQUAL,
    "!=": MaszynaEventCondition.OPERATOR_NOT_EQUAL,
    "<": MaszynaEventCondition.OPERATOR_LESS,
    ">": MaszynaEventCondition.OPERATOR_GREATER,
    "<=": MaszynaEventCondition.OPERATOR_LESS_EQUAL,
    ">=": MaszynaEventCondition.OPERATOR_GREATER_EQUAL,
}
## The fields of `<text> <value1> <value2>`, in their order
const MEMORY_FIELDS:Array[int] = [
    MaszynaScenarioEventServer.MEMORY_FIELD_TEXT, MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1, MaszynaScenarioEventServer.MEMORY_FIELD_VALUE2
]
## A launcher's key that is a radio call (EvLaunch.cpp:67-79)
const RADIO_CALLS:Dictionary[String, RailVehicleRadio.RadioCall] = {
    "radio_call1": RailVehicleRadio.RADIO_CALL1,
    "radio_call3": RailVehicleRadio.RADIO_CALL3,
}
## `sound <mode>`: 1 plays, -1 loops, 0 stops (Event.cpp:1379-1390)
const SOUND_MODES:Dictionary[int, MaszynaSoundAction.Mode] = {
    0: MaszynaSoundAction.Mode.STOP,
    1: MaszynaSoundAction.Mode.PLAY,
    -1: MaszynaSoundAction.Mode.LOOP,
}
## `animation <rotate|translate> <submodel> <x> <y> <z> <speed>` (Event.cpp:1606-1682)
const ANIMATION_MODES:Dictionary[String, MaszynaAnimationAction.Mode] = {
    "rotate": MaszynaAnimationAction.MODE_ROTATE,
    "translate": MaszynaAnimationAction.MODE_TRANSLATE,
}


static func build(
    root:SceneryContentNode,
    events:Array[MaszynaEventData],
    memcells:Array[MaszynaMemcellData],
    launchers:Array[MaszynaEventLauncherData],
    sounds:Array[SoundData],
    isolated_sections:Array[MaszynaIsolatedData],
    tracks:Array[TrackData],
    track_rids:Array[RID],
    models:Array[ModelData],
    model_rids:Array[RID],
    power_sources:Array[PowerSourceData],
) -> void:
    # the tracks and models come with the RIDs they were built as, in their order; a track is found
    # by its name in TrackServer
    var model_names:Dictionary[String, String] = {}
    var instances_by_name:Dictionary[String, RID] = {}
    for index:int in models.size():
        model_names[models[index].name.to_lower()] = models[index].name
        instances_by_name[models[index].name.to_lower()] = model_rids[index]
    var power_source_names:Dictionary[String, String] = {}
    for power_source_data:PowerSourceData in power_sources:
        power_source_names[power_source_data.name.to_lower()] = power_source_data.name

    # the scenery's events are of one event system, the original's (MaszynaScenarioEventServer): its
    # memories and launchers are made in it and go with it
    var system:RID = ScenarioEventServer.system_create()
    root._event_system_rids.append(system)
    ScenarioEventServer.system_attach_implementation(system, MaszynaScenarioEventServer.implementation_get())

    var memories:Dictionary[String, RID] = {}
    var memory_positions:Dictionary[String, Vector3] = {}
    for memcell:MaszynaMemcellData in memcells:
        var memory:RID = MaszynaScenarioEventServer.memory_create(system)
        MaszynaScenarioEventServer.memory_set_name(memory, memcell.name)
        MaszynaScenarioEventServer.memory_set_values(memory, memcell.text, memcell.value1, memcell.value2)
        MaszynaScenarioEventServer.memory_set_position(memory, memcell.position)
        if memcell.track:
            MaszynaScenarioEventServer.memory_set_track(memory, TrackServer.track_get_rid_by_name(memcell.track))
        memories[memcell.name.to_lower()] = memory
        memory_positions[memcell.name.to_lower()] = memcell.position

    # a track's `friction <memcell>`: the cell modifies its friction (Track.cpp:924, 3526-3529)
    for index:int in tracks.size():
        var scn_track:MaszynaTrackData = tracks[index] as MaszynaTrackData
        if not scn_track:
            continue
        var friction_memory:String = str(scn_track.parameters.get("friction", "")).to_lower()
        if friction_memory and memories.has(friction_memory):
            MaszynaScenarioEventServer.memory_add_friction_track(memories[friction_memory], track_rids[index])

    # the scenery's sounds, made audible only while the scenario runs (MaszynaScenario)
    var scenery_sounds:MaszynaScenerySounds = MaszynaScenerySounds.new(sounds)
    MaszynaSceneryProvider.scenery_set_sounds(root, scenery_sounds)
    root._scenery_radio = SceneryVirtualRadio.new()

    # the isolated sections, named by a track's `isolated`, an `isolated` block or an `area`
    var sections:Dictionary[String, RID] = {}
    var section_names:PackedStringArray = []
    for track_data:TrackData in tracks:
        var scn_track:MaszynaTrackData = track_data as MaszynaTrackData
        if scn_track and scn_track.parameters.has("isolated"):
            section_names.append(str(scn_track.parameters["isolated"]).to_lower())
    for block:MaszynaIsolatedData in isolated_sections:
        section_names.append(block.name)
        section_names.append_array(block.children)
    for section_name:String in section_names:
        if sections.has(section_name):
            continue
        var section:RID = TrackServer.isolated_create()
        root._isolated_rids.append(section)
        TrackServer.isolated_set_name(section, section_name)
        sections[section_name] = section
        # every section has a memory of its name, made if the scenery has none (Track.cpp:3556-3575)
        if not memories.has(section_name):
            var memory:RID = MaszynaScenarioEventServer.memory_create(system)
            MaszynaScenarioEventServer.memory_set_name(memory, section_name)
            memories[section_name] = memory
    for index:int in tracks.size():
        var scn_track:MaszynaTrackData = tracks[index] as MaszynaTrackData
        if scn_track and scn_track.parameters.has("isolated"):
            TrackServer.isolated_add_track(
                sections[str(scn_track.parameters["isolated"]).to_lower()], track_rids[index]
            )
    for block:MaszynaIsolatedData in isolated_sections:
        for track_name:String in block.tracks:
            var track:RID = TrackServer.track_get_rid_by_name(track_name)
            if track.is_valid():
                TrackServer.isolated_add_track(sections[block.name], track)
        for child:String in block.children:
            TrackServer.isolated_set_parent(sections[child], sections[block.name])

    # the definitions of each name, in their order; a duplicate of some names is dropped
    # (event_manager::insert(), Event.cpp:2304-2321)
    var kept_events:Array[MaszynaEventData] = []
    var definitions:Dictionary[String, Array] = {}
    for event:MaszynaEventData in events:
        var dropped:bool = event.name.begins_with(DROPPED_DUPLICATE_PREFIX)
        for suffix:String in DROPPED_DUPLICATE_SUFFIXES:
            dropped = dropped or event.name.ends_with(suffix)
        if dropped and definitions.has(event.name):
            continue
        if not definitions.has(event.name):
            definitions[event.name] = []
        definitions[event.name].append(kept_events.size())
        kept_events.append(event)

    # every event exists before any refers to another. A name defined more than once is an event
    # of its own, set up below once its definitions are built; they stay unnamed
    var event_rids:Array[RID] = []
    var events_by_name:Dictionary[String, RID] = {}
    for event:MaszynaEventData in kept_events:
        var rid:RID = _create_event(root, system)
        event_rids.append(rid)
        if definitions[event.name].size() == 1:
            ScenarioEventServer.event_set_name(rid, event.name)
            _set_delays(rid, event)
            events_by_name[event.name] = rid
    for event_name:String in definitions:
        if definitions[event_name].size() > 1:
            var named:RID = _create_event(root, system)
            ScenarioEventServer.event_set_name(named, event_name)
            events_by_name[event_name] = named

    for index:int in kept_events.size():
        await SceneryServer.frame_budget_wait()
        var event:MaszynaEventData = kept_events[index]
        var rid:RID = event_rids[index]
        var event_memories:Array[RID] = _get_memories(event.targets, memories)
        var targets:Array[RID] = []
        match event.type:
            "updatevalues", "addvalues":
                var action:MaszynaMemoryAction = MaszynaMemoryAction.new()
                action.memories = event_memories
                action.mask = _fields_mask(event.parameters)
                action.text = event.parameters[0]
                action.value1 = float(event.parameters[1])
                action.value2 = float(event.parameters[2])
                action.mode = MaszynaMemoryAction.MODE_ADD if event.type == "addvalues" else MaszynaMemoryAction.MODE_SET
                ScenarioEventServer.event_attach_action(rid, action)
            "copyvalues":
                var action:MaszynaMemoryAction = MaszynaMemoryAction.new()
                action.memories = event_memories
                action.source = memories.get(event.parameters[0].to_lower(), RID())
                action.mask = int(event.parameters[1]) if event.parameters.size() > 1 else ALL_FIELDS
                ScenarioEventServer.event_attach_action(rid, action)
            "multiple":
                var children:Array[RID] = []
                var else_children:Array[RID] = []
                var after_else:bool = false
                for child:String in event.parameters:
                    var child_name:String = child.to_lower()
                    if child_name == ELSE:
                        after_else = not after_else
                        continue
                    if not events_by_name.has(child_name):
                        continue
                    if after_else:
                        else_children.append(events_by_name[child_name])
                    else:
                        children.append(events_by_name[child_name])
                var action:MaszynaMultipleAction = MaszynaMultipleAction.new()
                action.events = children
                action.else_events = else_children
                ScenarioEventServer.event_attach_action(rid, action)
            "lights":
                var aspects:Array[StringName] = []
                for target:String in event.targets:
                    var signal_head:RID = SignallingServer.signal_head_get_rid_by_name(model_names.get(target, ""))
                    if not signal_head.is_valid():
                        continue
                    targets.append(signal_head)
                    aspects.append(MaszynaSignalHeadKindFactory.get_aspect_name(event.name, target))
                var action:MaszynaLightsAction = MaszynaLightsAction.new()
                action.signal_heads = targets
                action.aspects = aspects
                ScenarioEventServer.event_attach_action(rid, action)
            "switch":
                for target:String in event.targets:
                    var track:RID = TrackServer.track_get_rid_by_name(target)
                    if track.is_valid():
                        targets.append(track)
                var action:MaszynaSwitchAction = MaszynaSwitchAction.new()
                action.tracks = targets
                # the state is 0 or 1 (TTrack::Switch(), Track.cpp:1752)
                action.active_track = TrackServer.TRACK_DIVERGING if int(event.parameters[0]) == TrackServer.TRACK_DIVERGING else TrackServer.TRACK_COMMON
                ScenarioEventServer.event_attach_action(rid, action)
            "trackvel":
                for target:String in event.targets:
                    var track:RID = TrackServer.track_get_rid_by_name(target)
                    if track.is_valid():
                        targets.append(track)
                var action:MaszynaTrackVelocityAction = MaszynaTrackVelocityAction.new()
                action.tracks = targets
                action.velocity = float(event.parameters[0])
                ScenarioEventServer.event_attach_action(rid, action)
            "friction":
                # the scenery's rail friction multiplier (friction_event, Event.cpp:2090-2104)
                var action:MaszynaFrictionAction = MaszynaFrictionAction.new()
                action.friction = float(event.parameters[0])
                ScenarioEventServer.event_attach_action(rid, action)
            "voltage":
                for target:String in event.targets:
                    var power_source:RID = TractionServer.power_source_get_rid_by_name(power_source_names.get(target, ""))
                    if power_source.is_valid():
                        targets.append(power_source)
                var action:MaszynaVoltageAction = MaszynaVoltageAction.new()
                action.power_sources = targets
                action.voltage = float(event.parameters[0])
                ScenarioEventServer.event_attach_action(rid, action)
            "putvalues":
                # <x> <y> <z> <command> <value1> <value2> (Event.cpp:700-767)
                var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
                action.command = event.parameters[3]
                # a stop's name is unique past its `#`, the timetable knows it without, and in plain
                # ASCII as the timetable's own (Event.cpp:715-719)
                if action.command.begins_with(PASSENGER_STOP_POINT):
                    if action.command.contains(STOP_POINT_UNIQUE):
                        action.command = action.command.left(action.command.find(STOP_POINT_UNIQUE))
                    action.command = Windows1250.to_ascii(action.command)
                action.value1 = float(event.parameters[4])
                action.value2 = float(event.parameters[5])
                # the origin moves it, the rotation does not (Event.cpp:709-712)
                action.position = event.origin + Vector3(
                    float(event.parameters[0]), float(event.parameters[1]), float(event.parameters[2])
                )
                action.radio = root._scenery_radio
                ScenarioEventServer.event_attach_action(rid, action)
                MaszynaScenarioEventServer.event_set_passive(
                        rid, action.command in PASSIVE_PUT_COMMANDS or action.command.begins_with(PASSENGER_STOP_POINT))
            "getvalues":
                # the command is the first target memory's, read when the event runs (Event.cpp:577-597)
                if event_memories.is_empty():
                    continue
                var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
                action.source = event_memories[0]
                for target:String in event.targets:
                    if memory_positions.has(target):
                        action.position = memory_positions[target]
                        break
                action.radio = root._scenery_radio
                ScenarioEventServer.event_attach_action(rid, action)
                MaszynaScenarioEventServer.event_set_passive(
                        rid, MaszynaScenarioEventServer.memory_get_text(action.source) in PASSIVE_GET_COMMANDS)
            "sound":
                # any other mode does nothing (Event.cpp:1434)
                var sound_mode:int = int(event.parameters[0])
                if not SOUND_MODES.has(sound_mode):
                    push_warning("Sound event %s: mode %d does nothing" % [event.name, sound_mode])
                    continue
                var sound_names:PackedStringArray = []
                for target:String in event.targets:
                    if scenery_sounds.has_sound(target):
                        sound_names.append(target)
                var action:MaszynaSoundAction = MaszynaSoundAction.new()
                action.scenery_sounds = scenery_sounds
                action.radio = root._scenery_radio
                action.targets = sound_names
                action.mode = SOUND_MODES[sound_mode]
                # the optional radio channel it is a message on (Event.cpp:1382-1386)
                if event.parameters.size() > 1 and event.parameters[1].is_valid_int():
                    action.radio_channel = int(event.parameters[1])
                ScenarioEventServer.event_attach_action(rid, action)
            "animation":
                var mode:String = event.parameters[0].to_lower()
                if not ANIMATION_MODES.has(mode):
                    continue
                for target:String in event.targets:
                    if instances_by_name.has(target) and instances_by_name[target].is_valid():
                        targets.append(instances_by_name[target])
                var action:MaszynaAnimationAction = MaszynaAnimationAction.new()
                action.instances = targets
                action.mode = ANIMATION_MODES[mode]
                action.submodel = event.parameters[1]
                action.target = Vector3(float(event.parameters[2]), float(event.parameters[3]), float(event.parameters[4]))
                action.speed = float(event.parameters[5])
                ScenarioEventServer.event_attach_action(rid, action)
                # the event an animation of the submodel runs when it is done (Event.cpp:1588-1592)
                for target:String in event.targets:
                    var done:String = target + "." + action.submodel.to_lower() + ":done"
                    if instances_by_name.has(target) and events_by_name.has(done):
                        MaszynaE3DRenderingServer.instance_set_submodel_done_event(instances_by_name[target], action.submodel, events_by_name[done])

        # the targets of an event are the memories its `memcompare` reads (Event.cpp:1213-1233)
        var condition:MaszynaEventCondition = MaszynaEventCondition.new()
        var conditioned:bool = false
        var position:int = 0
        while position < event.condition.size():
            var keyword:String = event.condition[position].to_lower()
            position += 1
            if keyword == "memcompare":
                _set_memcompare(condition, event_memories, event.condition.slice(position, position + MEMCOMPARE_FIELDS))
                position += MEMCOMPARE_FIELDS
                conditioned = true
            elif keyword == "memcompareex":
                # <all|any|none>, then per field `*` or `<operator> <value>` (Event.cpp:222-252)
                condition.memories = event_memories
                condition.pass = COMPARISON_PASSES.get(event.condition[position].to_lower(), MaszynaEventCondition.PASS_ALL)
                position += 1
                var mask:int = 0
                for field:int in MEMCOMPARE_FIELDS:
                    if event.condition[position] == FIELD_KEPT:
                        position += 1
                        continue
                    var operator:MaszynaEventCondition.Operator = COMPARISON_OPERATORS.get(
                        event.condition[position], MaszynaEventCondition.OPERATOR_EQUAL
                    )
                    var value:String = event.condition[position + 1]
                    position += 2
                    mask |= MEMORY_FIELDS[field]
                    match MEMORY_FIELDS[field]:
                        MaszynaScenarioEventServer.MEMORY_FIELD_TEXT:
                            condition.text_operator = operator
                            condition.text = value
                        MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1:
                            condition.value1_operator = operator
                            condition.value1 = float(value)
                        MaszynaScenarioEventServer.MEMORY_FIELD_VALUE2:
                            condition.value2_operator = operator
                            condition.value2 = float(value)
                condition.mask = mask
                conditioned = true
            elif keyword == "probability" or keyword == "propability":
                condition.probability = float(event.condition[position])
                position += 1
                conditioned = true
            elif keyword == "trackoccupied" or keyword == "trackfree":
                # the event's targets as tracks; one that is no track drops the test (Event.cpp:44-59)
                var condition_tracks:Array[RID] = []
                for target:String in event.targets:
                    var track:RID = TrackServer.track_get_rid_by_name(target)
                    if track.is_valid():
                        condition_tracks.append(track)
                if condition_tracks.size() == event.targets.size():
                    condition.tracks = condition_tracks
                    condition.track_test = (
                        MaszynaEventCondition.TRACK_TEST_OCCUPIED if keyword == "trackoccupied"
                        else MaszynaEventCondition.TRACK_TEST_FREE
                    )
                    conditioned = true
        if conditioned:
            ScenarioEventServer.event_attach_condition(rid, condition)

    # A name defined more than once runs its definitions in their order, each with its own
    # condition, at the time and with the activator the name was queued with: the original hangs
    # the later ones on the first and puts each in its place in the queue as it runs
    # (event_manager::insert(), Event.cpp:2323-2334; CheckQuery(), :2471-2480). The name's event
    # queues them, with no delay of their own, so they run on the next frame, in their order.
    # A definition read without joinduplicatedevents disables the first (m_ignored, :2328-2334).
    # What starts the scenery queued is set up at its end.
    var chain_starts:Dictionary[int, RID] = {}
    for event_name:String in definitions:
        var indices:Array = definitions[event_name]
        if indices.size() == 1:
            continue
        var head:MaszynaEventData = kept_events[indices[0]]
        var head_rid:RID = event_rids[indices[0]]
        var named:RID = events_by_name[event_name]
        var ignored:bool = false
        var chain:Array[RID] = []
        for position:int in indices.size():
            var definition:MaszynaEventData = kept_events[indices[position]]
            chain.append(event_rids[indices[position]])
            if position == 0:
                continue
            if not definition.joined:
                ignored = true
                push_warning("Bad scenario: duplicate event name \"%s\"" % event_name)
            # one hung on another is queued (basic_event::append(), Event.cpp:393); a `getvalues`
            # becomes passive again when it is initialised, after (:577-588)
            if definition.type == "putvalues":
                MaszynaScenarioEventServer.event_set_passive(event_rids[indices[position]], false)
        # one queued by itself at the start runs the ones after it (InitEvents(), :2493-2499)
        for position:int in range(1, indices.size()):
            var definition:MaszynaEventData = kept_events[indices[position]]
            if definition.delay < 0.0:
                chain_starts[indices[position]] = _create_chain_event(root, system, definition, chain.slice(position))
        # the first of an "onstart" name is queued as it is read, before any other of its name
        # is (event_manager::insert(), :2338-2345)
        var onstart:bool = ONSTART in event_name
        if MaszynaScenarioEventServer.event_is_passive(head_rid):
            # a passive first is never queued, so neither are the ones after it; drivers read it
            # (AddToQuery(), :2383)
            _set_delays(named, head)
            ScenarioEventServer.event_attach_action(named, ScenarioEventServer.event_get_action(head_rid))
            ScenarioEventServer.event_attach_condition(named, ScenarioEventServer.event_get_condition(head_rid))
            MaszynaScenarioEventServer.event_set_passive(named, true)
        elif head.type == "addvalues" and head.delay == 0.0:
            # an instant first runs at once, and the next one is queued with its own delays; a
            # disabled one does not run (AddToQuery(), :2388-2401). Read alone, it ran alone.
            var next:MaszynaEventData = kept_events[indices[1]]
            if ignored:
                _set_chain(named, next, chain.slice(1))
            else:
                var tail:Array[RID] = [head_rid, _create_chain_event(root, system, next, chain.slice(1))]
                _set_chain(named, head, tail)
            if onstart:
                chain_starts[indices[0]] = head_rid
        elif ignored and not head.type == "multiple":
            # a disabled first is never queued (AddToQuery(), :2402-2403); queued before it was
            # disabled, it leaves its place in the queue to the next one (CheckQuery(), :2472-2480)
            _set_delays(named, head)
            if onstart:
                chain_starts[indices[0]] = _create_chain_event(root, system, head, chain.slice(1))
        else:
            # a disabled `multiple` comes back without its memory comparison (multi_event::init(),
            # :1218-1222)
            var head_condition:MaszynaEventCondition = ScenarioEventServer.event_get_condition(head_rid)
            if ignored and head_condition:
                var compared:Array[RID] = []
                head_condition.memories = compared
            _set_chain(named, head, chain)
            chain_starts[indices[0]] = named

    # the events a track names for its slots, and the ones named after the track and the slot
    for index:int in tracks.size():
        var track_data:TrackData = tracks[index]
        var scn_track:MaszynaTrackData = track_data as MaszynaTrackData
        for key:String in TRACK_EVENTS:
            var names:PackedStringArray = []
            if scn_track:
                for event_name:String in scn_track.events.get(key, PackedStringArray()):
                    names.append(event_name.to_lower())
            if track_data.name:
                names.append(track_data.name.to_lower() + ":" + key)
            for event_name:String in names:
                if events_by_name.has(event_name):
                    MaszynaScenarioEventServer.track_add_event(track_rids[index], TRACK_EVENTS[key], events_by_name[event_name])
                    # a passenger stop's track is a stop point of its station
                    var stop:MaszynaVehicleCommandAction = ScenarioEventServer.event_get_action(
                            events_by_name[event_name]) as MaszynaVehicleCommandAction
                    if stop and stop.command.begins_with(PASSENGER_STOP_POINT):
                        MaszynaStations.add_stop_point(
                                root, stop.command.trim_prefix(PASSENGER_STOP_POINT), track_rids[index])

    # a section's own memory is marked before its named events run, as the original updates it at
    # once (Track.cpp:128-130, 155-156) and the events are queued
    for section_name:String in sections:
        var section:RID = sections[section_name]
        for marker_mode:MaszynaMemoryAction.Mode in ISOLATED_MARKERS:
            var marker:RID = _create_event(root, system)
            var marked:Array[RID] = [memories[section_name]]
            var action:MaszynaMemoryAction = MaszynaMemoryAction.new()
            action.memories = marked
            action.mask = MaszynaScenarioEventServer.MEMORY_FIELD_VALUE2
            action.mode = marker_mode
            ScenarioEventServer.event_attach_action(marker, action)
            MaszynaScenarioEventServer.isolated_add_event(section, ISOLATED_MARKERS[marker_mode], marker)
        for suffix:String in ISOLATED_EVENTS:
            if events_by_name.has(section_name + suffix):
                MaszynaScenarioEventServer.isolated_add_event(section, ISOLATED_EVENTS[suffix], events_by_name[section_name + suffix])

    var launchers_by_name:Dictionary[String, Array] = {}
    for launcher_data:MaszynaEventLauncherData in launchers:
        var launcher:RID = MaszynaScenarioEventServer.launcher_create(system)
        if not launchers_by_name.has(launcher_data.name.to_lower()):
            launchers_by_name[launcher_data.name.to_lower()] = []
        launchers_by_name[launcher_data.name.to_lower()].append([launcher_data, launcher])
        MaszynaScenarioEventServer.launcher_set_name(launcher, launcher_data.name)
        MaszynaScenarioEventServer.launcher_set_position(launcher, launcher_data.position)
        MaszynaScenarioEventServer.launcher_set_radius(launcher, launcher_data.radius)
        MaszynaScenarioEventServer.launcher_set_events(
            launcher, events_by_name.get(launcher_data.event1, RID()), events_by_name.get(launcher_data.event2, RID())
        )
        # key codes are not ported (TODO.md)
        if RADIO_CALLS.has(launcher_data.key):
            MaszynaScenarioEventServer.launcher_set_radio_call(launcher, RADIO_CALLS[launcher_data.key])
        elif launcher_data.key.length() == 1:
            MaszynaScenarioEventServer.launcher_set_key(launcher, OS.find_keycode_from_string(launcher_data.key.to_upper()))
        if launcher_data.condition:
            var condition:MaszynaEventCondition = MaszynaEventCondition.new()
            _set_memcompare(
                condition, _get_memories(launcher_data.condition.slice(0, 1), memories), launcher_data.condition.slice(1)
            )
            MaszynaScenarioEventServer.launcher_attach_condition(launcher, condition)
        if launcher_data.delta_time > 0.0:
            var hhmm:int = int(launcher_data.delta_time)
            MaszynaScenarioEventServer.launcher_set_time_of_day(launcher, floori(hhmm / float(HHMM_HOUR)), hhmm % HHMM_HOUR)
        elif launcher_data.delta_time < 0.0 and not launcher_data.delta_time == FIRE_ONCE_IN_RANGE:
            MaszynaScenarioEventServer.launcher_set_interval(launcher, -launcher_data.delta_time)

    # a click on a model fires the launchers of its name that have it within their radius
    # (basic_cell::on_click(), scene.cpp:33-43); a negative radius has nothing within it
    # (EvLaunch.cpp:57-58); a model with no name fires none, whatever unnamed launcher is near
    # (basic_region::on_click(), scene.cpp:1055 - a crossing's launcher is unnamed, #332)
    for index:int in models.size():
        if not models[index].name:
            continue
        for entry:Array in launchers_by_name.get(models[index].name.to_lower(), []):
            var launcher_data:MaszynaEventLauncherData = entry[0]
            if not launcher_data.position.distance_to(models[index].position) < launcher_data.radius:
                continue
            var launcher:RID = entry[1]
            # the launcher's key does what a click does, with Shift the second event
            # (maszyna_scenario_keyboard.gd)
            var hints:PackedStringArray = []
            var key:Key = MaszynaScenarioEventServer.launcher_get_key(launcher)
            if not key == KEY_NONE:
                var key_text:String = InputEventNames.key_name(key)
                hints.append(key_text)
                if not launcher_data.event2 == "":
                    hints.append("Shift+" + key_text)
            root._pickable_rids.append(SceneryHUDMouseServer.pickable_create(
                model_rids[index],
                launcher_data.name,
                " / ".join(hints),
                MaszynaScenarioEventServer.launcher_fire.bind(launcher),
                MaszynaScenarioEventServer.launcher_fire_shift.bind(launcher),
            ))

    # queued at the start: a negative delay (InitEvents(), Event.cpp:2493-2499) and the first event
    # of each name containing "onstart" (Event.cpp:2336-2345); of a name defined more than once,
    # what its setup above left to start
    var started:Dictionary[String, bool] = {}
    for index:int in kept_events.size():
        var event:MaszynaEventData = kept_events[index]
        var onstart:bool = ONSTART in event.name and not started.has(event.name)
        started[event.name] = true
        if not (event.delay < 0.0 or onstart):
            continue
        if definitions[event.name].size() == 1:
            ScenarioEventServer.event_queue(event_rids[index])
        elif chain_starts.has(index):
            ScenarioEventServer.event_queue(chain_starts[index])


## The event runs as the definition says: its delay, random delay and departure delay
static func _set_delays(rid:RID, definition:MaszynaEventData) -> void:
    ScenarioEventServer.event_set_delay(rid, absf(definition.delay))
    ScenarioEventServer.event_set_random_delay(rid, definition.random_delay)
    MaszynaScenarioEventServer.event_set_departure_delay(rid, definition.departure_delay)


## The event runs the chain of definitions of one name, in their order, with the delays of the
## given definition (see the setup of the names defined more than once)
static func _set_chain(rid:RID, definition:MaszynaEventData, chain:Array[RID]) -> void:
    _set_delays(rid, definition)
    var action:MaszynaMultipleAction = MaszynaMultipleAction.new()
    action.events = chain
    ScenarioEventServer.event_attach_action(rid, action)


## A new event of the scenery's event system, freed with the scenery
static func _create_event(root:SceneryContentNode, system:RID) -> RID:
    var rid:RID = ScenarioEventServer.event_create()
    root._event_rids.append(rid)
    ScenarioEventServer.system_add_event(system, rid)
    return rid


## A new, unnamed event running the chain (_set_chain())
static func _create_chain_event(
    root:SceneryContentNode, system:RID, definition:MaszynaEventData, chain:Array[RID]
) -> RID:
    var rid:RID = _create_event(root, system)
    _set_chain(rid, definition, chain)
    return rid


## The fields a `<text> <value1> <value2>` triple sets or compares - all but those given as `*`
static func _fields_mask(fields:PackedStringArray) -> int:
    var mask:int = 0
    if not fields[0] == FIELD_KEPT:
        mask |= MaszynaScenarioEventServer.MEMORY_FIELD_TEXT
    if not fields[1] == FIELD_KEPT:
        mask |= MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1
    if not fields[2] == FIELD_KEPT:
        mask |= MaszynaScenarioEventServer.MEMORY_FIELD_VALUE2
    return mask


static func _get_memories(names:PackedStringArray, memories:Dictionary[String, RID]) -> Array[RID]:
    var found:Array[RID] = []
    for memory_name:String in names:
        if memories.has(memory_name):
            found.append(memories[memory_name])
    return found


## `memcompare <text> <value1> <value2>` - a memory that does not exist passes, as in the original
## (Event.cpp:131-134), so it is simply left out
static func _set_memcompare(
    condition:MaszynaEventCondition, compared:Array[RID], fields:PackedStringArray
) -> void:
    condition.memories = compared
    condition.mask = _fields_mask(fields)
    condition.text = fields[0]
    condition.value1 = float(fields[1])
    condition.value2 = float(fields[2])
