@tool
extends RefCounted

## `event <name> <type> <delay> <targets> <parameters> [condition ...] [randomdelay <s>]
## [departuredelay <s>] endevent` (Event.cpp:289-333, 442-448)

## Events named "none_..." are ignored by the original (Event.cpp:301)
const IGNORED_PREFIX:String = "none_"
## Targets are separated by either and read in lower case; "none" is no target (Event.cpp:327-329)
const TARGET_SEPARATORS:Array[String] = ["|", ","]
const NO_TARGET:String = "none"


func import(p:MaszynaParser, context: MaszynaImporterContext):
    var name:String = p.next_token().to_lower()
    if name.begins_with(IGNORED_PREFIX):
        while not p.eof_reached():
            if p.next_token().to_lower() == "endevent":
                break
        return []

    var event:MaszynaEventData = MaszynaEventData.new()
    event.name = name
    event.origin = context.origin
    event.type = p.next_token().to_lower()
    event.delay = float(p.next_token())
    var targets:PackedStringArray = [p.next_token().to_lower()]
    for separator:String in TARGET_SEPARATORS:
        var split:PackedStringArray = []
        for part:String in targets:
            split.append_array(part.split(separator, false))
        targets = split
    var none_index:int = targets.find(NO_TARGET)
    while not none_index == -1:
        targets.remove_at(none_index)
        none_index = targets.find(NO_TARGET)
    event.targets = targets

    var in_condition:bool = false
    while not p.eof_reached():
        var token:String = p.next_token()
        var keyword:String = token.to_lower()
        if keyword == "endevent":
            break
        if keyword == "randomdelay":
            event.random_delay = float(p.next_token())
            continue
        if keyword == "departuredelay":
            event.departure_delay = float(p.next_token())
            continue
        if keyword == "condition":
            in_condition = true
            continue
        if in_condition:
            event.condition.append(token)
        else:
            event.parameters.append(token)
    context.events.append(event)
    return []
