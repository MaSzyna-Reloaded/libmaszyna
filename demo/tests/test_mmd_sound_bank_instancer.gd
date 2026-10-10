extends MaszynaGutTest

const FIXTURE_PATH := "res://tests/fixtures/test_sound.mmd"
## An EN57 middle car: `doors: -3.71 both 2.94 both end` and a departuresignal: sound
const EN57_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.mmd"
const EN57_CAR_DOORS:Array[float] = [-3.71, 2.94]
## The door offsets are read as 32-bit floats
const FLOAT32_TOLERANCE:Vector3 = Vector3(0.000001, 0.000001, 0.000001)


func test_ignition_merges_into_empty_engine_sound_begin_but_explicit_soundend_wins_over_shutdown():
    var context := MmdImportContext.new()
    context.base_dir = FIXTURE_PATH.get_base_dir()
    var definitions:Array[MmdSoundSourceDefinition] = MmdSoundSourceParser.parse(FIXTURE_PATH, context)
    var internal_data:Array[MmdSoundSourceDefinition] = MmdSoundSourceParser.parse_internal_data(FIXTURE_PATH, context)
    MmdSoundBankInstancer._merge_ignition_and_shutdown_into_engine(definitions, internal_data)

    var engine:MmdSoundSourceDefinition = null
    for definition:MmdSoundSourceDefinition in definitions:
        if definition.label == "engine":
            engine = definition
    assert_not_null(engine)

    # engine: itself has no soundbegin: in the fixture - ignition: (internaldata:) fills it in.
    assert_eq(engine.sound_begin, "engine-start")
    # engine: DOES have its own soundend: ("engine-stop") - shutdown:'s "engine-shutdown-unused"
    # must NOT override it.
    assert_eq(engine.sound_end, "engine-stop")
    # the chunk table from engine:'s own block is untouched by the merge.
    assert_eq(engine.chunks.size(), 3)


func test_buzzer_and_buzzershp_are_catalog_matched_as_standalone_events():
    # unlike ignition:/shutdown: (merged into "engine" and never catalog-matched themselves),
    # buzzer:/buzzershp: are their own independent bank events - Train.cpp:10111-10151 plays them
    # as a SEPARATE, later-triggered stage from the light's own on/off click.
    assert_true(MmdSoundCatalog.has_label("buzzer"))
    assert_true(MmdSoundCatalog.has_label("buzzershp"))
    assert_eq(MmdSoundCatalog.get_entry("buzzer")["state_property"], "beeping")
    assert_eq(MmdSoundCatalog.get_entry("buzzershp")["state_property"], "cabsignal_beeping")


func test_internal_buzzers_route_to_the_cabin_bank() -> void:
    for label:String in ["buzzer", "buzzershp"]:
        var definition:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
        definition.label = label
        MmdSoundBankInstancer._apply_original_defaults(definition, true)

        assert_eq(definition.placement, &"internal")


func test_build_creates_independent_exterior_and_cabin_banks() -> void:
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    add_child(vehicle)
    var diagnostics:Array[Dictionary] = []
    MmdSoundBankInstancer.build_into(
            vehicle, RID(), ProjectSettings.globalize_path(FIXTURE_PATH), {}, {}, diagnostics)

    var exterior:SfxPlayer3D = vehicle.get_node("ExteriorSfxPlayer3D") as SfxPlayer3D
    var cabin:SfxPlayer3D = vehicle.get_node("CabinSfxPlayer3D") as SfxPlayer3D
    assert_not_null(exterior)
    assert_not_null(cabin)
    assert_eq(exterior.max_tracks, 16)
    assert_eq(cabin.max_tracks, 16)
    assert_ne(exterior.bank, cabin.bank)
    assert_not_null(exterior.bank.get_event(&"engine"))
    assert_null(exterior.bank.get_event(&"buzzer"))
    assert_not_null(cabin.bank.get_event(&"buzzer"))
    assert_null(cabin.bank.get_event(&"engine"))

    vehicle.queue_free()
    await wait_idle_frames(2)


## Train.cpp:8323-8335 - the Hasler (tachoclock:) is one cab event whose soundN chunks are
## crossfaded by speed in km/h.
func test_tachoclock_builds_one_cabin_event_with_speed_chunks() -> void:
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    add_child(vehicle)
    var diagnostics:Array[Dictionary] = []
    MmdSoundBankInstancer.build_into(
            vehicle, RID(), ProjectSettings.globalize_path(FIXTURE_PATH), {}, {}, diagnostics)

    var cabin:SfxPlayer3D = vehicle.get_node("CabinSfxPlayer3D") as SfxPlayer3D
    var event:SfxEvent = cabin.bank.get_event(&"tachoclock")
    assert_not_null(event)
    if event:
        assert_eq(event.automations.size(), 1)
        var automation:SfxAutomation = event.automations[0]
        assert_eq(automation.parameter_name, &"speed")
        assert_eq(automation.clips.size(), 3)
    assert_eq(MmdSoundCatalog.get_entry("tachoclock")["state_property"], "tachometer_clock_speed")

    vehicle.queue_free()
    await wait_idle_frames(2)


func test_original_default_placements_keep_engine_and_horns_external() -> void:
    var engine:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    engine.label = "engine"
    MmdSoundBankInstancer._apply_original_defaults(engine, false)
    assert_eq(engine.placement, &"engine")

    var horn:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    horn.label = "horn1"
    MmdSoundBankInstancer._apply_original_defaults(horn, false)
    assert_eq(horn.placement, &"external")


func test_explicit_placement_wins_over_original_defaults() -> void:
    var definition:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    definition.label = "engine"
    definition.placement = &"custom"
    definition.placement_defined = true
    MmdSoundBankInstancer._apply_original_defaults(definition, false)

    assert_eq(definition.placement, &"custom")


func test_brake_sounds_are_built_with_their_bookends_chunks_and_fallbacks() -> void:
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    add_child_autofree(vehicle)
    var diagnostics:Array[Dictionary] = []
    MmdSoundBankInstancer.build_into(
            vehicle, RID(), ProjectSettings.globalize_path("res://tests/fixtures/test_brake_sound.mmd"), {}, {}, diagnostics)
    var exterior:SfxPlayer3D = vehicle.get_node("ExteriorSfxPlayer3D") as SfxPlayer3D
    var cabin:SfxPlayer3D = vehicle.get_node("CabinSfxPlayer3D") as SfxPlayer3D

    # the local brake engage hiss plays its opening bookend, its loop, and its closing one on stop
    var engage:SfxEvent = cabin.bank.get_event(&"local_brake_engage_hiss")
    assert_not_null(engage)
    assert_eq(engage.clips.size(), 3)
    assert_eq(engage.clips[2].trigger_mode, SfxClip.TriggerMode.TRIGGER_SUSTAIN)
    # no local brake release hiss in the cab - a copy of the main valve's (Train.cpp:9082-9089)
    var release:SfxEvent = cabin.bank.get_event(&"local_brake_release_hiss")
    assert_not_null(release)
    assert_eq((release.clips[0].stream as MaszynaAudioStream).file_path,
            (cabin.bank.get_event(&"brake_valve_braking_hiss").clips[0].stream as MaszynaAudioStream).file_path)

    # a vehicle without unbrake: plays the default one (DynObj.cpp:7059-7062)
    assert_not_null(exterior.bank.get_event(&"brake_release_hiss"))
    assert_not_null(exterior.bank.get_event(&"brake_releaser"))
    assert_not_null(exterior.bank.get_event(&"emergency_brake_hiss"))
    # a combined cylinder click is one one-shot event per chunk
    assert_not_null(exterior.bank.get_event(&"brake_cylinder_increase_0"))
    assert_not_null(exterior.bank.get_event(&"brake_cylinder_increase_1"))
    assert_null(exterior.bank.get_event(&"brake_cylinder_increase"))


## DynObj.cpp:6359-6364, 6594-6639 - the departure signal sounds from a speaker at every door
## location, none of them at the sound's own offset
func test_the_departure_signal_is_built_at_every_door_speaker() -> void:
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    add_child(vehicle)
    var diagnostics:Array[Dictionary] = []
    MmdSoundBankInstancer.build_into(
            vehicle, RID(), ProjectSettings.globalize_path(EN57_CAR_PATH), {}, {}, diagnostics)

    var exterior:SfxPlayer3D = vehicle.get_node("ExteriorSfxPlayer3D") as SfxPlayer3D
    assert_null(exterior.bank.get_event(&"departure_signal"), "no copy at the sound's own place")
    var front:SfxEvent = exterior.bank.get_event(&"departure_signal_0")
    var rear:SfxEvent = exterior.bank.get_event(&"departure_signal_1")
    assert_not_null(front)
    assert_not_null(rear)
    assert_almost_eq(front.spatial_config.position,
            Vector3(0.0, MmdSoundBankInstancer.DOOR_SPEAKER_HEIGHT, EN57_CAR_DOORS[0]), FLOAT32_TOLERANCE)
    assert_almost_eq(rear.spatial_config.position,
            Vector3(0.0, MmdSoundBankInstancer.DOOR_SPEAKER_HEIGHT, EN57_CAR_DOORS[1]), FLOAT32_TOLERANCE)
    assert_null(exterior.bank.get_event(&"departure_signal_2"), "two doors, two speakers")

    vehicle.queue_free()
    await wait_idle_frames(2)


func test_a_door_location_is_its_offset_whatever_its_sides() -> void:
    var context:MmdImportContext = MmdImportContext.new()
    context.base_dir = EN57_CAR_PATH.get_base_dir()

    var locations:Dictionary = MmdSoundSourceParser.parse_locations(ProjectSettings.globalize_path(EN57_CAR_PATH), context)

    assert_eq(locations["doors"], PackedFloat32Array(EN57_CAR_DOORS))
    assert_eq(locations["bogies"], PackedFloat32Array([-7.59, 7.27]), "the next list read as before")
