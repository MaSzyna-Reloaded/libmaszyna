@tool
extends RefCounted
class_name FizTrainPowerParser

## Power: section - RailVehicleEnginePowerSource, what the vehicle's traction is fed from
## (LoadFIZ_Power: Mover.cpp:11058). Its pantograph keys are in Cntrl. (LoadFIZ_Cntrl,
## Mover.cpp:10919-10946), which may come before or after Power: (EN57's is in an include that
## follows Engine:), so both are stashed on the context and the node is built once the whole file
## is read - like FizTrainPowerSupplyParser. A car without an engine of its own (31WE B and C) has
## one too: it carries the pantographs of its unit.

## Physical layout of the collectors when the FIZ gives none (Mover.cpp:11593)
const DEFAULT_PHYSICAL_LAYOUT: int = 3
## The collectors a physical layout can describe (Mover.cpp:11594)
const MAX_COLLECTORS: int = 2
## The main switch opens below this part of MaxVoltage, and closes only above the other, when the
## FIZ gives no MinV/InsetV (Mover.cpp:11579-11583)
const MIN_MAIN_SWITCH_VOLTAGE_RATIO: float = 0.5
const REQUIRED_MAIN_SWITCH_VOLTAGE_RATIO: float = 0.6
## Pantograph tank pressures when the FIZ gives none (Mover.cpp:11585-11589)
const DEFAULT_MIN_PANTOGRAPH_TANK_PRESSURE: float = 3.5
const DEFAULT_MAX_PANTOGRAPH_TANK_PRESSURE: float = 5.0


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    context.power_kv = FizLineUtil.read_key_values(p)


## The vehicle's engine power source, or null when its FIZ declares none (EnginePower=)
static func create_node(context: FizImportContext) -> RailVehicleEnginePowerSource:
    var kv: Dictionary = context.power_kv
    if not kv.has("EnginePower"):
        return null
    var node := MoverRailVehicleEnginePowerSource.new()
    node.source_type = FizTrainControllerParser.parse_power_source(FizLineUtil.get_string(kv, "EnginePower"))
    # a vehicle without power (an EMU's control car) has no source, whatever it declares (LoadFIZ_Power)
    var controller: RailVehicleController = context.controller as RailVehicleController
    if controller and controller.power == 0.0:
        node.source_type = RailVehicleController.POWER_SOURCE_NOT_DEFINED
    node.transducer_input_voltage = FizLineUtil.get_float(kv, "TransducerInputV")
    node.power_cable_source = FizTrainControllerParser.parse_power_type(FizLineUtil.get_string(kv, "PowerTrans"))
    node.power_cable_steam_pressure = FizLineUtil.get_float(kv, "SteamPress")

    # LoadFIZ_PowerParamsDecode, CurrentCollector (Mover.cpp:11547-11596): every field starts at zero
    var max_voltage: float = FizLineUtil.get_float(kv, "MaxVoltage")
    node.current_collector_number_of_collectors = FizLineUtil.get_int(kv, "CollectorsNo")
    node.current_collector_min_collector_lifting = FizLineUtil.get_float(kv, "MinH")
    node.current_collector_max_collector_lifting = FizLineUtil.get_float(kv, "MaxH")
    node.current_collector_sliding_width = FizLineUtil.get_float(kv, "CSW")
    node.current_collector_pantograph_type = pantograph_type(FizLineUtil.get_string(kv, "PantType"))
    node.current_collector_max_voltage = max_voltage
    node.current_collector_max_current = FizLineUtil.get_float(kv, "MaxCurrent")
    node.current_collector_overvoltage_relay = FizLineUtil.get_bool(kv, "OverVoltProt")
    node.current_collector_min_main_switch_voltage = FizLineUtil.get_float(
            kv, "MinV", MIN_MAIN_SWITCH_VOLTAGE_RATIO * max_voltage)
    node.current_collector_required_main_switch_voltage = FizLineUtil.get_float(
            kv, "InsetV", REQUIRED_MAIN_SWITCH_VOLTAGE_RATIO * max_voltage)
    node.current_collector_min_pantograph_tank_pressure = FizLineUtil.get_float(
            kv, "MinPress", DEFAULT_MIN_PANTOGRAPH_TANK_PRESSURE)
    node.current_collector_max_pantograph_tank_pressure = FizLineUtil.get_float(
            kv, "MaxPress", DEFAULT_MAX_PANTOGRAPH_TANK_PRESSURE)
    node.current_collector_fake_power = FizLineUtil.get_bool(kv, "FakePower")
    node.current_collector_physical_layout = FizLineUtil.get_int(kv, "PhysicalLayout", DEFAULT_PHYSICAL_LAYOUT)
    # a layout given in the FIZ also says how many collectors there are (Mover.cpp:11593-11594)
    if kv.has("PhysicalLayout"):
        node.current_collector_number_of_collectors = mini(node.current_collector_physical_layout, MAX_COLLECTORS)

    # The pantographs of Cntrl. (LoadFIZ_Cntrl, Mover.cpp:10919-10946): the pantograph compressor,
    # its automatic valve, the master valve and each pantograph's own. The defaults are the
    # properties' own: the master valve automatic, each pantograph's valve manual. PantAutoValve
    # defaults to true for an EZT in the original (Mover.cpp:10925) - not ported with the train type.
    var cntrl_kv: Dictionary = context.cntrl_kv
    if cntrl_kv.has("PantCompressorStart"):
        node.cntrl_pantograph_compressor_start_mode = FizTrainControllerParser.parse_start_mode(
                FizLineUtil.get_string(cntrl_kv, "PantCompressorStart"), RailVehicleController.START_MODE_MANUAL)
    if cntrl_kv.has("PantAutoValve"):
        node.cntrl_pantograph_auto_valve = FizLineUtil.get_bool(cntrl_kv, "PantAutoValve")
    if cntrl_kv.has("PantEPValveStart"):
        node.cntrl_pantographs_valve_start_mode = FizTrainControllerParser.parse_start_mode(
                FizLineUtil.get_string(cntrl_kv, "PantEPValveStart"), RailVehicleController.START_MODE_AUTOMATIC)
    if cntrl_kv.has("PantEPValveSpring"):
        node.cntrl_pantographs_valve_spring = FizLineUtil.get_bool(cntrl_kv, "PantEPValveSpring")
    if cntrl_kv.has("PantValveStart"):
        node.cntrl_pantograph_valve_start_mode = FizTrainControllerParser.parse_start_mode(
                FizLineUtil.get_string(cntrl_kv, "PantValveStart"), RailVehicleController.START_MODE_MANUAL)
    if cntrl_kv.has("PantValveSpring"):
        node.cntrl_pantograph_valve_spring = FizLineUtil.get_bool(cntrl_kv, "PantValveSpring")
    if cntrl_kv.has("PantValveSolenoid"):
        node.cntrl_pantograph_valve_solenoid = FizLineUtil.get_bool(cntrl_kv, "PantValveSolenoid")
    return node


## PantType= - AKP_4E, DSA..., EC160 or EC200, WBL85 (Mover.cpp:11620-11629); none without the key
static func pantograph_type(value: String) -> RailVehicleEnginePowerSource.PantographType:
    if value == "AKP_4E":
        return RailVehicleEnginePowerSource.PANTOGRAPH_TYPE_AKP_4E
    if value.begins_with("DSA"):
        return RailVehicleEnginePowerSource.PANTOGRAPH_TYPE_DSAX
    if value in ["EC160", "EC200"]:
        return RailVehicleEnginePowerSource.PANTOGRAPH_TYPE_EC160_200
    if value == "WBL85":
        return RailVehicleEnginePowerSource.PANTOGRAPH_TYPE_WBL85
    return RailVehicleEnginePowerSource.PANTOGRAPH_TYPE_NONE
