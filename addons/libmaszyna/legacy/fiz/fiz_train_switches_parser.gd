@tool
extends RefCounted
class_name FizTrainSwitchesParser

## Switches: and DimmerList: section parser -> RailVehicleSwitches. Registered directly in
## FizVehicleBuilder's section table (both prefixes share this one instance).
##
## Its keys as LoadFIZ_Switches reads them (Mover.cpp:11384-11403): the switch types ("impulse",
## any case, is impulse), the relay reset buttons' relays and the pantograph presets. `ModernDimmer=`
## and `DimmerList:` are the cab's headlight dimmer, not ported (TODO.md).
##
## DimmerList: header keys `Cycle=`/`DefaultPos=` (LoadFIZ_DimmerList, Mover.cpp:11537-11542) and rows
## high beam / dimmed / off (readDimmerList, Mover.cpp:9447-9461).

## The digits of PantographPresets= (Train.cpp:3522 reads each as `preset - '0'`)
const PANTOGRAPH_PRESETS: Dictionary[String, RailVehicleSwitches.PantographPreset] = {
    "0": RailVehicleSwitches.PANTOGRAPH_PRESET_NONE,
    "1": RailVehicleSwitches.PANTOGRAPH_PRESET_OWN_END,
    "2": RailVehicleSwitches.PANTOGRAPH_PRESET_OTHER_END,
    "3": RailVehicleSwitches.PANTOGRAPH_PRESET_BOTH,
}

var _dimmer_rows: Array[RailVehicleDimmerListItem] = []


func create_node() -> RailVehicleSwitches:
    return MoverRailVehicleSwitches.new()


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    if prefix == "DimmerList:":
        var kv: Dictionary = FizLineUtil.read_key_values(p)
        var node := _get_node(context)
        if node:
            if kv.has("Cycle"):
                node.dimmer_list_cycle = FizLineUtil.get_bool(kv, "Cycle")
            if kv.has("DefaultPos"):
                node.dimmer_list_default_position = FizLineUtil.get_int(kv, "DefaultPos")
        _dimmer_rows = []
        return

    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleSwitches", node)

    if kv.has("Pantograph"):
        node.pantograph_impulse = FizLineUtil.get_string(kv, "Pantograph").to_lower() == "impulse"
    if kv.has("Converter"):
        node.converter_impulse = FizLineUtil.get_string(kv, "Converter").to_lower() == "impulse"
    if kv.has("MotorConnectors"):
        # every value but "toggle" is an impulse button (Train.cpp:5045)
        node.motor_connectors_impulse = not FizLineUtil.get_string(kv, "MotorConnectors").to_lower() == "toggle"
    if kv.has("RelayResetButton1"):
        node.relay_reset_button_1 = FizLineUtil.get_int(kv, "RelayResetButton1")
    if kv.has("RelayResetButton2"):
        node.relay_reset_button_2 = FizLineUtil.get_int(kv, "RelayResetButton2")
    if kv.has("RelayResetButton3"):
        node.relay_reset_button_3 = FizLineUtil.get_int(kv, "RelayResetButton3")
    if kv.has("PantographPresets"):
        var tokens: PackedStringArray = FizLineUtil.get_string(kv, "PantographPresets").split("|")
        var presets := PackedInt32Array()
        for token: String in tokens:
            if PANTOGRAPH_PRESETS.has(token.strip_edges()):
                presets.append(PANTOGRAPH_PRESETS[token.strip_edges()])
        node.pantograph_presets = presets
    if kv.has("PantographPresetDefault"):
        node.pantograph_preset_default = FizLineUtil.get_int(kv, "PantographPresetDefault")
    if kv.has("ModernDimmer"):
        node.modern_dimmer = FizLineUtil.get_bool(kv, "ModernDimmer")


func _get_node(context: FizImportContext) -> RailVehicleSwitches:
    var node: VehicleComponent = context.get_part("RailVehicleSwitches")
    return node as RailVehicleSwitches


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    var tokens: Array = p.get_tokens(3)
    if tokens.size() < 3:
        return
    var item := RailVehicleDimmerListItem.new()
    item.high_beam = String(tokens[0]).to_lower() in ["1", "yes", "true"]
    item.dimmed = String(tokens[1]).to_lower() in ["1", "yes", "true"]
    item.off = String(tokens[2]).to_lower() in ["1", "yes", "true"]
    _dimmer_rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node := _get_node(context)
    if node == null:
        _dimmer_rows = []
        return
    if _dimmer_rows:
        node.dimmer_list_positions = _dimmer_rows
    _dimmer_rows = []
