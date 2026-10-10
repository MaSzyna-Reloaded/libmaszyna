@tool
extends RefCounted

func import(p:MaszynaParser, context: MaszynaImporterContext):
    var rotate = p.get_tokens(3)
    var new_rotate = Vector3(
       deg_to_rad(float(rotate[0])),
       deg_to_rad(float(rotate[1])),
       deg_to_rad(float(rotate[2])),
    )
    context.push_rotate(new_rotate)
    
    return []
