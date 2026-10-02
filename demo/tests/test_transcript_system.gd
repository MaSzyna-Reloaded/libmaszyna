extends GutTest

## TranscriptSystem: a line shows from its show time to its hide time on the simulation's clock
## (TTranscripts, uitranscripts.cpp)


func after_each() -> void:
    # the lines left by a test expire before the next one
    SimulationServer.simulation_advanced.emit(1000.0)


func test_a_line_shows_at_its_time_and_hides_at_its_own() -> void:
    TranscriptSystem.add_line("Monter radiowy.", 1.0, 3.0)
    assert_eq(TranscriptSystem.get_lines(), PackedStringArray())
    SimulationServer.simulation_advanced.emit(1.5)
    assert_eq(TranscriptSystem.get_lines(), PackedStringArray(["Monter radiowy."]))
    SimulationServer.simulation_advanced.emit(2.0)
    assert_eq(TranscriptSystem.get_lines(), PackedStringArray())


func test_the_change_of_the_lines_shown_is_announced() -> void:
    watch_signals(TranscriptSystem)
    TranscriptSystem.add_line("!! RADIO-STOP !!", 0.0, 10.0)
    assert_signal_emit_count(TranscriptSystem, "lines_changed", 1)
    SimulationServer.simulation_advanced.emit(5.0)
    assert_signal_emit_count(TranscriptSystem, "lines_changed", 1, "nothing changed yet")
    SimulationServer.simulation_advanced.emit(5.0)
    assert_signal_emit_count(TranscriptSystem, "lines_changed", 2)


func test_a_line_shown_and_hidden_at_once_is_never_shown() -> void:
    TranscriptSystem.add_line("Komentarz", 2.0, 2.0)
    SimulationServer.simulation_advanced.emit(2.0)
    assert_eq(TranscriptSystem.get_lines(), PackedStringArray())


func test_a_transcript_adds_every_line() -> void:
    var transcript:Transcript = Transcript.new()
    transcript.add_line("Jest na odbiorze.", 0.0, 1.0)
    transcript.add_line("Dobrze.", 0.0, 1.0)
    TranscriptSystem.add(transcript)
    assert_eq(TranscriptSystem.get_lines(), PackedStringArray(["Jest na odbiorze.", "Dobrze."]))
