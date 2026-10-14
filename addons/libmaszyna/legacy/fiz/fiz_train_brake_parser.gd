@tool
extends RefCounted
class_name FizTrainBrakeParser

## Brake: section, the brake-relevant subset of Cntrl. (delegated from FizTrainCntrlParser),
## and the brake-position table (BPT, rows immediately following the Cntrl. line) and
## CompressorList: table -> RailVehicleBrake. LoadFIZ_Brake: Mover.cpp:10394, brake subset of
## LoadFIZ_Cntrl: Mover.cpp:10707, readBPT: Mover.cpp:9200, readCompressorList: Mover.cpp:9499.
##
## Setters are only called when the corresponding FIZ key is present, except where the
## original LoadFIZ_Brake/_Cntrl logic computes a default that genuinely differs from
## RailVehicleBrake's compiled default (rig_effectiveness, valve/type "ESt" fallback).

const _VALVE_MAP := {
    "w": RailVehicleBrake.BRAKE_VALVE_W, "w_lu_l": RailVehicleBrake.BRAKE_VALVE_W_LU_L,
    "w_lu_xr": RailVehicleBrake.BRAKE_VALVE_W_LU_XR, "w_lu_vi": RailVehicleBrake.BRAKE_VALVE_W_LU_VI,
    "k": RailVehicleBrake.BRAKE_VALVE_K, "kg": RailVehicleBrake.BRAKE_VALVE_KG, "kp": RailVehicleBrake.BRAKE_VALVE_KP,
    "kss": RailVehicleBrake.BRAKE_VALVE_KSS, "kkg": RailVehicleBrake.BRAKE_VALVE_KKG, "kkp": RailVehicleBrake.BRAKE_VALVE_KKP,
    "kks": RailVehicleBrake.BRAKE_VALVE_KKS, "hikp1": RailVehicleBrake.BRAKE_VALVE_HIKP1,
    "hikss": RailVehicleBrake.BRAKE_VALVE_HIKSS, "hikg1": RailVehicleBrake.BRAKE_VALVE_HIKG1,
    "ke": RailVehicleBrake.BRAKE_VALVE_KE, "sw": RailVehicleBrake.BRAKE_VALVE_SW, "ested": RailVehicleBrake.BRAKE_VALVE_ESTED,
    "nest3": RailVehicleBrake.BRAKE_VALVE_NEST3, "est3": RailVehicleBrake.BRAKE_VALVE_EST3, "lst": RailVehicleBrake.BRAKE_VALVE_LST,
    "est4": RailVehicleBrake.BRAKE_VALVE_EST4, "est3al2": RailVehicleBrake.BRAKE_VALVE_EST3AL2,
    "ep1": RailVehicleBrake.BRAKE_VALVE_EP1, "ep2": RailVehicleBrake.BRAKE_VALVE_EP2, "m483": RailVehicleBrake.BRAKE_VALVE_M483,
    "cv1_l_tr": RailVehicleBrake.BRAKE_VALVE_CV1_L_TR, "cv1": RailVehicleBrake.BRAKE_VALVE_CV1,
    "cv1_r": RailVehicleBrake.BRAKE_VALVE_CV1_R,
}

const _METHOD_MAP := {
    "p10-bg": RailVehicleBrake.BRAKE_METHOD_P10_BG, "p10-bgu": RailVehicleBrake.BRAKE_METHOD_P10_BGU,
    "fr513": RailVehicleBrake.BRAKE_METHOD_FR513, "cosid": RailVehicleBrake.BRAKE_METHOD_COSID,
    "p10ybg": RailVehicleBrake.BRAKE_METHOD_P10Y_BG, "p10ybgu": RailVehicleBrake.BRAKE_METHOD_P10Y_BGU,
    "disk1": RailVehicleBrake.BRAKE_METHOD_D1, "disk1+mg": RailVehicleBrake.BRAKE_METHOD_D1MG,
    "disk2": RailVehicleBrake.BRAKE_METHOD_D2,
}

const _HANDLE_TYPE_MAP := {
    "fv4a": RailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A, "test": RailVehicleBrake.BRAKE_HANDLE_TYPE_TESTH,
    "d2": RailVehicleBrake.BRAKE_HANDLE_TYPE_D2, "mhz_en57": RailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_EN57,
    "mhz_k5p": RailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_K5P, "mhz_k8p": RailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_K8P,
    "mhz_6p": RailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_6P, "m394": RailVehicleBrake.BRAKE_HANDLE_TYPE_M394,
    "knorr": RailVehicleBrake.BRAKE_HANDLE_TYPE_KNORR, "westinghouse": RailVehicleBrake.BRAKE_HANDLE_TYPE_WESTINGHOUSE,
    "fvel6": RailVehicleBrake.BRAKE_HANDLE_TYPE_FVEL6, "fve408": RailVehicleBrake.BRAKE_HANDLE_TYPE_FVE408,
    "st113": RailVehicleBrake.BRAKE_HANDLE_TYPE_ST113,
}

## The local brake handles LoadFIZ_Cntrl knows (Mover.cpp:10778)
const _LOCAL_HANDLE_TYPE_MAP := {
    "fd1": RailVehicleBrake.BRAKE_HANDLE_TYPE_FD1, "knorr": RailVehicleBrake.BRAKE_HANDLE_TYPE_KNORR,
    "westinghouse": RailVehicleBrake.BRAKE_HANDLE_TYPE_WESTINGHOUSE,
}

const _LOCAL_BRAKE_TYPE_MAP := {
    "manualbrake": RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL, "pneumaticbrake": RailVehicleBrake.LOCAL_BRAKE_TYPE_PNEUMATIC,
    "hydraulicbrake": RailVehicleBrake.LOCAL_BRAKE_TYPE_HYDRAULIC,
}


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    if prefix == "CompressorList:":
        # header carries only CompressorListPosNo/Wrap/DefPos, none of which have a Godot
        # property home on RailVehicleBrake yet - just start collecting rows.
        _active_table = "CompressorList"
        _compressor_rows = []
        return

    var node := MoverRailVehicleBrake.new()
    _parse_brake(kv, node)
    # Mover.cpp:10528 - by default a diesel's releaser works only at the controller's zero; the
    # engine is the one parsed before this line, as in the original
    node.releaser_enabled_only_at_no_power_pos = FizLineUtil.get_bool(kv, "ReleaserPowerPosLock") \
            if kv.has("ReleaserPowerPosLock") \
            else context.engine_type in [RailVehicleEngine.DIESEL, RailVehicleEngine.DIESEL_ELECTRIC]
    context.add_part("RailVehicleBrake", node)


func _parse_brake(kv: Dictionary, node: RailVehicleBrake) -> void:
    if kv.has("AirLeakRate"):
        node.air_leak_multiplier = FizLineUtil.get_float(kv, "AirLeakRate") * 0.01

    var method_str: String = FizLineUtil.get_string(kv, "BM").to_lower()
    if method_str:
        if _METHOD_MAP.has(method_str):
            node.brake_method = _METHOD_MAP[method_str]
        else:
            push_warning("FIZ Brake:BM: unmapped value '%s'" % method_str)

    if kv.has("MBF"):
        node.brake_force_max = FizLineUtil.get_float(kv, "MBF")
    if kv.has("TBF"):
        node.brake_force_traction = FizLineUtil.get_float(kv, "TBF")

    if kv.has("MaxBP"):
        var max_cylinder_pressure: float = FizLineUtil.get_float(kv, "MaxBP")
        node.max_cylinder_pressure = max_cylinder_pressure
        if kv.has("BCN"):
            node.cylinder_count = FizLineUtil.get_int(kv, "BCN")
        node.max_aux_pressure = FizLineUtil.get_float(kv, "MaxLBP", max_cylinder_pressure)
        if kv.has("TareMaxBP"):
            node.max_tare_pressure = FizLineUtil.get_float(kv, "TareMaxBP")
        if kv.has("MedMaxBP"):
            node.max_medium_pressure = FizLineUtil.get_float(kv, "MedMaxBP")
        if kv.has("MaxASBP"):
            node.max_antislip_pressure = FizLineUtil.get_float(kv, "MaxASBP")

    if kv.has("BCR"):
        node.cylinder_radius = FizLineUtil.get_float(kv, "BCR")
    if kv.has("BCD"):
        node.cylinder_distance = FizLineUtil.get_float(kv, "BCD")
    if kv.has("BCS"):
        node.cylinder_spring_force = FizLineUtil.get_float(kv, "BCS")
    if kv.has("BSA"):
        node.piston_stroke_adjuster_resistance = FizLineUtil.get_float(kv, "BSA")
    # rig_effectiveness' FIZ-format default (1.0) differs from RailVehicleBrake's compiled default (0.0).
    node.rig_effectiveness = FizLineUtil.get_float(kv, "BRE", 1.0)
    if kv.has("BCM"):
        node.cylinder_gear_ratio = FizLineUtil.get_float(kv, "BCM")
    if kv.has("BCMlo"):
        node.cylinder_gear_ratio_low = FizLineUtil.get_float(kv, "BCMlo")
    if kv.has("BCMHi"):
        node.cylinder_gear_ratio_high = FizLineUtil.get_float(kv, "BCMHi")
    if kv.has("Size"):
        node.est_valve_size = FizLineUtil.get_int(kv, "Size")
    if kv.has("NBpA"):
        node.friction_elements_per_axle = FizLineUtil.get_int(kv, "NBpA")

    if kv.has("LPOn"):
        node.main_pipe_blocking_pressure = FizLineUtil.get_float(kv, "LPOn")
    if kv.has("LPOff"):
        node.main_pipe_unblocking_pressure = FizLineUtil.get_float(kv, "LPOff")
    if kv.has("HandlePipeUnlockPos"):
        node.main_pipe_minimum_unblocking_handle_position = FizLineUtil.get_int(kv, "HandlePipeUnlockPos")
    if kv.has("EmergencyCutsOffHandle"):
        node.main_pipe_emergency_cuts_off_handle = FizLineUtil.get_bool(kv, "EmergencyCutsOffHandle")

    var high_pressure: float = FizLineUtil.get_float(kv, "HiPP", 5.0)
    if kv.has("HiPP"):
        node.pipe_pressure_max = high_pressure
    node.pipe_pressure_min = FizLineUtil.get_float(kv, "LoPP", minf(high_pressure, 3.5))

    if kv.has("Vv"):
        node.tank_volume_main = FizLineUtil.get_float(kv, "Vv")
    if kv.has("BVV"):
        node.tank_volume_aux = FizLineUtil.get_float(kv, "BVV")

    if kv.has("MinCP"):
        node.compressor_cab_a_min_pressure = FizLineUtil.get_float(kv, "MinCP")
    if kv.has("MaxCP"):
        node.compressor_cab_a_max_pressure = FizLineUtil.get_float(kv, "MaxCP")
    if kv.has("MinCP_B"):
        node.compressor_cab_b_min_pressure = FizLineUtil.get_float(kv, "MinCP_B")
    if kv.has("MaxCP_B"):
        node.compressor_cab_b_max_pressure = FizLineUtil.get_float(kv, "MaxCP_B")
    if kv.has("CompressorSpeed"):
        node.compressor_speed = FizLineUtil.get_float(kv, "CompressorSpeed")
    if kv.has("CompressorPower"):
        match FizLineUtil.get_string(kv, "CompressorPower").to_lower():
            "main": node.compressor_power = RailVehicleBrake.COMPRESSOR_POWER_MAIN
            "converter": node.compressor_power = RailVehicleBrake.COMPRESSOR_POWER_CONVERTER
            "engine": node.compressor_power = RailVehicleBrake.COMPRESSOR_POWER_ENGINE
            "coupler1": node.compressor_power = RailVehicleBrake.COMPRESSOR_POWER_COUPLER1
            "coupler2": node.compressor_power = RailVehicleBrake.COMPRESSOR_POWER_COUPLER2
    if kv.has("CompressorTankValve"):
        node.compressor_tank_valve_active = FizLineUtil.get_bool(kv, "CompressorTankValve")
    if kv.has("EVArea"):
        node.compressor_emergency_valve_area = FizLineUtil.get_float(kv, "EVArea")
    if kv.has("MinEVP"):
        node.compressor_lower_emergency_closing_pressure = FizLineUtil.get_float(kv, "MinEVP")
    if kv.has("MaxEVP"):
        node.compressor_higher_emergency_closing_pressure = FizLineUtil.get_float(kv, "MaxEVP")

    if kv.has("UBB1"):
        node.universal_brake_button_1 = FizLineUtil.get_int(kv, "UBB1")
    if kv.has("UBB2"):
        node.universal_brake_button_2 = FizLineUtil.get_int(kv, "UBB2")
    if kv.has("UBB3"):
        node.universal_brake_button_3 = FizLineUtil.get_int(kv, "UBB3")

    if kv.has("RM"):
        node.rapid_transfer = FizLineUtil.get_float(kv, "RM")
    if kv.has("RV"):
        node.rapid_switching_speed = FizLineUtil.get_float(kv, "RV")

    node.valve_parameters = FizLineUtil.get_string(kv, "BrakeValve")
    var valve_str: String = node.valve_parameters.to_lower()
    if valve_str:
        if _VALVE_MAP.has(valve_str):
            node.valve_type = _VALVE_MAP[valve_str]
        elif valve_str.find("est") != -1:
            node.valve_type = RailVehicleBrake.BRAKE_VALVE_EST3
        else:
            node.valve_type = RailVehicleBrake.BRAKE_VALVE_OTHER


## Called by FizTrainCntrlParser with the full Cntrl. key/value set - applies only the
## brake-relevant subset.
## BrakeDelays= (Mover.cpp:10738-10746)
const _DELAY_MAP := {
    "g": RailVehicleBrake.BRAKE_DELAY_G, "p": RailVehicleBrake.BRAKE_DELAY_P, "r": RailVehicleBrake.BRAKE_DELAY_R,
    "gp": RailVehicleBrake.BRAKE_DELAY_GP, "pr": RailVehicleBrake.BRAKE_DELAY_PR,
    "gpr": RailVehicleBrake.BRAKE_DELAY_GPR, "gpr+mg": RailVehicleBrake.BRAKE_DELAY_GPR_MG,
    "pr+mg": RailVehicleBrake.BRAKE_DELAY_PR_MG,
}


func apply_cntrl(kv: Dictionary, node: RailVehicleBrake, context: FizImportContext) -> void:
    var brake_system: int = RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL
    match FizLineUtil.get_string(kv, "BrakeSystem").to_lower():
        "pneumatic": brake_system = RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC
        "electropneumatic": brake_system = RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC
    node.cntrl_brake_system = brake_system
    context.brake_system = brake_system

    # LoadFIZ_Cntrl (Mover.cpp:10817-10895): the local, manual and dynamic brake and the spring
    # brake keys are read whatever the brake system
    var local_brake_str: String = FizLineUtil.get_string(kv, "LocalBrake").to_lower()
    if _LOCAL_BRAKE_TYPE_MAP.has(local_brake_str):
        node.cntrl_local_brake_type = _LOCAL_BRAKE_TYPE_MAP[local_brake_str]
    if kv.has("ManualBrake"):
        node.cntrl_manual_brake_present = FizLineUtil.get_bool(kv, "ManualBrake")
    var dynamic_str: String = FizLineUtil.get_string(kv, "DynamicBrake").to_lower()
    match dynamic_str:
        "passive": node.cntrl_dynamic_brake_type = RailVehicleBrake.DYNAMIC_BRAKE_PASSIVE
        "switch": node.cntrl_dynamic_brake_type = RailVehicleBrake.DYNAMIC_BRAKE_SWITCH
        "reversal": node.cntrl_dynamic_brake_type = RailVehicleBrake.DYNAMIC_BRAKE_REVERSAL
        "automatic": node.cntrl_dynamic_brake_type = RailVehicleBrake.DYNAMIC_BRAKE_AUTOMATIC
    if kv.has("LocalBrakeTraxx"):
        node.cntrl_local_brake_traxx = FizLineUtil.get_bool(kv, "LocalBrakeTraxx")
    if kv.has("ReleaseParkingBySpringBrake"):
        node.cntrl_release_parking_by_spring_brake = FizLineUtil.get_bool(kv, "ReleaseParkingBySpringBrake")
    if kv.has("ReleaseParkingBySpringBrakeWhenDoorIsOpen"):
        node.cntrl_release_parking_by_spring_brake_when_door_open = FizLineUtil.get_bool(kv, "ReleaseParkingBySpringBrakeWhenDoorIsOpen")
    if kv.has("SpringBrakeCutsOffDrive"):
        node.cntrl_spring_brake_cuts_off_drive = FizLineUtil.get_bool(kv, "SpringBrakeCutsOffDrive")
    if kv.has("SpringBrakeDriveEmergencyVel"):
        node.cntrl_spring_brake_drive_emergency_velocity = FizLineUtil.get_float(kv, "SpringBrakeDriveEmergencyVel")

    if brake_system == RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL:
        return

    if kv.has("BCPN"):
        node.cntrl_brake_ctrl_position_count = FizLineUtil.get_int(kv, "BCPN")
    if kv.has("BDelay1"):
        node.cntrl_brake_delay_1 = FizLineUtil.get_float(kv, "BDelay1")
    if kv.has("BDelay2"):
        node.cntrl_brake_delay_2 = FizLineUtil.get_float(kv, "BDelay2")
    if kv.has("BDelay3"):
        node.cntrl_brake_delay_3 = FizLineUtil.get_float(kv, "BDelay3")
    if kv.has("BDelay4"):
        node.cntrl_brake_delay_4 = FizLineUtil.get_float(kv, "BDelay4")
    # the mass the cylinders reach MaxBP at [t] (Mover.cpp:10771)
    if kv.has("MaxBPMass"):
        node.cntrl_max_brake_pressure_mass = FizLineUtil.get_float(kv, "MaxBPMass")

    var delays_str: String = FizLineUtil.get_string(kv, "BrakeDelays").to_lower()
    if _DELAY_MAP.has(delays_str):
        node.cntrl_brake_delays = _DELAY_MAP[delays_str]

    var op_modes_str: String = FizLineUtil.get_string(kv, "BrakeOpModes").to_lower()
    match op_modes_str:
        "pn": node.cntrl_brake_op_modes = RailVehicleBrake.BRAKE_OP_MODE_PN
        "pnepmed": node.cntrl_brake_op_modes = RailVehicleBrake.BRAKE_OP_MODE_PNEPMED
        "pnep": node.cntrl_brake_op_modes = RailVehicleBrake.BRAKE_OP_MODE_PNEP

    var handle_str: String = FizLineUtil.get_string(kv, "BrakeHandle").to_lower()
    if _HANDLE_TYPE_MAP.has(handle_str):
        node.cntrl_brake_handle_type = _HANDLE_TYPE_MAP[handle_str]
    # a key moves an FV4a while held, any other handle a position per press
    # (OnCommand_trainbrakeincrease(), Train.cpp:1960-1966)
    if node.cntrl_brake_handle_type == RailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A:
        node.handle_movement = RailVehicleBrake.BRAKE_HANDLE_MOVEMENT_CONTINUOUS
    var loc_handle_str: String = FizLineUtil.get_string(kv, "LocBrakeHandle").to_lower()
    if _LOCAL_HANDLE_TYPE_MAP.has(loc_handle_str):
        node.cntrl_local_brake_handle_type = _LOCAL_HANDLE_TYPE_MAP[loc_handle_str]

    match FizLineUtil.get_string(kv, "ASB").to_lower():
        "manual": node.cntrl_anti_skid_brake_type = RailVehicleBrake.ANTI_SKID_BRAKE_MANUAL
        "automatic": node.cntrl_anti_skid_brake_type = RailVehicleBrake.ANTI_SKID_BRAKE_AUTOMATIC
        "yes": node.cntrl_anti_skid_brake_type = RailVehicleBrake.ANTI_SKID_BRAKE_YES


func wants_bpt_table(context: FizImportContext) -> bool:
    if context.brake_system == RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL:
        return false
    _active_table = "BPT"
    _bpt_rows = []
    return true


var _bpt_rows: Array[RailVehicleBrakePressureTableItem] = []
var _compressor_rows: Array[RailVehicleCompressorListItem] = []
var _active_table: String = ""


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    match _active_table:
        "BPT": _parse_bpt_row(p)
        "CompressorList": _parse_compressor_row(p)


func _parse_bpt_row(p: MaszynaParser) -> void:
    var tokens: Array = p.get_tokens(5)
    if tokens.size() < 5:
        return
    var item := RailVehicleBrakePressureTableItem.new()
    item.handle_position = int(tokens[0])
    item.pipe_pressure = float(tokens[1])
    item.brake_cylinder_pressure = float(tokens[2])
    item.fill_speed = float(tokens[3])
    match String(tokens[4]).to_lower():
        "pneumatic", "p": item.brake_type = RailVehicleBrakePressureTableItem.BRAKE_TYPE_PNEUMATIC
        "electropneumatic", "ep": item.brake_type = RailVehicleBrakePressureTableItem.BRAKE_TYPE_ELECTRO_PNEUMATIC
        _: item.brake_type = RailVehicleBrakePressureTableItem.BRAKE_TYPE_INDIVIDUAL
    _bpt_rows.append(item)


func _parse_compressor_row(p: MaszynaParser) -> void:
    var tokens: Array = p.get_tokens(4)
    if tokens.size() < 4:
        return
    var item := RailVehicleCompressorListItem.new()
    item.allow = int(tokens[0])
    item.speed_factor = int(tokens[1])
    item.min_pressure_factor = int(tokens[2])
    item.max_pressure_factor = int(tokens[3])
    _compressor_rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node: RailVehicleBrake = context.get_part("RailVehicleBrake")
    if node == null:
        return
    if _bpt_rows:
        node.brake_pressure_table = _bpt_rows
        _bpt_rows = []
    if _compressor_rows:
        node.compressor_list = _compressor_rows
        _compressor_rows = []
    _active_table = ""
