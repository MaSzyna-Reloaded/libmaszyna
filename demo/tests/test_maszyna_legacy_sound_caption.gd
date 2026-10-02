extends GutTest

## MaszynaLegacySoundCaption.parse(): the original's caption format (TTranscripts::Add(),
## uitranscripts.cpp:37-60) made a Transcript


func test_a_timed_line_is_shown_and_hidden_in_the_originals_units() -> void:
    var transcript:Transcript = MaszynaLegacySoundCaption.parse("[30][60]83202 odjazd.")
    assert_eq(transcript.texts, PackedStringArray(["83202 odjazd."]))
    assert_almost_eq(transcript.shows[0], 3.0, 0.0001)
    assert_almost_eq(transcript.hides[0], 7.2, 0.0001)


func test_every_line_of_a_caption_with_crlf_is_read() -> void:
    var transcript:Transcript = MaszynaLegacySoundCaption.parse(
            "[08][31]Monter radiowy.\r\n[42][48]Słucham?\r\n")
    assert_eq(transcript.texts, PackedStringArray(["Monter radiowy.", "Słucham?"]))
    assert_almost_eq(transcript.shows[1], 4.2, 0.0001)


func test_a_line_without_times_is_shown_from_the_start_as_long_as_it_is_long() -> void:
    var transcript:Transcript = MaszynaLegacySoundCaption.parse("Pociąg odjedzie")
    assert_eq(transcript.texts, PackedStringArray(["Pociąg odjedzie"]))
    assert_eq(transcript.shows[0], 0.0)
    assert_almost_eq(transcript.hides[0], "Pociąg odjedzie".length() * 0.12, 0.0001)


func test_the_legacy_new_line_mark_is_a_space() -> void:
    var transcript:Transcript = MaszynaLegacySoundCaption.parse("[0][10]Tor|pierwszy")
    assert_eq(transcript.texts[0], "Tor pierwszy")


func test_the_caption_is_read_beside_its_sound_wherever_it_is() -> void:
    # a scenery's radio message has its caption beside it, not in the sounds (audio.cpp:84-94)
    var directory:String = "user://sound_caption_test"
    DirAccess.make_dir_recursive_absolute(directory)
    var caption:String = "%s/ex6435radio-%s.txt" % [directory, MaszynaTranslationServer.language]
    var file:FileAccess = FileAccess.open(caption, FileAccess.WRITE)
    file.store_string("[0][30]EX6435 odjazd.")
    file.close()

    var transcript:Transcript = MaszynaLegacySoundCaption.from_sound_file(directory.path_join("EX6435RADIO"))

    DirAccess.remove_absolute(caption)
    DirAccess.remove_absolute(directory)
    assert_not_null(transcript)
    assert_eq(transcript.texts, PackedStringArray(["EX6435 odjazd."]))
