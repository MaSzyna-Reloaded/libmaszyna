@tool
extends RefCounted
class_name FizTrainTurboParser

## TurboPos: section parser - the master controller position from which the turbocharger is heard
## (LoadFIZ_TurboPos, Mover.cpp:9864-9870, 10714). Held in the context until the whole file is read
## (FizVehicleBuilder): the section may come before the engine it belongs to.


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    if kv.has("TurboPos"):
        context.turbo_position = FizLineUtil.get_int(kv, "TurboPos")
