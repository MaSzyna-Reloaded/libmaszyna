@tool
extends RefCounted
class_name FizTrainElectroPneumaticDynamicBrakeParser

## Blending: and DCEMUED: section parser -> RailVehicleElectroPneumaticDynamicBrake. Registered
## directly in FizVehicleBuilder's section table (both prefixes share this instance,
## since they configure the same node - a vehicle can have either or both).
##
## Blending: keys confirmed against a real vehicle line: `Blending: MED_Vmax=120 MED_Vmin=120
## MED_Vref=120 MED_amax=0.84 MED_EPVC=No MED_Ncor=No MED_MinBrakeReqED=0.15`.
##
## DCEMUED: keys confirmed against a real vehicle line: `DCEMUED: CouplerCheck=2
## EP_max_Vel=48`. Properties use their canonical public names directly.


func create_node() -> RailVehicleElectroPneumaticDynamicBrake:
    return MoverRailVehicleElectroPneumaticDynamicBrake.new()


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := _get_node(context)
    if node == null:
        node = create_node()
        context.add_part("RailVehicleElectroPneumaticDynamicBrake", node)

    if prefix == "DCEMUED:":
        _apply_dcemued(kv, node)
    else:
        _apply_blending(kv, node)


func _get_node(context: FizImportContext) -> RailVehicleElectroPneumaticDynamicBrake:
    var node: VehicleComponent = context.get_part("RailVehicleElectroPneumaticDynamicBrake")
    return node as RailVehicleElectroPneumaticDynamicBrake


func _apply_blending(kv: Dictionary, node: RailVehicleElectroPneumaticDynamicBrake) -> void:
    if kv.has("MED_Vmax"):
        node.blending_max_velocity = FizLineUtil.get_float(kv, "MED_Vmax")
    if kv.has("MED_Vmin"):
        node.blending_min_velocity = FizLineUtil.get_float(kv, "MED_Vmin")
    if kv.has("MED_Vref"):
        node.blending_reference_velocity = FizLineUtil.get_float(kv, "MED_Vref")
    if kv.has("MED_amax"):
        node.blending_max_deceleration = FizLineUtil.get_float(kv, "MED_amax")
    if kv.has("MED_EPVC"):
        node.blending_velocity_correction = FizLineUtil.get_bool(kv, "MED_EPVC")
    if kv.has("MED_Ncor"):
        node.blending_load_correction = FizLineUtil.get_bool(kv, "MED_Ncor")
    if kv.has("MED_MinBrakeReqED"):
        node.blending_min_ed_brake_request = FizLineUtil.get_float(kv, "MED_MinBrakeReqED")


func _apply_dcemued(kv: Dictionary, node: RailVehicleElectroPneumaticDynamicBrake) -> void:
    if kv.has("CouplerCheck"):
        node.coupler_check = FizLineUtil.get_int(kv, "CouplerCheck")
    if kv.has("EP_max_Vel"):
        node.electro_pneumatic_max_ep_brake_engagement_speed = FizLineUtil.get_float(kv, "EP_max_Vel")
    if kv.has("EP_delay"):
        node.electro_pneumatic_brake_delay = FizLineUtil.get_float(kv, "EP_delay")
    if kv.has("EP_min_Im"):
        node.electro_pneumatic_min_regenerative_braking = FizLineUtil.get_float(kv, "EP_min_Im")
