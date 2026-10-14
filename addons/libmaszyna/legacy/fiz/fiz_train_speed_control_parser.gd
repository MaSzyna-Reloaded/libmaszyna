@tool
extends RefCounted
class_name FizTrainSpeedControlParser

## SpeedControl: section parser -> RailVehicleSpeedControl. Registered directly in
## FizVehicleBuilder's section table.
##
## Its keys and their defaults as LoadFIZ_SpeedControl reads them (Mover.cpp:11093-11130).


func create_node() -> RailVehicleSpeedControl:
    return MoverRailVehicleSpeedControl.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleSpeedControl", node)

    if kv.has("SpeedCtrl"):
        node.speed_control_enabled = FizLineUtil.get_bool(kv, "SpeedCtrl")
    if kv.has("SpeedCtrlDelay"):
        node.delay = FizLineUtil.get_float(kv, "SpeedCtrlDelay")
    # SpeedCtrlTypeTime (Mover.cpp:11100)
    node.impulse_lever = FizLineUtil.get_string(kv, "SpeedCtrlType") == "Time"
    if kv.has("SpeedCtrlATOF"):
        node.disables_on = FizLineUtil.get_int(kv, "SpeedCtrlATOF")
    if kv.has("SpeedButtons"):
        var tokens: PackedStringArray = FizLineUtil.get_string(kv, "SpeedButtons").split("|")
        var speeds := PackedFloat64Array()
        for token: String in tokens:
            if token.strip_edges().is_valid_float():
                speeds.append(token.strip_edges().to_float())
        node.preset_speeds = speeds
    if kv.has("OverrideManual"):
        node.override_manual_power = FizLineUtil.get_bool(kv, "OverrideManual")
    if kv.has("InitPwr"):
        node.initial_power = FizLineUtil.get_float(kv, "InitPwr")
    if kv.has("MaxPwrVel"):
        node.full_power_velocity = FizLineUtil.get_float(kv, "MaxPwrVel")
    if kv.has("StartVel"):
        node.start_velocity = FizLineUtil.get_float(kv, "StartVel")
    if kv.has("VelStep"):
        node.velocity_step = FizLineUtil.get_float(kv, "VelStep")
    if kv.has("PwrStep"):
        node.power_step = FizLineUtil.get_float(kv, "PwrStep")
    if kv.has("MinPwr"):
        node.min_power = FizLineUtil.get_float(kv, "MinPwr")
    if kv.has("MaxPwr"):
        node.max_power = FizLineUtil.get_float(kv, "MaxPwr")
    if kv.has("MinVel"):
        node.min_velocity = FizLineUtil.get_float(kv, "MinVel")
    if kv.has("MaxVel"):
        node.max_velocity = FizLineUtil.get_float(kv, "MaxVel")
    if kv.has("Offset"):
        node.offset = FizLineUtil.get_float(kv, "Offset")
    if kv.has("kPpos"):
        node.proportional_gain_positive = FizLineUtil.get_float(kv, "kPpos")
    if kv.has("kPneg"):
        node.proportional_gain_negative = FizLineUtil.get_float(kv, "kPneg")
    if kv.has("kIpos"):
        node.integral_gain_positive = FizLineUtil.get_float(kv, "kIpos")
    if kv.has("kIneg"):
        node.integral_gain_negative = FizLineUtil.get_float(kv, "kIneg")
    if kv.has("BrakeIntervention"):
        node.brake_intervention = FizLineUtil.get_bool(kv, "BrakeIntervention")
    if kv.has("BrakeIntMaxVel"):
        node.brake_intervention_max_velocity = FizLineUtil.get_float(kv, "BrakeIntMaxVel")
    if kv.has("PowerUpSpeed"):
        node.power_up_speed = FizLineUtil.get_float(kv, "PowerUpSpeed")
    if kv.has("PowerDownSpeed"):
        node.power_down_speed = FizLineUtil.get_float(kv, "PowerDownSpeed")
