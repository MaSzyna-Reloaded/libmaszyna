@tool
extends RefCounted
class_name FizTrainAIHintsParser

## AI: section parser -> RailVehicleAIHints. Registered directly in FizVehicleBuilder's
## section table.
##
## Keys confirmed against a real vehicle line: `AI: Pantstate=1 LocalBrakeAccFactor=0.98
## IdlePantUp=No`.


func create_node() -> RailVehicleAIHints:
    return MoverRailVehicleAIHints.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleAIHints", node)

    if kv.has("Pantstate"):
        node.pantograph_state = FizLineUtil.get_int(kv, "Pantstate", RailVehicleAIHints.PANTOGRAPH_STATE_AUTOMATIC)
    if kv.has("LocalBrakeAccFactor"):
        node.local_brake_acceleration_factor = FizLineUtil.get_float(kv, "LocalBrakeAccFactor")
    if kv.has("IdlePantUp"):
        node.raise_pantographs_when_idle = FizLineUtil.get_bool(kv, "IdlePantUp")
