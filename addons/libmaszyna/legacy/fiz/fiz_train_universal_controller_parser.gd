@tool
extends RefCounted
class_name FizTrainUniversalControllerParser

## UCList: section parser -> RailVehicleUniversalController. Registered directly in
## FizVehicleBuilder's section table.
##
## Row format confirmed exactly against readUCList (Mover.cpp:8427-8442): 10 tokens per row -
## a leading index (discarded, matches every other List's convention) then 9 data columns in
## this exact order: mode, MinCtrlVal, MaxCtrlVal, SetCtrlVal, SpeedUp, SpeedDown,
## ReturnPosition, NextPosFastInc, PrevPosFastDec - mapping 1:1 onto
## RailVehicleUniversalControllerListItem's pneumatic_brake_position/min_percentage/max_percentage/
## target_value/increase_speed/decrease_speed/bounce_back_position/nearest_stable_up/
## nearest_stable_down.

var _rows: Array[RailVehicleUniversalControllerListItem] = []


func create_node() -> RailVehicleUniversalController:
    return MoverRailVehicleUniversalController.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleUniversalController", node)

    if kv.has("IntegratedBrakePN"):
        node.integrated_brake_pn = FizLineUtil.get_bool(kv, "IntegratedBrakePN")
    if kv.has("IntegratedBrake"):
        node.integrated_brake = FizLineUtil.get_bool(kv, "IntegratedBrake")
    if kv.has("IntegratedLocBrake"):
        node.integrated_local_brake = FizLineUtil.get_bool(kv, "IntegratedLocBrake")
    _rows = []


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    var tokens: Array = p.get_tokens(10)
    if tokens.size() < 10:
        return
    var item := RailVehicleUniversalControllerListItem.new()
    item.pneumatic_brake_position = int(tokens[1])
    item.min_percentage = float(tokens[2])
    item.max_percentage = float(tokens[3])
    item.target_value = float(tokens[4])
    item.increase_speed = float(tokens[5])
    item.decrease_speed = float(tokens[6])
    item.bounce_back_position = int(tokens[7])
    item.nearest_stable_up = int(tokens[8])
    item.nearest_stable_down = int(tokens[9])
    _rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node: VehicleComponent = context.get_part("RailVehicleUniversalController")
    if node == null:
        _rows = []
        return
    if _rows:
        (node as RailVehicleUniversalController).positions = _rows
    _rows = []
