@tool
extends RefCounted
class_name FizTrainBuffCouplParser

## BuffCoupl./BuffCoupl1./BuffCoupl2. sections -> RailVehicleBuffCoupl (one node per coupler end
## encountered). LoadFIZ_BuffCoupl: Mover.cpp:10619.
##
## Setters are only called when the corresponding FIZ key is present - RailVehicleBuffCoupl's own
## compiled-in property defaults already match the FIZ format's "key absent" behavior.
## kC/FmaxC/kB/FmaxB stay in the FIZ units (kN, kN/m): RailVehicleBuffCoupl applies the x1000
## conversion itself, depending on the coupler type (Mover.cpp:10350).

const _COUPLER_TYPE_MAP := {
    "automatic": RailVehicleBuffCoupl.COUPLER_TYPE_AUTOMATIC,
    "screw": RailVehicleBuffCoupl.COUPLER_TYPE_SCREW,
    "chain": RailVehicleBuffCoupl.COUPLER_TYPE_CHAIN,
    "bare": RailVehicleBuffCoupl.COUPLER_TYPE_BARE,
    "articulated": RailVehicleBuffCoupl.COUPLER_TYPE_ARTICULATED,
}


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := MoverRailVehicleBuffCoupl.new()
    node.buffer_location = _buffer_location_for(prefix)

    var coupler_type: int = _COUPLER_TYPE_MAP.get(FizLineUtil.get_string(kv, "CType").to_lower(), RailVehicleBuffCoupl.COUPLER_TYPE_AUTOMATIC)
    if kv.has("CType"):
        node.coupler_type = coupler_type

    if kv.has("kC"):
        node.coupler_stiffness_k = FizLineUtil.get_float(kv, "kC")
    if kv.has("DmaxC"):
        node.coupler_max_compression_tolerance = FizLineUtil.get_float(kv, "DmaxC")
    if kv.has("FmaxC"):
        node.coupler_max_tension_tolerance = FizLineUtil.get_float(kv, "FmaxC")

    if kv.has("kB"):
        node.buffer_stiffness_k = FizLineUtil.get_float(kv, "kB")
    if kv.has("DmaxB"):
        node.buffer_max_compression_tolerance = FizLineUtil.get_float(kv, "DmaxB")
    if kv.has("FmaxB"):
        node.buffer_max_tension_tolerance = FizLineUtil.get_float(kv, "FmaxB")

    if kv.has("beta"):
        node.damping_beta = FizLineUtil.get_float(kv, "beta")

    if kv.has("AllowedFlag"):
        var allowed: int = FizLineUtil.get_int(kv, "AllowedFlag")
        if allowed < 0:
            allowed = -allowed | RailVehicleController.COUPLING_FLAG_PERMANENT
        node.allowed_flag = allowed
    if kv.has("AutomaticFlag"):
        node.automatic_flag = FizLineUtil.get_int(kv, "AutomaticFlag")
    if kv.has("PowerCoupling"):
        node.power_coupling = FizLineUtil.get_int(kv, "PowerCoupling")
    if kv.has("PowerFlag"):
        node.power_flag = FizLineUtil.get_int(kv, "PowerFlag")
    if kv.has("ControlType"):
        node.control_type = FizLineUtil.get_string(kv, "ControlType")

    context.add_part(_part_name_for(prefix, context), node)


func _part_name_for(prefix: String, context: FizImportContext) -> String:
    match prefix:
        "BuffCoupl1.": return "TrainBuffCouplFront"
        "BuffCoupl2.": return "TrainBuffCouplBack"
        _: return "RailVehicleBuffCoupl"


## LoadFIZ_BuffCoupl (Mover.cpp:9613-9629): BuffCoupl. describes both couplers, BuffCoupl1. the
## front one and BuffCoupl2. the rear one.
func _buffer_location_for(prefix: String) -> RailVehicleBuffCoupl.BufferLocation:
    match prefix:
        "BuffCoupl1.": return RailVehicleBuffCoupl.BUFFER_LOCATION_FRONT
        "BuffCoupl2.": return RailVehicleBuffCoupl.BUFFER_LOCATION_BACK
        _: return RailVehicleBuffCoupl.BUFFER_LOCATION_BOTH
