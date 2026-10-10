@tool
extends RefCounted
class_name FizTrainWipersParser

## WiperList: section parser -> RailVehicleWipers. Registered directly in
## FizVehicleBuilder's section table. Rows as TMoverParameters::readWiperList()
## (Mover.cpp:9429) reads them: byteSum -> wiper_mask, WiperSpeed -> transit_time,
## interval -> period, outBackDelay -> return_delay.
##
## Size= bounds the list (WiperListSize, Train.cpp:2643): real data ends it with a foreign marker
## ("endL" instead of "endwl", e186_v2/eu47.fiz), so rows of whatever follows could get in.

var _rows: Array[RailVehicleWiperListItem] = []
var _size: int = 0


func create_node() -> RailVehicleWipers:
    return MoverRailVehicleWipers.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleWipers", node)

    if kv.has("Angle"):
        node.angle = FizLineUtil.get_float(kv, "Angle")
    if kv.has("Default"):
        node.default_position = FizLineUtil.get_int(kv, "Default")
    _rows = []
    _size = FizLineUtil.get_int(kv, "Size") if kv.has("Size") else 0


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    var tokens: Array = p.get_tokens(4)
    if tokens.size() < 4 or (_size > 0 and _rows.size() >= _size):
        return
    var item := RailVehicleWiperListItem.new()
    item.wiper_mask = int(tokens[0])
    item.transit_time = float(tokens[1])
    item.period = float(tokens[2])
    item.return_delay = float(tokens[3])
    _rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node: VehicleComponent = context.get_part("RailVehicleWipers")
    if node == null:
        _rows = []
        return
    if _rows:
        (node as RailVehicleWipers).positions = _rows
    _rows = []
