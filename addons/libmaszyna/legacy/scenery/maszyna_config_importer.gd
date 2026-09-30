@tool
extends RefCounted

## Original engine's "config" section redefines any Global.ConfigParse() entry (Globals.cpp); only
## the entries a scenery sets for its own environment are kept: "movelight" (Globals.cpp:333) and
## "scenario.weather.temperature" (Globals.cpp:386), each followed by a single value.
func import(p: MaszynaParser, _context: MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens_until("endconfig")
    var node := MaszynaConfigNode.new()
    node.name = "Config"
    for index:int in range(tokens.size() - 1):
        var key:String = String(tokens[index]).to_lower()
        if key in MaszynaConfigNode.KEYS:
            node.values[key] = String(tokens[index + 1])
    if not node.values:
        node.free()
        return []
    return [node]
