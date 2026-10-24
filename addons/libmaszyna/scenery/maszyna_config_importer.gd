@tool
extends RefCounted

## Original engine's "config" section redefines any Global.ConfigParse() entry (Globals.cpp); only
## the entries a scenery sets for its own environment are kept: "movelight" (Globals.cpp:333) and
## "scenario.weather.temperature" (Globals.cpp:386), each followed by a single value - and
## "joinduplicatedevents" (Globals.cpp:668-672), which holds for the events read after it
## (MaszynaImporterContext.join_duplicated_events).

const KEY_JOIN_DUPLICATED_EVENTS:String = "joinduplicatedevents"
## What the original's parser reads as true (cParser::operator>>(bool &), parser.cpp:148)
const TRUE_TOKENS:Array[String] = ["true", "yes", "1"]


func import(p: MaszynaParser, context: MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens_until("endconfig")
    var node := MaszynaConfigNode.new()
    node.name = "Config"
    for index:int in range(tokens.size() - 1):
        var key:String = String(tokens[index]).to_lower()
        if key == KEY_JOIN_DUPLICATED_EVENTS:
            context.set_join_duplicated_events(String(tokens[index + 1]).to_lower() in TRUE_TOKENS)
        elif key in MaszynaConfigNode.KEYS:
            node.values[key] = String(tokens[index + 1])
    if not node.values:
        node.free()
        return []
    return [node]
