@tool
extends MaszynaLegacyIncludeNode
class_name MaszynaLegacySceneryNode

## A scenery of the original: its .scn, whose time, config and atmo sections declare its scenario's
## environment.


## A later section overrides an earlier one, like the original parsing the file top to bottom
func _read_environment_declarations() -> void:
    for node:Node in find_children("", "MaszynaTimeNode", true, false):
        start_time = (node as MaszynaTimeNode).start_time
    for node:Node in find_children("", "MaszynaConfigNode", true, false):
        var values:Dictionary[String, String] = (node as MaszynaConfigNode).values
        if values.has(MaszynaConfigNode.KEY_DAY_OF_YEAR):
            day_of_year_defined = true
            day_of_year = values[MaszynaConfigNode.KEY_DAY_OF_YEAR].to_int()
        if values.has(MaszynaConfigNode.KEY_TEMPERATURE):
            temperature_defined = true
            temperature = clampf(
                values[MaszynaConfigNode.KEY_TEMPERATURE].to_float(), TEMPERATURE_MIN, TEMPERATURE_MAX)
    for node:Node in find_children("", "MaszynaAtmoNode", true, false):
        var atmo:MaszynaAtmoNode = node as MaszynaAtmoNode
        fog_end_defined = true
        fog_end = clampf(
            randf_range(
                minf(atmo.fog_range_start, atmo.fog_range_end), maxf(atmo.fog_range_start, atmo.fog_range_end)),
            FOG_END_MIN, FOG_END_MAX)
        if atmo.overcast_defined:
            overcast_defined = true
            # negative overcast means a random value in range 0-abs(specified range)
            # (simulationstateserializer.cpp:226)
            overcast = (
                atmo.overcast if atmo.overcast >= 0.0
                else randf_range(0.0, minf(absf(atmo.overcast), OVERCAST_MAX)))
