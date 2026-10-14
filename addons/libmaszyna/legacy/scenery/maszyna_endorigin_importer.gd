@tool
extends RefCounted

func import(p:MaszynaParser, context: MaszynaImporterContext):
    context.pop_origin()
    return []
