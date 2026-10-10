@tool
extends RefCounted


func import(p:MaszynaParser, context: MaszynaImporterContext) -> MaszynaSoundData:
    var sound:MaszynaSoundData = MaszynaSoundData.new()
    var x:float = float(p.next_token())
    var y:float = float(p.next_token())
    var z:float = float(p.next_token())
    sound.position = Vector3(x, y, z)
    # the streams are looked up as <name>.ogg (AudioStreamManager.get_stream())
    sound.file = p.next_token().get_basename()
    p.get_tokens_until("endsound")
    return sound
