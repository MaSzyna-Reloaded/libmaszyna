extends Node

## The lines of what the sounds heard say, on the simulation's clock - the original's
## ui::Transcripts (uitranscripts.cpp). A line shows from its show time until its hide time;
## lines_changed says when the lines shown are different.

signal lines_changed

## The simulation's own time [s], advanced with it: a line is hidden sooner at x10, never while paused
var _time:float = 0.0
var _texts:PackedStringArray = []
var _shows:PackedFloat64Array = []
var _hides:PackedFloat64Array = []
## The next time a line shows or hides - nothing to do on the clock before it (fRefreshTime)
var _next_change:float = INF


func _ready() -> void:
    SimulationServer.simulation_advanced.connect(_on_simulation_advanced)


func _exit_tree() -> void:
    SimulationServer.simulation_advanced.disconnect(_on_simulation_advanced)


## A line shown `show` [s] from now until `hide` [s] from now; one shown and hidden at once is
## never shown (TTranscripts::AddLine, uitranscripts.cpp:14-33)
func add_line(text:String, show:float, hide:float) -> void:
    if show == hide:
        return
    _texts.append(text)
    _shows.append(_time + show)
    _hides.append(_time + hide)
    if show > 0.0:
        _next_change = minf(_next_change, _time + show)
        return
    _next_change = minf(_next_change, _time + hide)
    lines_changed.emit()


## Every line of the transcript, timed from now
func add(transcript:Transcript) -> void:
    for index:int in transcript.texts.size():
        add_line(transcript.texts[index], transcript.shows[index], transcript.hides[index])


## The lines to show now, oldest first
func get_lines() -> PackedStringArray:
    var lines:PackedStringArray = []
    for index:int in _texts.size():
        if _shows[index] <= _time and _time < _hides[index]:
            lines.append(_texts[index])
    return lines


## Expired lines go, and the next time anything shows or hides is found again
## (TTranscripts::Update(), uitranscripts.cpp:65-90)
func _on_simulation_advanced(seconds:float) -> void:
    _time += seconds
    if _time < _next_change:
        return
    _next_change = INF
    for index:int in range(_texts.size() - 1, -1, -1):
        if _hides[index] <= _time:
            _texts.remove_at(index)
            _shows.remove_at(index)
            _hides.remove_at(index)
            continue
        _next_change = minf(_next_change, _hides[index] if _shows[index] <= _time else _shows[index])
    lines_changed.emit()
