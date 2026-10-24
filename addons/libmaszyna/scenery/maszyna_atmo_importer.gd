@tool
extends RefCounted

## Original engine's "atmo" section (deserialize_atmo(), simulationstateserializer.cpp:198):
## three legacy sky colour values, the fog range, three legacy fog colour values (read whenever
## Global.fFogEnd is set, which it is by default) and an optional overcast.
const FOG_RANGE_INDEX:int = 3
const OVERCAST_INDEX:int = 8

func import(p: MaszynaParser, _context: MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens_until("endatmo")
    if tokens.size() < FOG_RANGE_INDEX + 2:
        return []
    var node := MaszynaAtmoNode.new()
    node.name = "Atmo"
    node.fog_range_start = String(tokens[FOG_RANGE_INDEX]).to_float()
    node.fog_range_end = String(tokens[FOG_RANGE_INDEX + 1]).to_float()
    if tokens.size() > OVERCAST_INDEX:
        node.overcast_defined = true
        node.overcast = String(tokens[OVERCAST_INDEX]).to_float()
    return [node]
