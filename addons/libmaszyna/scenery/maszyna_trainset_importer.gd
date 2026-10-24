@tool
extends RefCounted

## "trainset: name track offset velocity" (deserialize_trainset(), simulationstateserializer.cpp)
## opens a trainset: every "node ... dynamic ... enddynamic" until the matching "endtrainset:" is a
## vehicle of it, which the trainset stands on this track from this offset.
func import(p:MaszynaParser, context: MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens(4)
    if tokens.size() < 4:
        return []

    context.trainset = TrainsetData.new()
    # the name and the track in lower case, as the original reads every token
    # (simulationstateserializer.cpp:777-782)
    context.trainset.name = tokens[0].to_lower()
    context.trainset.timetable = tokens[0].to_lower()
    context.trainset.track_name = tokens[1].to_lower()
    context.trainset.offset = float(tokens[2])
    context.trainset.velocity = float(tokens[3])
    context.trainsets.append(context.trainset)
    return []
