@tool
extends RefCounted
class_name FizTrainDoorsParser

## Doors: section -> RailVehicleDoors. LoadFIZ_Doors: Mover.cpp:10537.
##
## Setters are only called when the corresponding FIZ key is present - RailVehicleDoors' own
## compiled-in property defaults already match the FIZ format's "key absent" behavior, except
## `voltage` (default depends on open/close method) and `max_shift` (two alternate source
## keys with a priority rule), which are genuine cross-field/conditional defaults.

const _CONTROLS_MAP := {
    "passenger": RailVehicleDoors.CONTROLS_PASSENGER,
    "automaticctrl": RailVehicleDoors.CONTROLS_AUTOMATIC,
    "driverctrl": RailVehicleDoors.CONTROLS_DRIVER,
    "conductor": RailVehicleDoors.CONTROLS_CONDUCTOR,
    "mixed": RailVehicleDoors.CONTROLS_MIXED,
}

const _TYPE_MAP := {
    "shift": RailVehicleDoors.TYPE_SHIFT,
    "rotate": RailVehicleDoors.TYPE_ROTATE,
    "fold": RailVehicleDoors.TYPE_FOLD,
    "plug": RailVehicleDoors.TYPE_PLUG,
}


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := MoverRailVehicleDoors.new()

    var open_method: int = _CONTROLS_MAP.get(FizLineUtil.get_string(kv, "OpenCtrl").to_lower(), RailVehicleDoors.CONTROLS_PASSENGER)
    if kv.has("OpenCtrl"):
        node.open_method = open_method
    if kv.has("CloseCtrl"):
        node.close_method = _CONTROLS_MAP.get(FizLineUtil.get_string(kv, "CloseCtrl").to_lower(), RailVehicleDoors.CONTROLS_PASSENGER)

    if kv.has("DoorStayOpen"):
        node.open_time = FizLineUtil.get_float(kv, "DoorStayOpen")
    if kv.has("OpenSpeed"):
        node.open_speed = FizLineUtil.get_float(kv, "OpenSpeed")
    if kv.has("DoorOpenDelay"):
        node.open_delay = FizLineUtil.get_float(kv, "DoorOpenDelay")
    if kv.has("CloseSpeed"):
        node.close_speed = FizLineUtil.get_float(kv, "CloseSpeed")
    if kv.has("DoorCloseDelay"):
        node.close_delay = FizLineUtil.get_float(kv, "DoorCloseDelay")
    if kv.has("DoorClosureWarning"):
        node.close_warning = FizLineUtil.get_bool(kv, "DoorClosureWarning")
    if kv.has("DoorClosureWarningAuto"):
        node.close_auto_close_warning = FizLineUtil.get_bool(kv, "DoorClosureWarningAuto")
    if kv.has("DoorAutoCloseRemote"):
        node.close_auto_close_remote = FizLineUtil.get_bool(kv, "DoorAutoCloseRemote")
    if kv.has("DoorAutoCloseVel"):
        node.close_auto_close_velocity = FizLineUtil.get_float(kv, "DoorAutoCloseVel")
    if kv.has("DoorBlocked"):
        node.has_lock = FizLineUtil.get_bool(kv, "DoorBlocked")
    if kv.has("DoorOpenWithPermit"):
        node.open_with_permit = FizLineUtil.get_float(kv, "DoorOpenWithPermit")

    # DoorMaxShiftL and DoorMaxShiftR both write the same field; when both are present, R wins.
    if kv.has("DoorMaxShiftL"):
        node.max_shift = FizLineUtil.get_float(kv, "DoorMaxShiftL")
    if kv.has("DoorMaxShiftR"):
        node.max_shift = FizLineUtil.get_float(kv, "DoorMaxShiftR")
    if kv.has("DoorMaxShiftPlug"):
        node.max_shift_plug = FizLineUtil.get_float(kv, "DoorMaxShiftPlug")

    if kv.has("DoorOpenMethod"):
        node.type = _TYPE_MAP.get(FizLineUtil.get_string(kv, "DoorOpenMethod").to_lower(), RailVehicleDoors.TYPE_ROTATE)

    # DoorVoltage's absent-key default depends on whether doors are remote-controlled, which
    # differs from the compiled default (0/unset) for driver/conductor/mixed doors.
    var voltage_str: String = FizLineUtil.get_string(kv, "DoorVoltage")
    if not voltage_str:
        var remote: bool = open_method in [RailVehicleDoors.CONTROLS_DRIVER, RailVehicleDoors.CONTROLS_CONDUCTOR, RailVehicleDoors.CONTROLS_MIXED]
        if remote:
            node.voltage = RailVehicleDoors.VOLTAGE_24
    else:
        match voltage_str.to_int():
            12: node.voltage = RailVehicleDoors.VOLTAGE_12
            24: node.voltage = RailVehicleDoors.VOLTAGE_24
            110: node.voltage = RailVehicleDoors.VOLTAGE_110
            0: node.voltage = RailVehicleDoors.VOLTAGE_0
            _: push_warning("FIZ Doors:DoorVoltage: unexpected value '%s'" % voltage_str)

    if kv.has("DoorNeedPermit"):
        node.permit_required = FizLineUtil.get_bool(kv, "DoorNeedPermit")
    var permit_list_str: String = FizLineUtil.get_string(kv, "DoorPermitList")
    if permit_list_str:
        var permit_list: Array = []
        for part: String in permit_list_str.split("|", false):
            permit_list.append(part.to_int())
        if permit_list:
            node.permit_list = permit_list
            node.permit_default = FizLineUtil.get_int(kv, "DoorPermitListDefault", 1)
    if kv.has("DoorsPermitLightBlinking"):
        node.permit_light_blinking = FizLineUtil.get_int(kv, "DoorsPermitLightBlinking")

    if kv.has("PlatformSpeed"):
        node.platform_speed = FizLineUtil.get_float(kv, "PlatformSpeed")
    if kv.has("PlatformMaxShift"):
        node.platform_max_shift = FizLineUtil.get_float(kv, "PlatformMaxShift")
    if kv.has("PlatformMaxSpeed"):
        node.platform_max_speed = FizLineUtil.get_float(kv, "PlatformMaxSpeed")
    if kv.has("PlatformOpenMethod"):
        node.platform_type = (
                RailVehicleDoors.PLATFORM_TYPE_SHIFT if FizLineUtil.get_string(kv, "PlatformOpenMethod").to_lower() == "shift"
                else RailVehicleDoors.PLATFORM_TYPE_ROTATE)

    if kv.has("MirrorMaxShift"):
        node.mirror_max_shift = FizLineUtil.get_float(kv, "MirrorMaxShift")
    if kv.has("MirrorVelClose"):
        node.mirror_close_velocity = FizLineUtil.get_float(kv, "MirrorVelClose")

    context.add_part("RailVehicleDoors", node)
