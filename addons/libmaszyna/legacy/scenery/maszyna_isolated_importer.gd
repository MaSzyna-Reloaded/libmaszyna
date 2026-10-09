@tool
extends RefCounted

## `isolated <name> <tracks...> endisolated`


func import(p:MaszynaParser, context: MaszynaImporterContext):
    var isolated:MaszynaIsolatedData = MaszynaIsolatedData.new()
    isolated.name = p.next_token().to_lower()
    for token:String in p.get_tokens_until("endisolated"):
        if not token.to_lower() == "endisolated":
            isolated.tracks.append(token.to_lower())
    context.isolated_sections.append(isolated)
    return []
