@tool
extends RefCounted
class_name FizTrainCntrlParser

## Cntrl. section dispatcher: this single FIZ section's keys fan out to several different
## Godot classes (VehicleController general subset, RailVehiclePowerSupply's battery/converter
## subset, RailVehicleMasterController - a vehicle with a cab only, an engine's or not -,
## RailVehicleBrake brake subset, and later
## RailVehicleEngine's controller-position-count subset once Engine: creates that node - Cntrl.
## conventionally appears before Engine: in real files, so the engine-relevant keys are
## stashed on the context for Engine:'s parser to pick up). Also owns the brake-position table
## (BPT) that immediately follows the Cntrl. line, by delegating to the brake parser.
## LoadFIZ_Cntrl: Mover.cpp:10707.

## A master controller has more than one position. Every vehicle has Cntrl. - it carries the
## brake keys - and wagons, even pedestrians, write MCPN=1 there; the original takes any
## MainCtrlPosNo > 0 for "has steering" (Mover.cpp:2380), yet one position is nothing to turn, and
## only an MMD gives a vehicle the cab to turn it from (MASZYNA_ORIGINAL_QUIRKS.md).
const MASTER_CONTROLLER_MIN_POSITIONS: int = 1

var controller_parser: FizTrainControllerParser
var brake_parser: FizTrainBrakeParser


func _init(p_controller_parser: FizTrainControllerParser, p_brake_parser: FizTrainBrakeParser) -> void:
    controller_parser = p_controller_parser
    brake_parser = p_brake_parser


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    controller_parser.apply_cntrl(kv, context)

    if FizLineUtil.get_int(kv, "MCPN") > MASTER_CONTROLLER_MIN_POSITIONS:
        var master_controller:MoverRailVehicleMasterController = MoverRailVehicleMasterController.new()
        master_controller.main_position_count = FizLineUtil.get_int(kv, "MCPN")
        master_controller.second_position_count = FizLineUtil.get_int(kv, "SCPN")
        master_controller.direction_change_max_position = FizLineUtil.get_int(kv, "DirChangeMaxPos")
        master_controller.coupled_controllers = FizLineUtil.get_bool(kv, "CoupledCtrl")
        master_controller.initial_delay = FizLineUtil.get_float(kv, "IniCDelay")
        master_controller.step_delay = FizLineUtil.get_float(kv, "SCDelay")
        # without SCDDelay stepping down is as slow as up (Mover.cpp:10868)
        master_controller.step_down_delay = FizLineUtil.get_float(kv, "SCDDelay", master_controller.step_delay)
        master_controller.tachometer_max_speed = FizLineUtil.get_float(kv, "MaxTachoSpeed")
        context.add_part("RailVehicleMasterController", master_controller)

    var brake: RailVehicleBrake = context.get_part("RailVehicleBrake")
    if brake != null:
        brake_parser.apply_cntrl(kv, brake, context)
    else:
        push_warning("FIZ Cntrl.: no RailVehicleBrake node yet (Brake: should precede Cntrl.) - brake-related Cntrl. keys ignored.")

    # Engine:'s subset (AutoRelay, Camshaft, ...) is applied once
    # Engine: creates the RailVehicleEngine-family node when Cntrl. precedes it, or here when it comes
    # after - EN57's Cntrl. is in the brake include that follows its Engine:
    context.cntrl_kv = kv
    var engine: RailVehicleEngine = context.get_part("RailVehicleEngine") as RailVehicleEngine
    if engine:
        FizTrainEngineCommon.apply_cntrl_engine_subset(engine, kv)


func wants_bpt_table(context: FizImportContext) -> bool:
    return brake_parser.wants_bpt_table(context)


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    brake_parser.parse_row(p, context)


func end_table(context: FizImportContext) -> void:
    brake_parser.end_table(context)
