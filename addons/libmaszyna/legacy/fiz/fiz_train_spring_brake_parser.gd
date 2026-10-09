@tool
extends RefCounted
class_name FizTrainSpringBrakeParser

## SpringBrake: section parser -> RailVehicleSpringBrake. Registered directly in
## FizVehicleBuilder's section table. Key mapping confirmed directly against
## RailVehicleSpringBrake::_do_update_internal_mover's own field-name comments (which mirror the
## original MaSzyna short key names almost 1:1, e.g. MaxSetPressure/ResetPressure/PressureOff).


func create_node() -> RailVehicleSpringBrake:
    return MoverRailVehicleSpringBrake.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleSpringBrake", node)

    if kv.has("Volume"):
        node.spring_actuator_chamber_volume = FizLineUtil.get_float(kv, "Volume")
    if kv.has("MBF"):
        node.pressure_force_coefficient = FizLineUtil.get_float(kv, "MBF")
    if kv.has("MaxSP"):
        node.spring_actuator_preload_pressure = FizLineUtil.get_float(kv, "MaxSP")
    if kv.has("ResetP"):
        node.spring_actuator_max_filling_force = FizLineUtil.get_float(kv, "ResetP")
    if kv.has("MinFP"):
        node.spring_full_balance_pressure = FizLineUtil.get_float(kv, "MinFP")
    if kv.has("PressOff"):
        node.brake_signal_released_state_pressure = FizLineUtil.get_float(kv, "PressOff")
    if kv.has("PressOn"):
        node.brake_signal_braked_state_pressure = FizLineUtil.get_float(kv, "PressOn")
    if kv.has("ValveOnArea"):
        node.valve_cross_section_actuator_discharge = FizLineUtil.get_float(kv, "ValveOnArea")
    if kv.has("ValveOffArea"):
        node.valve_cross_section_actuator_charge = FizLineUtil.get_float(kv, "ValveOffArea")
    if kv.has("ValvePNBArea"):
        node.valve_cross_section_pneumatic_brake = FizLineUtil.get_float(kv, "ValvePNBArea")
    if kv.has("MTC"):
        node.required_coupler_connection_method = FizLineUtil.get_int(kv, "MTC")
