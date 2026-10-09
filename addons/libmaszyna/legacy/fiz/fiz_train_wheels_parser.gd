@tool
extends RefCounted
class_name FizTrainWheelsParser

## Wheels: section -> RailVehicleWheels. LoadFIZ_Wheels: Mover.cpp:10361.
##
## Setters are only called when the corresponding FIZ key is present - RailVehicleWheels' own
## compiled-in property defaults already match the FIZ format's "key absent" behavior, except
## WheelDiameterL/Dt which fall back to the powered wheel diameter (a cross-field default, not
## the compiled per-property default of 0.0).


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := MoverRailVehicleWheels.new()

    var diameter: float = FizLineUtil.get_float(kv, "D")
    if kv.has("D"):
        node.powered_wheel_diameter = diameter
    node.front_rolling_wheel_diameter = FizLineUtil.get_float(kv, "Dl", diameter)
    node.rear_rolling_wheel_diameter = FizLineUtil.get_float(kv, "Dt", diameter)
    if kv.has("Tw"):
        node.track_width = FizLineUtil.get_float(kv, "Tw")
    # NOTE: original LoadFIZ_Wheels derives AxleInertialMoment (and recomputes Mred) from wheel
    # diameter/axle count when AIM<=0 or absent; that derivation formula wasn't pinned down
    # precisely enough to replicate faithfully, so only the explicit key is applied here.
    if kv.has("AIM"):
        node.axle_inertial_moment = FizLineUtil.get_float(kv, "AIM")
    if kv.has("Axle"):
        node.axle_arrangement = FizLineUtil.get_string(kv, "Axle")
    if kv.has("BearingType"):
        node.bearing_type = (
                RailVehicleWheels.BEARING_TYPE_ROLL if FizLineUtil.get_string(kv, "BearingType").to_lower() == "roll"
                else RailVehicleWheels.BEARING_TYPE_SLIDE)
    if kv.has("Ad"):
        node.bogie_axle_spacing = FizLineUtil.get_float(kv, "Ad")
    if kv.has("Bd"):
        node.bogie_pivot_spacing = FizLineUtil.get_float(kv, "Bd")
    if kv.has("Rmin"):
        node.minimum_curve_radius = FizLineUtil.get_float(kv, "Rmin")

    context.add_part("RailVehicleWheels", node)
