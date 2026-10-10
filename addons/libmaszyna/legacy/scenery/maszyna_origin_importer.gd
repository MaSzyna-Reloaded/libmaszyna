@tool
extends RefCounted

func import(p:MaszynaParser, context: MaszynaImporterContext):
    var origin = p.get_tokens(3)
    context.push_origin(Vector3(float(origin[0]), float(origin[1]), float(origin[2])))
    return []
