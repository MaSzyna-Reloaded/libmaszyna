@tool
extends RefCounted
class_name FizTrainLightingParser

## Light: and Headlights: sections -> RailVehicleLighting. LoadFIZ_Light: Mover.cpp:11035,
## LoadFIZ_Headlights: Mover.cpp:10328, LoadFIZ_PowerParamsDecode: Mover.cpp:11547.
## LightsList: header (Size/Wrap/Default) and rows of two light bitmasks, the occupied cab's end
## and the other one (LoadFIZ_LightsList, readLightsList, Mover.cpp:8558).

## Original engine: enum light (MOVER.h:189) - the bits of a LightsList: row
const LIGHT_HEADLIGHT_LEFT: int = 1 << 0
const LIGHT_REDMARKER_LEFT: int = 1 << 1
const LIGHT_HEADLIGHT_UPPER: int = 1 << 2
const LIGHT_HEADLIGHT_RIGHT: int = 1 << 4
const LIGHT_REDMARKER_RIGHT: int = 1 << 5
const LIGHT_REAR_END_SIGNALS: int = 1 << 6
const LIGHT_AUXILIARY_LEFT: int = 1 << 7
const LIGHT_AUXILIARY_RIGHT: int = 1 << 8

## Rows of the LightsList: table being read, and its declared Size= (LightsPosNo)
var _lights_rows: Array[RailVehicleLightListItem] = []
var _lights_size: int = 0


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    if prefix == "Headlights:":
        _parse_headlights(FizLineUtil.read_key_values(p), context)
        return
    if prefix == "LightsList:":
        _parse_lights_list(p, context)
        return
    _parse_light(FizLineUtil.read_key_values(p), context)


func _get_node(context: FizImportContext) -> RailVehicleLighting:
    var node: RailVehicleLighting = context.get_part("RailVehicleLighting")
    if node == null:
        node = MoverRailVehicleLighting.new()
        context.add_part("RailVehicleLighting", node)
    return node


func _parse_light(kv: Dictionary, context: FizImportContext) -> void:
    var node := _get_node(context)

    if kv.has("Light"):
        node.light_source = FizTrainControllerParser.parse_power_source(FizLineUtil.get_string(kv, "Light"))
    if kv.has("LGeneratorEngine"):
        node.source_generator_engine = FizTrainEngineCommon.parse_engine_type(FizLineUtil.get_string(kv, "LGeneratorEngine"))
    if kv.has("AlterLight"):
        node.light_alternative_source = FizTrainControllerParser.parse_power_source(FizLineUtil.get_string(kv, "AlterLight"))
    if kv.has("AlterLMaxVoltage"):
        node.light_alternative_max_voltage = FizLineUtil.get_float(kv, "AlterLMaxVoltage")

    # LMaxVoltage is the battery's nominal voltage, RailVehiclePowerSupply's - not a
    # RailVehicleLighting property (FizTrainPowerSupplyParser)
    if kv.has("LMaxVoltage"):
        context.battery_voltage = FizLineUtil.get_float(kv, "LMaxVoltage")


func _parse_headlights(kv: Dictionary, context: FizImportContext) -> void:
    var node := _get_node(context)
    if kv.has("DimmedMultiplier"):
        node.head_light_dimmed_multiplier = FizLineUtil.get_float(kv, "DimmedMultiplier")
    if kv.has("NormalMultiplier"):
        node.head_light_normal_multiplier = FizLineUtil.get_float(kv, "NormalMultiplier")
    if kv.has("HighbeamDimmedMultiplier"):
        node.head_light_high_beam_dimmed_multiplier = FizLineUtil.get_float(kv, "HighbeamDimmedMultiplier")
    if kv.has("HighBeamMultiplier"):
        node.head_light_high_beam_normal_multiplier = FizLineUtil.get_float(kv, "HighBeamMultiplier")
    if kv.has("LampRed") or kv.has("LampGreen") or kv.has("LampBlue"):
        node.head_light_color = Color(
                FizLineUtil.get_float(kv, "LampRed", 255.0) / 255.0,
                FizLineUtil.get_float(kv, "LampGreen", 255.0) / 255.0,
                FizLineUtil.get_float(kv, "LampBlue", 255.0) / 255.0)


## LoadFIZ_LightsList (Mover.cpp:11531): Size= is the number of presets, Default= the one the
## selector starts at (1-based, like LightsPos).
func _parse_lights_list(p: MaszynaParser, context: FizImportContext) -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := _get_node(context)
    if kv.has("Wrap"):
        node.lights_wrap_selector = FizLineUtil.get_bool(kv, "Wrap")
    if kv.has("Default"):
        node.lights_default_selector_position = FizLineUtil.get_int(kv, "Default")
    _lights_size = FizLineUtil.get_int(kv, "Size") if kv.has("Size") else 0
    _lights_rows = []


## readLightsList (Mover.cpp:8558): the light bits of one preset, cabin A's end then cabin B's
func parse_row(p: MaszynaParser, _context: FizImportContext) -> void:
    var tokens: Array = p.get_tokens(2)
    if tokens.size() < 2 or _lights_rows.size() >= _lights_size:
        return
    var cabin_a: int = int(tokens[0])
    var cabin_b: int = int(tokens[1])
    var item := RailVehicleLightListItem.new()
    item.cabin_a_head_light = bool(cabin_a & LIGHT_HEADLIGHT_UPPER)
    item.cabin_a_left_white_signal = bool(cabin_a & LIGHT_HEADLIGHT_LEFT)
    item.cabin_a_left_red_signal = bool(cabin_a & LIGHT_REDMARKER_LEFT)
    item.cabin_a_right_white_signal = bool(cabin_a & LIGHT_HEADLIGHT_RIGHT)
    item.cabin_a_right_red_signal = bool(cabin_a & LIGHT_REDMARKER_RIGHT)
    item.cabin_a_end_signals = bool(cabin_a & LIGHT_REAR_END_SIGNALS)
    item.cabin_a_left_auxiliary_light = bool(cabin_a & LIGHT_AUXILIARY_LEFT)
    item.cabin_a_right_auxiliary_light = bool(cabin_a & LIGHT_AUXILIARY_RIGHT)
    item.cabin_b_head_light = bool(cabin_b & LIGHT_HEADLIGHT_UPPER)
    item.cabin_b_left_white_signal = bool(cabin_b & LIGHT_HEADLIGHT_LEFT)
    item.cabin_b_left_red_signal = bool(cabin_b & LIGHT_REDMARKER_LEFT)
    item.cabin_b_right_white_signal = bool(cabin_b & LIGHT_HEADLIGHT_RIGHT)
    item.cabin_b_right_red_signal = bool(cabin_b & LIGHT_REDMARKER_RIGHT)
    item.cabin_b_end_signals = bool(cabin_b & LIGHT_REAR_END_SIGNALS)
    item.cabin_b_left_auxiliary_light = bool(cabin_b & LIGHT_AUXILIARY_LEFT)
    item.cabin_b_right_auxiliary_light = bool(cabin_b & LIGHT_AUXILIARY_RIGHT)
    _lights_rows.append(item)


func end_table(context: FizImportContext) -> void:
    _get_node(context).lights_list = _lights_rows
    _lights_rows = []
