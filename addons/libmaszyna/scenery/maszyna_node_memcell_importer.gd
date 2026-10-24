@tool
extends RefCounted

const NO_TRACK:String = "none"


func import(p:MaszynaParser, context: MaszynaImporterContext) -> MaszynaMemcellData:
    var memcell:MaszynaMemcellData = MaszynaMemcellData.new()
    var x:float = float(p.next_token())
    var y:float = float(p.next_token())
    var z:float = float(p.next_token())
    memcell.position = Vector3(x, y, z)
    memcell.text = p.next_token()
    memcell.value1 = float(p.next_token())
    memcell.value2 = float(p.next_token())
    var track:String = p.next_token().to_lower()
    memcell.track = "" if track == NO_TRACK else track
    p.get_tokens_until("endmemcell")
    return memcell
