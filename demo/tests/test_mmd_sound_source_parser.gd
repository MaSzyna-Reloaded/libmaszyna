extends MaszynaGutTest

const FIXTURE_PATH := "res://tests/fixtures/test_sound.mmd"


func _parse() -> Array[MmdSoundSourceDefinition]:
    var context := MmdImportContext.new()
    context.base_dir = FIXTURE_PATH.get_base_dir()
    return MmdSoundSourceParser.parse(FIXTURE_PATH, context)


func _find(definitions:Array[MmdSoundSourceDefinition], label:String) -> MmdSoundSourceDefinition:
    for definition:MmdSoundSourceDefinition in definitions:
        if definition.label == label:
            return definition
    return null


func test_bare_single_filename_is_parsed_as_sound_main():
    var oilpump:MmdSoundSourceDefinition = _find(_parse(), "oilpump")
    assert_not_null(oilpump)
    assert_eq(oilpump.sound_main, "697_test-oilpump")


func test_numeric_filename_prefix_is_kept_and_extension_stripped():
    var fuelpump:MmdSoundSourceDefinition = _find(_parse(), "fuelpump")
    assert_not_null(fuelpump)
    assert_eq(fuelpump.sound_main, "[1007]test-fuelpump")


func test_bare_multipart_horn_is_parsed_into_begin_main_end():
    var horn1:MmdSoundSourceDefinition = _find(_parse(), "horn1")
    assert_not_null(horn1)
    assert_eq(horn1.sound_begin, "horn-sn61-start")
    assert_eq(horn1.sound_main, "horn-sn61")
    assert_eq(horn1.sound_end, "horn-sn61-stop")


# audio/sound.cpp:105-111 reads a legacy sound's files with "," among the delimiters
func test_bare_multipart_joined_by_commas_is_parsed_into_begin_main_end():
    var definitions:Array[MmdSoundSourceDefinition] = _parse()
    var small_compressor:MmdSoundSourceDefinition = _find(definitions, "small-compressor")
    assert_not_null(small_compressor)
    assert_eq(small_compressor.sound_begin, "s-compressor-start")
    assert_eq(small_compressor.sound_main, "s-compressor")
    assert_eq(small_compressor.sound_end, "s-compressor-stop")
    assert_eq(small_compressor.range, 20.0)
    assert_eq(_find(definitions, "pantographup").sound_main, "pantup1")


func test_block_begin_main_end_is_parsed():
    var battery:MmdSoundSourceDefinition = _find(_parse(), "battery")
    assert_not_null(battery)
    assert_eq(battery.sound_begin, "battery-start")
    assert_eq(battery.sound_main, "battery-trwa")
    assert_eq(battery.sound_end, "battery-stop")


func test_soundset_resolves_one_random_candidate_and_splits_into_begin_main_end():
    var compressor:MmdSoundSourceDefinition = _find(_parse(), "compressor")
    assert_not_null(compressor)
    assert_true(compressor.sound_begin in ["comp-a-start", "comp-b-start"])
    # all three parts must come from the SAME chosen candidate, not mixed
    var chosen_suffix:String = "a" if compressor.sound_begin == "comp-a-start" else "b"
    assert_eq(compressor.sound_main, "comp-%s-mid" % chosen_suffix)
    assert_eq(compressor.sound_end, "comp-%s-end" % chosen_suffix)


func test_soundset_random_choice_is_persisted_across_reparse():
    var random_choices:Dictionary = {}
    var context1 := MmdImportContext.new()
    context1.base_dir = FIXTURE_PATH.get_base_dir()
    context1.random_choices = random_choices
    var first:MmdSoundSourceDefinition = _find(MmdSoundSourceParser.parse(FIXTURE_PATH, context1), "compressor")

    var context2 := MmdImportContext.new()
    context2.base_dir = FIXTURE_PATH.get_base_dir()
    context2.random_choices = random_choices
    var second:MmdSoundSourceDefinition = _find(MmdSoundSourceParser.parse(FIXTURE_PATH, context2), "compressor")

    assert_eq(first.sound_begin, second.sound_begin)


func test_numbered_chunks_are_parsed_with_threshold_and_pitch():
    var engine:MmdSoundSourceDefinition = _find(_parse(), "engine")
    assert_not_null(engine)
    assert_eq(engine.chunks.size(), 3)
    var by_threshold:Dictionary = {}
    for chunk:Dictionary in engine.chunks:
        by_threshold[chunk["threshold"]] = chunk
    assert_eq(by_threshold[400]["filename"], "silence1")
    assert_almost_eq(float(by_threshold[400]["pitch"]), 400.0, 0.001)
    assert_eq(by_threshold[600]["filename"], "engine-idle-1")
    assert_eq(by_threshold[1000]["filename"], "engine-idle-2")


func test_crossfade_percent_is_parsed():
    var engine:MmdSoundSourceDefinition = _find(_parse(), "engine")
    assert_not_null(engine)
    assert_eq(engine.crossfade_percent, 90)


func test_unrecognized_label_does_not_desync_following_labels():
    var definitions:Array[MmdSoundSourceDefinition] = _parse()
    assert_not_null(_find(definitions, "unrecognizedlabel"))
    # if unrecognizedlabel's block braces were miscounted, this would either be missing entirely
    # or the loop would run past endsounds without stopping.
    assert_null(_find(definitions, "endsounds"))


const _INTERNAL_DATA_LABELS := [
    "ignition", "shutdown", "buzzer", "buzzershp", "tachoclock",
    "brakesound", "slipperysound", "localbrakesound", "localbrakesound2",
    "airsound", "airsound2",
]


func test_parse_internal_data_returns_ignition_shutdown_buzzer_and_buzzershp():
    var context := MmdImportContext.new()
    context.base_dir = FIXTURE_PATH.get_base_dir()
    var definitions:Array[MmdSoundSourceDefinition] = MmdSoundSourceParser.parse_internal_data(FIXTURE_PATH, context)

    var ignition:MmdSoundSourceDefinition = _find(definitions, "ignition")
    var shutdown:MmdSoundSourceDefinition = _find(definitions, "shutdown")
    var buzzer:MmdSoundSourceDefinition = _find(definitions, "buzzer")
    var buzzershp:MmdSoundSourceDefinition = _find(definitions, "buzzershp")

    assert_not_null(ignition)
    assert_eq(ignition.sound_main, "engine-start")

    assert_not_null(shutdown)
    assert_eq(shutdown.sound_main, "engine-shutdown-unused")

    assert_not_null(buzzer)
    assert_true(buzzer.sound_main in ["buczek", "24150_buczek_cashp"])

    assert_not_null(buzzershp)
    assert_eq(buzzershp.sound_main, "buzzershp-main")

    # rainsound:/cab1definition:/... are out of scope for this entry point - only the labels in
    # _INTERNAL_DATA_LABELS are collected, but parsing must still stay aligned across them (not
    # desync).
    for definition:MmdSoundSourceDefinition in definitions:
        assert_true(definition.label in _INTERNAL_DATA_LABELS)


func test_parse_internal_data_returns_brake_related_labels():
    var context := MmdImportContext.new()
    context.base_dir = FIXTURE_PATH.get_base_dir()
    var definitions:Array[MmdSoundSourceDefinition] = MmdSoundSourceParser.parse_internal_data(FIXTURE_PATH, context)

    var brakesound:MmdSoundSourceDefinition = _find(definitions, "brakesound")
    assert_not_null(brakesound)
    assert_eq(brakesound.sound_main, "cab-brakesound")

    var slipperysound:MmdSoundSourceDefinition = _find(definitions, "slipperysound")
    assert_not_null(slipperysound)
    assert_eq(slipperysound.sound_main, "cab-slippery")

    var airsound:MmdSoundSourceDefinition = _find(definitions, "airsound")
    assert_not_null(airsound)
    assert_eq(airsound.sound_main, "cab-airsound")

    var airsound2:MmdSoundSourceDefinition = _find(definitions, "airsound2")
    assert_not_null(airsound2)
    assert_eq(airsound2.sound_main, "[1007]cab-airsound2")

    var localbrakesound:MmdSoundSourceDefinition = _find(definitions, "localbrakesound")
    assert_not_null(localbrakesound)
    assert_eq(localbrakesound.sound_begin, "local-start")
    assert_eq(localbrakesound.sound_main, "local-mid")
    assert_eq(localbrakesound.sound_end, "local-end")

    var localbrakesound2:MmdSoundSourceDefinition = _find(definitions, "localbrakesound2")
    assert_not_null(localbrakesound2)
    assert_eq(localbrakesound2.sound_begin, "local2-start")
    assert_eq(localbrakesound2.sound_main, "local2-mid")


func test_parsing_stays_bounded_to_sounds_section_and_does_not_leak_internaldata():
    # "brakesound:" appears ONLY under internaldata: in the fixture, never inside sounds:/
    # endsounds - confirmed real ambiguity (the same label can appear in both sections with
    # different shapes), must not be picked up here.
    var definitions:Array[MmdSoundSourceDefinition] = _parse()
    assert_null(_find(definitions, "brakesound"))
    assert_null(_find(definitions, "mainctrl"))
