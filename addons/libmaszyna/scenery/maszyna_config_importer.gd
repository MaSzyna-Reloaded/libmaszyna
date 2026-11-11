@tool
extends RefCounted

## Original engine's "config" section redefines any Global.ConfigParse() entry (Globals.cpp); only
## the entries a scenery sets for its own environment are kept: "movelight" (Globals.cpp:333) and
## "scenario.weather.temperature" (Globals.cpp:386), each followed by a single value - and
## "joinduplicatedevents" (Globals.cpp:668-672), which holds for the events read after it
## (MaszynaImporterContext.join_duplicated_events), and "hiddenevents" (Globals.cpp:674-677), the
## last of which holds for the scenery (MaszynaImporterContext.hidden_events).

const KEY_JOIN_DUPLICATED_EVENTS:String = "joinduplicatedevents"
const KEY_HIDDEN_EVENTS:String = "hiddenevents"


func import(p: MaszynaParser, context: MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens_until("endconfig")
    var node := MaszynaConfigNode.new()
    node.name = "Config"
    for index:int in range(tokens.size() - 1):
        var key:String = String(tokens[index]).to_lower()
        if key == KEY_JOIN_DUPLICATED_EVENTS:
            context.set_join_duplicated_events(MaszynaParser.as_bool(String(tokens[index + 1])))
        elif key == KEY_HIDDEN_EVENTS:
            context.set_hidden_events(int(String(tokens[index + 1])))
        elif key in MaszynaConfigNode.KEYS:
            node.values[key] = String(tokens[index + 1])
    if not node.values:
        node.free()
        return []
    return [node]
