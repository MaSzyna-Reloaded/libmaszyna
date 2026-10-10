@tool
extends RefCounted
class_name FizTrainSecuritySystemParser

## Security: section parser -> RailVehicleSecuritySystem (czuwak/SHP/radiostop). Registered directly
## in FizVehicleBuilder's section table.
##
## Its keys and their defaults as TSecuritySystem::load reads them (Mover.cpp:235-254).


func create_node() -> RailVehicleSecuritySystem:
    return MoverRailVehicleSecuritySystem.new()


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := create_node()
    context.add_part("RailVehicleSecuritySystem", node)

    if kv.has("AwareSystem"):
        var tokens: PackedStringArray = FizLineUtil.get_string(kv, "AwareSystem").split(",")
        var flags: Array[String] = []
        for token: String in tokens:
            flags.append(token.strip_edges().to_lower())
        node.aware_system_active = "active" in flags
        node.aware_system_cabsignal = "cabsignal" in flags
        node.aware_system_separate_acknowledge = "separateacknowledge" in flags
        node.aware_system_sifa = "sifa" in flags
    if kv.has("AwareDelay"):
        node.aware_delay = FizLineUtil.get_float(kv, "AwareDelay")
    if kv.has("SoundSignalDelay"):
        node.sound_signal_delay = FizLineUtil.get_float(kv, "SoundSignalDelay")
    if kv.has("MaxHoldTime"):
        node.ca_max_hold_time = FizLineUtil.get_float(kv, "MaxHoldTime")
    if kv.has("EmergencyBrakeDelay"):
        node.emergency_brake_delay = FizLineUtil.get_float(kv, "EmergencyBrakeDelay")
    if kv.has("RadioStop"):
        node.radio_stop_enabled = FizLineUtil.get_bool(kv, "RadioStop")
    if kv.has("MagnetLocation"):
        node.shp_magnet_distance = FizLineUtil.get_float(kv, "MagnetLocation")
    if kv.has("AwareMinSpeed"):
        node.aware_min_speed = FizLineUtil.get_float(kv, "AwareMinSpeed")
    if kv.has("CabDependent"):
        node.cab_dependent = FizLineUtil.get_bool(kv, "CabDependent")
