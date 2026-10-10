@tool
extends RefCounted

## "trainset: name track offset velocity" (deserialize_trainset(), simulationstateserializer.cpp)
## opens a trainset: every "node ... dynamic ... enddynamic" until the matching "endtrainset:" is a
## vehicle of it, which the trainset stands on this track from this offset.
func import(p:MaszynaParser, context: MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens(4)
    if tokens.size() < 4:
        return []

    context.trainset = MaszynaTrainsetData.new()
    context.trainset.name = tokens[0]
    context.trainset.timetable = tokens[0]
    context.trainset.track_name = tokens[1]
    context.trainset.offset = float(tokens[2])
    context.trainset.velocity = float(tokens[3])
    context.trainsets.append(context.trainset)
    return []
