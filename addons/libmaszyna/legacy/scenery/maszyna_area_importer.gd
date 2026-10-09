@tool
extends RefCounted

## `area <parent> <sections...> endarea`


func import(p:MaszynaParser, context: MaszynaImporterContext):
    var area:MaszynaIsolatedData = MaszynaIsolatedData.new()
    area.name = p.next_token().to_lower()
    for token:String in p.get_tokens_until("endarea"):
        if not token.to_lower() == "endarea":
            area.children.append(token.to_lower())
    context.isolated_sections.append(area)
    return []
