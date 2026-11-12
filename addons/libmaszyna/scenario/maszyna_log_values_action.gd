@tool
extends ScenarioEventAction
class_name MaszynaLogValuesAction

## The original's `logvalues` event (logvalues_event, Event.cpp:1180-1201): writes its memory cells
## to the scenario's log, each as TMemCell::LogValues() does (MemCell.cpp:217-225) - every cell of
## the scenery when it names none.

## The memory cells written
var memories:Array[RID] = []

var _log:GameLogger = GameLog.get_logger("scenario")


func _run(_event:RID, _activator:RID) -> void:
    for memory:RID in memories:
        _log.info("Memcell %s: [%s] [%.2f] [%.2f]" % [
            MaszynaScenarioEventServer.memory_get_name(memory),
            MaszynaScenarioEventServer.memory_get_text(memory),
            MaszynaScenarioEventServer.memory_get_value1(memory),
            MaszynaScenarioEventServer.memory_get_value2(memory),
        ])
