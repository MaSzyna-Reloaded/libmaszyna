extends RefCounted
class_name MmdSoundBankInstancer

const _INTERNAL_BRAKE_LABELS:Array[String] = [
    "brakesound", "airsound", "airsound2", "airsound3", "airsound4", "airsound5",
    "localbrakesound", "localbrakesound2",
]
const _VEHICLE_PLAYER_VOICE_COUNT:int = 16
const _HORN_LABELS:Array[String] = ["horn1", "horn2", "horn3"]
## Coupler sounds, external and read from internaldata: too (DynObj.cpp:6409-6520, sound_placement::external)
const _COUPLER_LABELS:Array[String] = [
    "couplerattach", "brakehoseattach", "mainhoseattach", "controlattach", "gangwayattach", "heatingattach",
    "couplerdetach", "brakehosedetach", "mainhosedetach", "controldetach", "gangwaydetach", "heatingdetach",
    "coupleradapterattach", "coupleradapterremove",
]
## Used when the vehicle defines none (DynObj.cpp:6693-6700, 7059-7062)
const _DEFAULT_SOUNDS:Dictionary = {
    "couplerattach": "couplerattach_default",
    "couplerdetach": "couplerdetach_default",
    "unbrake": "[1007]estluz",
}
## A cab without its own local brake hiss plays a copy of the main valve's (Train.cpp:9082-9089)
const _LOCAL_BRAKE_FALLBACKS:Dictionary = {
    "localbrakesound": "airsound",
    "localbrakesound2": "airsound2",
}
## Placement and range of the running sounds when the MMD gives none (DynObj.h:390, 528-533,
## DynObj.cpp:5711, 6125; Train.cpp:8571)
const _RUNNING_PLACEMENTS:Dictionary = {
    "tractionmotor": &"external",
    "ventilator": &"engine",
    "curve": &"external",
    "outernoise": &"external",
    "wheel_clatter": &"external",
}
## Where an outernoise bogie copy starts in its sample, as a fraction of it: an even copy in the
## later range, an odd one in the earlier, so two neighbours never start close (DynObj.cpp:6505-6514)
const _BOGIE_NOISE_EVEN_START_MIN:float = 0.5
const _BOGIE_NOISE_EVEN_START_MAX:float = 0.8
const _BOGIE_NOISE_ODD_START_MIN:float = 0.0
const _BOGIE_NOISE_ODD_START_MAX:float = 0.3
## How high a door speaker stands over the rail [m] (DynObj.cpp:6636)
const DOOR_SPEAKER_HEIGHT:float = 3.0
const _RUNNING_RANGES:Dictionary = {
    "curve": 200.0,
    "outernoise": 200.0,
    "runningnoise": -1.0,
}
## Cab instruments, as unit_size/max_distance rather than a range: the generic mapping
## (MmdSoundEventBuilder._build_spatial_config, range/16 and range*7.5) ties the two together in
## a ratio meant for sources out in the world, so no single range fits. A Hasler has to be
## plainly audible standing anywhere in the cab - that is unit_size of roughly the cab's own
## size - while not carrying across the station the way the old 375 m cutoff did. Deriving the
## 20 m cutoff from a range instead would force unit_size down to 0.17 and leave the ticking
## audible only with one's head against the gauge. Same escape hatch the horns already use
## (_apply_horn_spatial_config). The alerter/SHP buzzers are not here: they are meant to fill
## the cab, not to come from a point on the desk - buzzer is here only to widen its unit_size
## past what range 50 gives (3.125), keeping that range's own 375 m cutoff untouched.
##
## unit_size is where the falloff starts, not how loud the sound is: inside it the gain is
## clamped to 1.0 and does not change at all. A Hasler at unit_size 3 is therefore dead flat
## across the whole cab. Leaning into the gauge only reads as leaning in when unit_size is
## smaller than that movement, so the level is set by volume_db instead and the falloff is left
## steep - inverse-square, so the last metre or two is where most of it happens. unit_size 1.0
## puts the flat zone at the gauge itself and the driver's seat on the slope, a bit over 10 dB
## below it.
const _CABIN_SPATIAL:Dictionary = {
    "tachoclock": {
        "unit_size": 1.0,
        "max_distance": 20.0,
        "attenuation": AudioStreamPlayer3D.ATTENUATION_INVERSE_SQUARE_DISTANCE,
        "volume_db": 12.0,
    },
    "buzzer": {"unit_size": 6.0, "max_distance": 375.0},
}
const _HORN_RANGE_UNIT_DIVISOR:float = 24.0
const _HORN_MAX_DISTANCE_FACTOR:float = 2.0
static var _HORN_SOUNDPROOFING:PackedFloat32Array = PackedFloat32Array([0.65, 1.0, 0.65, 1.0, 1.0, 1.0])


## The sound players of the vehicle `vehicle_rid`, built into `parent` - a node riding on the
## vehicle - as its internal children; returned
static func build_into(
        parent:Node3D, vehicle_rid:RID, abs_mmd_path:String, parameters:Dictionary, random_choices:Dictionary,
        diagnostics:Array[Dictionary]) -> Array[Node]:
    var context := MmdImportContext.new()
    context.base_dir = abs_mmd_path.get_base_dir()
    context.parameters = parameters
    context.random_choices = random_choices

    var exterior_definitions:Array[MmdSoundSourceDefinition] = MmdSoundSourceParser.parse(abs_mmd_path, context)
    var internal_data:Array[MmdSoundSourceDefinition] = MmdSoundSourceParser.parse_internal_data(abs_mmd_path, context)
    var soundproofing:Array[PackedFloat32Array] = MmdSoundSourceParser.parse_vehicle_soundproofing(abs_mmd_path, context)
    var locations:Dictionary = MmdSoundSourceParser.parse_locations(abs_mmd_path, context)
    _merge_ignition_and_shutdown_into_engine(exterior_definitions, internal_data)
    # outernoise: takes the vehicle's Vmax for its Chunkrange (DynObj.cpp:6389)
    var controller:VehicleController = VehicleServer.vehicle_get_controller(vehicle_rid)
    for definition:MmdSoundSourceDefinition in exterior_definitions:
        if definition.label == "outernoise" and controller:
            definition.chunk_range = int(controller.max_velocity)

    var cabin_definitions:Array[MmdSoundSourceDefinition] = []
    for definition:MmdSoundSourceDefinition in internal_data:
        if definition.label in ["ignition", "shutdown"]:
            continue
        if not definition.label in [
                "buzzer", "buzzershp", "tachoclock", "brakesound", "slipperysound", "airsound", "airsound2",
                "airsound3", "airsound4", "airsound5", "localbrakesound", "localbrakesound2", "runningnoise"] \
                and not definition.label in _COUPLER_LABELS:
            continue
        _apply_original_defaults(definition, true)
        if definition.placement == &"internal":
            cabin_definitions.append(definition)
        else:
            exterior_definitions.append(definition)

    _add_default_sounds(exterior_definitions)

    var routed_exterior:Array[MmdSoundSourceDefinition] = []
    var running_exterior:Array[MmdSoundSourceDefinition] = []
    for definition:MmdSoundSourceDefinition in exterior_definitions:
        _apply_original_defaults(definition, false)
        if definition.placement == &"internal":
            cabin_definitions.append(definition)
        elif _is_running(definition):
            running_exterior.append(definition)
        else:
            routed_exterior.append(definition)

    var players:Array[Node] = [
        _build_player(parent, vehicle_rid, "ExteriorSfxPlayer3D", routed_exterior, soundproofing, context, abs_mmd_path, false, locations),
        _build_player(parent, vehicle_rid, "CabinSfxPlayer3D", cabin_definitions, soundproofing, context, abs_mmd_path, true, locations),
    ]
    # own voice pool, so the looping running sounds never steal from (or lose to) the others
    if running_exterior:
        players.append(_build_player(
                parent, vehicle_rid, "RunningSfxPlayer3D", running_exterior, soundproofing, context, abs_mmd_path, false, locations))
    diagnostics.append_array(context.diagnostics)
    return players


static func _build_player(
        parent:Node3D, vehicle_rid:RID, player_name:String, definitions:Array[MmdSoundSourceDefinition],
        soundproofing:Array[PackedFloat32Array], context:MmdImportContext,
        abs_mmd_path:String, cabin_only:bool, locations:Dictionary) -> SfxPlayer3D:
    var events:Array[SfxEvent] = []
    # {definition, event_name} of every event a trigger starts
    var regular:Array[Dictionary] = []
    var brake_definitions:Dictionary[String, MmdSoundSourceDefinition] = {}
    var running := RunningSoundModel.new()
    for definition:MmdSoundSourceDefinition in definitions:
        if not MmdSoundCatalog.has_label(definition.label):
            context.warn_unsupported_label(definition.label, abs_mmd_path, 0)
            continue
        var entry:Dictionary = MmdSoundCatalog.get_entry(definition.label)
        if entry.get("controller", &"") == &"brake":
            _apply_brake_source_defaults(definition)
            brake_definitions[definition.label] = definition
            continue
        if entry.get("controller", &"") == &"running":
            _build_running_events(definition, entry["event_name"], locations, events, running)
            continue
        # one copy at every door speaker, at its offset in place of the sound's own (DynObj.cpp:6362),
        # none without door locations; MMD offsets turned into the vehicle's -Z forward frame
        var positions:Array = [null]
        if entry.get("at_door_speakers", false):
            positions = Array(locations["doors"]).map(func(offset:float) -> Vector3:
                return Vector3(0.0, DOOR_SPEAKER_HEIGHT, offset))
        for index:int in range(positions.size()):
            var event_name:StringName = (entry["event_name"] if not positions[index] is Vector3
                    else StringName("%s_%d" % [entry["event_name"], index]))
            # CHANGE triggers only start their event - a one-shot sample, never stopped
            var event:SfxEvent = MmdSoundEventBuilder.build(
                    definition, event_name, entry.get("sound_parameter", &""), false, true,
                    not entry["trigger_mode"] == TrainSoundTrigger.TriggerMode.CHANGE)
            if definition.label in _HORN_LABELS:
                _apply_horn_spatial_config(event, definition)
            elif _CABIN_SPATIAL.has(definition.label) and not definition.range_defined:
                _apply_cabin_spatial_config(event, definition.label)
            if positions[index] is Vector3:
                event.spatial_config.position = positions[index]
            var emitter:Array[SfxEvent] = [event]
            MmdSoundEventBuilder.shape_emitter(emitter, definition, definition.start_offset)
            events.append(event)
            regular.append({"definition": definition, "event_name": event_name})

    for label:String in _LOCAL_BRAKE_FALLBACKS:
        var fallback:MmdSoundSourceDefinition = brake_definitions.get(_LOCAL_BRAKE_FALLBACKS[label])
        if fallback and not brake_definitions.has(label):
            brake_definitions[label] = fallback.copy_as(label)
    var brakes := BrakeSoundModel.new()
    for definition:MmdSoundSourceDefinition in brake_definitions.values():
        _build_brake_sound(definition, MmdSoundCatalog.get_entry(definition.label)["event_name"], events, brakes)

    var bank := SfxBank.new()
    bank.events = events
    var player := SfxPlayer3D.new()
    player.name = player_name
    player.bank = bank
    # a running sound (or a clatter axle) crossfades at most two chunks at once
    player.max_tracks = (
            2 * running.sources.size() if running.sources and not (regular or brake_definitions)
            else _VEHICLE_PLAYER_VOICE_COUNT)
    player.attenuation_model = AudioStreamPlayer3D.ATTENUATION_INVERSE_DISTANCE
    player.unit_size = 20.0
    player.max_distance = 100.0
    # Cab and outside each go to their own bus, so either space can be shaped (filter, level)
    # without touching the calibration of the individual events. The running-sound player is
    # built with cabin_only false and lands on Exterior with the rest of the outside.
    player.bus = &"Cabin" if cabin_only else &"Exterior"
    parent.add_child(player, false, Node.INTERNAL_MODE_BACK)

    var triggers:Array[Dictionary] = []
    for source:Dictionary in regular:
        var definition:MmdSoundSourceDefinition = source["definition"]
        var entry:Dictionary = MmdSoundCatalog.get_entry(definition.label)
        triggers.append({
            "state_property": entry["state_property"],
            "trigger_mode": entry["trigger_mode"],
            "sound_event": source["event_name"],
            "sound_parameter": entry.get("sound_parameter", &""),
            "trigger_threshold_min": entry.get("trigger_threshold_min", 0.0),
            "trigger_threshold_max": entry.get("trigger_threshold_max", 1.0),
            "source": definition,
        })

    TrainSoundSystem.register_bank(player, {
        "vehicle": vehicle_rid,
        "cabin_only": cabin_only,
        "triggers": triggers,
        "brakes": brakes if brakes.sounds else null,
        "running": running if running.sources else null,
        "soundproofing": soundproofing,
    })
    return player


static func _apply_original_defaults(definition:MmdSoundSourceDefinition, from_internal_data:bool) -> void:
    if definition.label in _HORN_LABELS and not definition.soundproofing.size() == 6:
        definition.soundproofing = _HORN_SOUNDPROOFING
    if _RUNNING_RANGES.has(definition.label) and not definition.range_defined:
        definition.range = _RUNNING_RANGES[definition.label]
    elif MmdSoundCatalog.get_entry(definition.label).has("range") and not definition.range_defined:
        definition.range = MmdSoundCatalog.get_entry(definition.label)["range"]
    if definition.placement_defined:
        return
    if from_internal_data:
        definition.placement = (
                &"external" if definition.label == "slipperysound" or definition.label in _COUPLER_LABELS
                else &"internal")
    elif definition.label == "engine":
        definition.placement = &"engine"
    elif definition.label in _HORN_LABELS:
        definition.placement = &"external"
    elif _RUNNING_PLACEMENTS.has(definition.label):
        definition.placement = _RUNNING_PLACEMENTS[definition.label]
    elif definition.label == "emergencybrake":
        # DynObj.h:543 - m_emergencybrake { sound_placement::engine }
        definition.placement = &"engine"
    elif MmdSoundCatalog.has_label(definition.label) \
            and MmdSoundCatalog.get_entry(definition.label).get("controller", &"") == &"brake":
        definition.placement = &"external"


static func _is_running(definition:MmdSoundSourceDefinition) -> bool:
    return MmdSoundCatalog.get_entry(definition.label).get("controller", &"") == &"running"


## One event per sound location: traction motors and bogie noise are repeated at every
## `tractionmotors:`/`bogies:` offset, keeping the x/y of the sound's own offset (DynObj.cpp:5722-5738,
## 6137-6146); a wheel clatter definition is one axle already. MMD offsets are in the original
## vehicle frame (+Z forward), turned by 180 degrees into the vehicle's -Z forward frame.
static func _build_running_events(
        definition:MmdSoundSourceDefinition, event_name:StringName, locations:Dictionary,
        events:Array[SfxEvent], running:RunningSoundModel) -> void:
    var offsets:Array[Vector3] = [definition.offset]
    var location_key:String = {"tractionmotor": "tractionmotors", "outernoise": "bogies"}.get(definition.label, "")
    if location_key and locations[location_key]:
        offsets.clear()
        for location:float in locations[location_key]:
            offsets.append(Vector3(definition.offset.x, definition.offset.y, -location))
    var first_index:int = 0
    if definition.label == "wheel_clatter":
        first_index = running.sources.filter(func(entry:Dictionary) -> bool:
            return (entry["source"] as MmdSoundSourceDefinition).label == "wheel_clatter").size()
    for index:int in range(offsets.size()):
        var event_id:StringName = event_name
        if offsets.size() > 1 or definition.label == "wheel_clatter":
            event_id = StringName("%s_%d" % [event_name, first_index + index])
        var position:Vector3 = Vector3(-offsets[index].x, offsets[index].y, -offsets[index].z)
        if definition.label == "wheel_clatter" and definition.chunks:
            running.sources.append({
                "event": event_id, "source": definition,
                "chunk_events": _build_chunk_events(definition, event_id, position, events),
            })
            continue
        var event:SfxEvent = MmdSoundEventBuilder.build(
                definition, event_id, &"point", true, true, not definition.label == "wheel_clatter")
        event.spatial_config.position = position
        # every copy its own start, as the original sets it when it reads the MMD: bogies
        # alternate between two ranges (DynObj.cpp:6505-6514), located motors start anywhere
        # (DynObj.cpp:6085), anything else where the MMD's startoffset: says
        var start_fraction:float = definition.start_offset
        if definition.label == "outernoise":
            start_fraction = (randf_range(_BOGIE_NOISE_ODD_START_MIN, _BOGIE_NOISE_ODD_START_MAX) if index % 2
                    else randf_range(_BOGIE_NOISE_EVEN_START_MIN, _BOGIE_NOISE_EVEN_START_MAX))
        elif definition.label == "tractionmotor" and location_key and locations[location_key]:
            start_fraction = randf()
        var emitter:Array[SfxEvent] = [event]
        MmdSoundEventBuilder.shape_emitter(emitter, definition, start_fraction)
        events.append(event)
        running.sources.append({"event": event_id, "source": definition})


## One brake sound's event, played as BrakeSoundModel decides: a loop with its bookends or its
## chunks crossfaded on `point`, or a one-shot
static func _build_brake_sound(
        definition:MmdSoundSourceDefinition, event_name:StringName, events:Array[SfxEvent],
        brakes:BrakeSoundModel) -> void:
    var chunk_events:Array[StringName] = []
    if definition.label in BrakeSoundModel.ONE_SHOT_LABELS and definition.chunks:
        chunk_events = _build_chunk_events(definition, event_name, definition.offset, events)
    else:
        var looped:bool = not definition.label in BrakeSoundModel.ONE_SHOT_LABELS
        var event:SfxEvent = MmdSoundEventBuilder.build(
                definition, event_name, &"point" if looped else &"", true, true, looped)
        var emitter:Array[SfxEvent] = [event]
        MmdSoundEventBuilder.shape_emitter(emitter, definition, definition.start_offset)
        events.append(event)
    brakes.add_sound(definition, event_name, chunk_events)


## A combined one-shot (a clatter click, a brake cylinder click) plays the chunk its value picks -
## one single-sample event per chunk, since an automation event never finishes while any of its
## chunks is left unplayed.
static func _build_chunk_events(
        definition:MmdSoundSourceDefinition, event_id:StringName, position:Vector3,
        events:Array[SfxEvent]) -> Array[StringName]:
    var chunk_events:Array[StringName] = []
    var emitter:Array[SfxEvent] = []
    for chunk:Dictionary in RunningSoundModel.sorted_chunks(definition):
        var chunk_definition := MmdSoundSourceDefinition.new()
        chunk_definition.label = definition.label
        chunk_definition.sound_main = chunk["filename"]
        chunk_definition.range = definition.range
        chunk_definition.placement = definition.placement
        chunk_definition.soundproofing = definition.soundproofing
        var chunk_event_id:StringName = StringName("%s_%d" % [event_id, chunk_events.size()])
        var event:SfxEvent = MmdSoundEventBuilder.build(chunk_definition, chunk_event_id, &"", true, true, false)
        event.spatial_config.position = position
        events.append(event)
        emitter.append(event)
        chunk_events.append(chunk_event_id)
    # one emitter whichever chunk it clicks with
    MmdSoundEventBuilder.shape_emitter(emitter, definition, definition.start_offset)
    return chunk_events


static func _apply_horn_spatial_config(event:SfxEvent, definition:MmdSoundSourceDefinition) -> void:
    if definition.range < 0.0:
        return
    event.spatial_config.unit_size = maxf(definition.range / _HORN_RANGE_UNIT_DIVISOR, 0.01)
    event.spatial_config.max_distance = minf(
            definition.range * _HORN_MAX_DISTANCE_FACTOR, 2750.0)


static func _apply_cabin_spatial_config(event:SfxEvent, label:String) -> void:
    var spatial:Dictionary = _CABIN_SPATIAL[label]
    event.spatial_config.unit_size = float(spatial["unit_size"])
    event.spatial_config.max_distance = float(spatial["max_distance"])
    event.spatial_config.attenuation_model = int(spatial.get(
            "attenuation", AudioStreamPlayer3D.ATTENUATION_INVERSE_DISTANCE))
    var volume_db:float = float(spatial.get("volume_db", 0.0))
    if is_zero_approx(volume_db):
        return
    for automation:SfxAutomation in event.automations:
        for clip:SfxClip in automation.clips:
            if clip.track:
                clip.track.volume_db += volume_db


static func _apply_brake_source_defaults(definition:MmdSoundSourceDefinition) -> void:
    if definition.label == "brake" and definition.amplitude_factor > 10.0:
        definition.amplitude_factor = 1.0
        definition.amplitude_offset = 0.0
    if not definition.range_defined:
        if definition.label == "brakesound" and definition.placement == &"internal":
            definition.range = -1.0
        elif definition.label in _INTERNAL_BRAKE_LABELS:
            definition.range = 7.5
        elif definition.label in ["brake", "brakesound", "slipperysound"]:
            definition.range = 100.0


static func _merge_ignition_and_shutdown_into_engine(
        definitions:Array[MmdSoundSourceDefinition], internal_data:Array[MmdSoundSourceDefinition]) -> void:
    var engine:MmdSoundSourceDefinition = null
    for definition:MmdSoundSourceDefinition in definitions:
        if definition.label == "engine":
            engine = definition
            break
    if not engine:
        return
    for definition:MmdSoundSourceDefinition in internal_data:
        if definition.label == "ignition" and not engine.sound_begin:
            engine.sound_begin = definition.sound_main
        elif definition.label == "shutdown" and not engine.sound_end:
            engine.sound_end = definition.sound_main


static func _add_default_sounds(definitions:Array[MmdSoundSourceDefinition]) -> void:
    for label:String in _DEFAULT_SOUNDS:
        if definitions.any(func(definition:MmdSoundSourceDefinition) -> bool: return definition.label == label):
            continue
        var definition := MmdSoundSourceDefinition.new()
        definition.label = label
        definition.sound_main = _DEFAULT_SOUNDS[label]
        definition.placement = &"external"
        definition.placement_defined = true
        definitions.append(definition)
