@tool
extends RefCounted

## `lua <file>` (simulationstateserializer.cpp:346-356): a script of the scenery directory, which
## ScenarioScriptServer runs once the scenery is built


func import(p:MaszynaParser, context:MaszynaImporterContext) -> Array:
    var filename:String = p.next_token()
    if not filename:
        push_error("lua without a filename at offset %d" % p.get_position())
        return []
    context.scripts.append(SceneryInstancer.include_importer.resolve_filename(filename))
    return []
