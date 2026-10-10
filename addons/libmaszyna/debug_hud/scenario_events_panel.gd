extends VBoxContainer

## Live view of ScenarioEventServer, like the original's event queue panel
## (driveruipanels.cpp:1328-1362): the simulation time, the events waiting in the queue with the
## seconds left and who queued them, and the last events that ran.

## Rows of the queue and of the log shown at most
const MAX_QUEUE_ROWS:int = 30
const MAX_LOG_ROWS:int = 20

## Queued events, by RID, and their activators
var _queued:Dictionary[RID, RID] = {}
var _log:PackedStringArray = []


func _ready() -> void:
    ScenarioEventServer.event_queued.connect(_on_event_queued)
    ScenarioEventServer.event_launched.connect(_on_event_launched)
    ScenarioEventServer.event_dequeued.connect(_on_event_dequeued)
    _refresh()


func _exit_tree() -> void:
    ScenarioEventServer.event_queued.disconnect(_on_event_queued)
    ScenarioEventServer.event_launched.disconnect(_on_event_launched)
    ScenarioEventServer.event_dequeued.disconnect(_on_event_dequeued)


func _on_event_queued(event:RID, activator:RID) -> void:
    _queued[event] = activator


## Freed while queued (its scenery unloaded): it will never launch
func _on_event_dequeued(event:RID) -> void:
    _queued.erase(event)


func _on_event_launched(event:RID, activator:RID) -> void:
    _queued.erase(event)
    _log.insert(0, "%.1f  %s%s" % [
        SimulationServer.simulation_get_time(), ScenarioEventServer.event_get_name(event), _activator_suffix(activator)
    ])
    if _log.size() > MAX_LOG_ROWS:
        _log.resize(MAX_LOG_ROWS)


## The countdown, on the refresh timer (a [connection] in the scene)
func _refresh() -> void:
    var now:float = SimulationServer.simulation_get_time()
    %Time.text = tr("Simulation time: %.1f s") % now
    var rows:Array[Array] = []
    for event:RID in _queued:
        var run_time:float = ScenarioEventServer.event_get_run_time(event)
        if run_time < 0.0:
            continue
        rows.append([run_time, event])
    rows.sort_custom(func(a:Array, b:Array) -> bool: return a[0] < b[0])
    var lines:PackedStringArray = []
    for row:Array in rows.slice(0, MAX_QUEUE_ROWS):
        lines.append("%6.1f s  %s%s" % [
            row[0] - now, ScenarioEventServer.event_get_name(row[1]), _activator_suffix(_queued[row[1]])
        ])
    %QueueCaption.text = tr("Queue (%d)") % rows.size()
    %Queue.text = "\n".join(lines)
    %Log.text = "\n".join(_log)


func _activator_suffix(activator:RID) -> String:
    return " [%s]" % VehicleServer.vehicle_get_name(activator) if activator.is_valid() else ""
