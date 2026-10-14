@tool
extends RefCounted
class_name FizTrainLoadParser

## Load: section parser -> RailVehicleLoad. Registered directly in FizVehicleBuilder's
## section table. Its keys as LoadFIZ_Load reads them (Mover.cpp:10309-10340).


func create_node() -> RailVehicleLoad:
    return MoverRailVehicleLoad.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleLoad", node)

    if kv.has("MaxLoad"):
        node.max_load = FizLineUtil.get_float(kv, "MaxLoad")
    if kv.has("LoadQ"):
        match FizLineUtil.get_string(kv, "LoadQ").to_lower():
            "pieces": node.load_unit = RailVehicleLoad.LOAD_UNIT_PIECES
            "tonns", "tons": node.load_unit = RailVehicleLoad.LOAD_UNIT_TONS
    if kv.has("LoadAccepted"):
        var loads: PackedStringArray = FizLineUtil.get_string(kv, "LoadAccepted").split(",")
        var accepted: Array[String] = []
        for load: String in loads:
            accepted.append(load.strip_edges())
        node.accepted_loads = accepted
    if kv.has("LoadSpeed"):
        node.load_speed = FizLineUtil.get_float(kv, "LoadSpeed")
    if kv.has("UnLoadSpeed"):
        node.unload_speed = FizLineUtil.get_float(kv, "UnLoadSpeed")
    if kv.has("OverLoadFactor"):
        node.overload_factor = FizLineUtil.get_float(kv, "OverLoadFactor")
    # one offset per accepted load, the last one for the rest (Mover.cpp:10319-10331)
    if kv.has("LoadMinOffset"):
        var offsets: Array[float] = []
        for offset: String in FizLineUtil.get_string(kv, "LoadMinOffset").split(","):
            offsets.append(offset.strip_edges().to_float())
        node.minimum_load_offsets = offsets
