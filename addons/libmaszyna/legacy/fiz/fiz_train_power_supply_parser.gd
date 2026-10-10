@tool
extends RefCounted
class_name FizTrainPowerSupplyParser

## RailVehiclePowerSupply - the battery and the converter - has no FIZ section of its own: Light:
## gives the battery's nominal voltage (LMaxVoltage, LoadFIZ_Light Mover.cpp:11035) and Cntrl. how
## the battery and the converter start (BatteryStart=/ConverterStart=/ConverterStartDelay=,
## LoadFIZ_Cntrl Mover.cpp:10707). A vehicle with neither - a freight wagon - has no low voltage.

const _CNTRL_KEYS: Array[String] = ["BatteryStart", "ConverterStart", "ConverterStartDelay"]


## The vehicle's power supply, or null when its FIZ describes none
static func create_node(context: FizImportContext) -> RailVehiclePowerSupply:
    var kv: Dictionary = context.cntrl_kv
    if not context.battery_voltage > 0.0 and not _CNTRL_KEYS.any(func(key: String) -> bool: return kv.has(key)):
        return null
    var node := MoverRailVehiclePowerSupply.new()
    node.battery_voltage = context.battery_voltage
    if kv.has("BatteryStart"):
        node.cntrl_battery_start_mode = FizTrainControllerParser.parse_start_mode(
                FizLineUtil.get_string(kv, "BatteryStart"), RailVehicleController.START_MODE_MANUAL)
    if kv.has("ConverterStart"):
        node.cntrl_converter_start_mode = FizTrainControllerParser.parse_start_mode(
                FizLineUtil.get_string(kv, "ConverterStart"), RailVehicleController.START_MODE_MANUAL)
    if kv.has("ConverterStartDelay"):
        node.cntrl_converter_start_delay = FizLineUtil.get_float(kv, "ConverterStartDelay")
    return node
