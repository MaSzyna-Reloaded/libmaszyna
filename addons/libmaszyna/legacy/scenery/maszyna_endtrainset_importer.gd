@tool
extends RefCounted

## Closes whatever "trainset:" opened (deserialize_endtrainset(), simulationstateserializer.cpp).
## Its vehicles are stood on the track and coupled, and its timetable reaches its driver, when the
## scenery is built (SceneryInstancer).
func import(_p:MaszynaParser, context: MaszynaImporterContext) -> Array:
    context.trainset = null
    return []
