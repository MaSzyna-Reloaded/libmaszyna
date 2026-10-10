@tool
extends RefCounted
class_name FizTrainControllerParser

## Param. and Dimensions: sections, plus the VehicleController-relevant subset of Cntrl.
## (see fiz_train_cntrl_parser.gd, which delegates the controller-relevant keys here via
## apply_cntrl()). Configures `context.controller` directly rather than creating a new part.
## LoadFIZ_Param: Mover.cpp:10244, LoadFIZ_Dimensions: Mover.cpp:10361.
##
## Setters are only called when the corresponding FIZ key is present - VehicleController's own
## compiled-in property defaults already match the FIZ format's "key absent" behavior except
## where noted (Cx, Floor, GroundRelayStart), so there is no need to re-specify them here.

const _CATEGORY_MAP := {
    "train": VehicleController.CATEGORY_TRAIN,
    "road": VehicleController.CATEGORY_ROAD,
    "unimog": VehicleController.CATEGORY_ROAD,
    "ship": VehicleController.CATEGORY_SHIP,
    "airplane": VehicleController.CATEGORY_AIRPLANE,
}

const _TRAIN_TYPE_MAP := {
    "pseudodiesel": RailVehicleController.TRAIN_TYPE_PSEUDODIESEL,
    "ezt": RailVehicleController.TRAIN_TYPE_EZT,
    "dmu": RailVehicleController.TRAIN_TYPE_DMU,
    "sn61": RailVehicleController.TRAIN_TYPE_SN61,
    "et22": RailVehicleController.TRAIN_TYPE_ET22,
    "et40": RailVehicleController.TRAIN_TYPE_ET40,
    "et41": RailVehicleController.TRAIN_TYPE_ET41,
    "et42": RailVehicleController.TRAIN_TYPE_ET42,
    "ep05": RailVehicleController.TRAIN_TYPE_EP05,
    "181": RailVehicleController.TRAIN_TYPE_181,
    # "182" has no corresponding dt_* constant in this port's MOVER.h - left unmapped.
}


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    if kv.has("M") or kv.has("PWR") or kv.has("Vmax") or kv.has("Category") or kv.has("Type"):
        _parse_param(kv, context)
    if kv.has("L") or kv.has("H") or kv.has("W") or kv.has("Cx") or kv.has("Floor"):
        _parse_dimensions(kv, context)


func _parse_param(kv: Dictionary, context: FizImportContext) -> void:
    var controller: VehicleController = context.controller
    if kv.has("M"):
        controller.mass = FizLineUtil.get_float(kv, "M")
    if kv.has("Mred"):
        controller.reduced_mass = FizLineUtil.get_float(kv, "Mred")
    if kv.has("Vmax"):
        controller.max_velocity = FizLineUtil.get_float(kv, "Vmax")
    if kv.has("PWR"):
        controller.power = FizLineUtil.get_float(kv, "PWR")
    if kv.has("SandCap"):
        controller.sand_capacity = FizLineUtil.get_float(kv, "SandCap")
    if kv.has("HeatingP"):
        controller.heating_power = FizLineUtil.get_float(kv, "HeatingP")
    if kv.has("LightP"):
        controller.light_power = FizLineUtil.get_float(kv, "LightP")

    if kv.has("Category"):
        var category_str: String = FizLineUtil.get_string(kv, "Category").to_lower()
        if _CATEGORY_MAP.has(category_str):
            controller.category = _CATEGORY_MAP[category_str]
        else:
            push_warning("FIZ Param.Category: unknown value '%s'" % category_str)

    # Type= has no "absent" fallback distinct from the compiled default (dt_Default == 0),
    # but context.train_type must always be set (later sections' defaults depend on it) even
    # when Type= wasn't in this line.
    var train_type: int = _TRAIN_TYPE_MAP.get(FizLineUtil.get_string(kv, "Type").to_lower(), RailVehicleController.TRAIN_TYPE_DEFAULT)
    if kv.has("Type"):
        controller.train_type = train_type
    context.train_type = train_type


func _parse_dimensions(kv: Dictionary, context: FizImportContext) -> void:
    var controller: VehicleController = context.controller
    if kv.has("L"):
        controller.dimensions_length = FizLineUtil.get_float(kv, "L")
    var height: float = FizLineUtil.get_float(kv, "H")
    if kv.has("H"):
        controller.dimensions_height = height
    if kv.has("W"):
        controller.dimensions_width = FizLineUtil.get_float(kv, "W")
    # Cx's FIZ-format default (0.3) differs from VehicleController's compiled default (0.0).
    controller.dimensions_drag_coefficient = FizLineUtil.get_float(kv, "Cx", 0.3)

    # Floor's default is conditional on H, which differs from the compiled default (0.96).
    var floor_height: float = height if height <= 2.0 else 0.0
    controller.dimensions_floor_height = FizLineUtil.get_float(kv, "Floor", floor_height)


## Called by FizTrainCntrlParser with the full Cntrl. key/value set - applies only the
## VehicleController-relevant subset (Mover.cpp:10707 LoadFIZ_Cntrl, general subsection) - the
## battery and converter keys are RailVehiclePowerSupply's (FizTrainPowerSupplyParser).
func apply_cntrl(kv: Dictionary, context: FizImportContext) -> void:
    var controller: VehicleController = context.controller
    if kv.has("AutomaticCabActivation"):
        controller.cntrl_automatic_cab_activation = FizLineUtil.get_bool(kv, "AutomaticCabActivation")

    # GroundRelayStart's default depends on TrainType (EZT), which differs from the compiled
    # default - so this one is always applied, even when the key is absent.
    var ground_relay_default: int = (
            RailVehicleController.START_MODE_AUTOMATIC if context.train_type == RailVehicleController.TRAIN_TYPE_EZT
            else RailVehicleController.START_MODE_MANUAL)
    controller.cntrl_ground_relay_start_mode = parse_start_mode(FizLineUtil.get_string(kv, "GroundRelayStart"), ground_relay_default)

    if kv.has("CompartmentLightsStart"):
        controller.cntrl_compartment_lights_start_mode = parse_start_mode(FizLineUtil.get_string(kv, "CompartmentLightsStart"), RailVehicleController.START_MODE_AUTOMATIC)
    if kv.has("InactiveCabFlag"):
        controller.cntrl_inactive_cab_flag = FizLineUtil.get_int(kv, "InactiveCabFlag")


## Shared `...Start=` device activation mode decode (RailVehicleController.StartMode). Used by
## Cntrl., Engine:, and other sections.
static func parse_start_mode(value: String, default_value: int) -> int:
    if not value:
        return default_value
    match value.to_lower():
        "disabled": return RailVehicleController.START_MODE_DISABLED
        "manual": return RailVehicleController.START_MODE_MANUAL
        "automatic": return RailVehicleController.START_MODE_AUTOMATIC
        "mixed": return RailVehicleController.START_MODE_MANUAL_WITH_AUTO_FALLBACK
        "battery": return RailVehicleController.START_MODE_BATTERY
        "converter": return RailVehicleController.START_MODE_CONVERTER
        "direction": return RailVehicleController.START_MODE_DIRECTION
        _: return default_value


## Power-source decode (RailVehicleController.TrainPowerSource - the enum this class owns).
## LoadFIZ_SourceDecode: Mover.cpp:11677. Used by Light:/Clima:/Power:.
static func parse_power_source(value: String, default_value: int = RailVehicleController.POWER_SOURCE_NOT_DEFINED) -> int:
    if not value:
        return default_value
    match value.to_lower():
        "transducer": return RailVehicleController.POWER_SOURCE_TRANSDUCER
        "generator": return RailVehicleController.POWER_SOURCE_GENERATOR
        "accu", "accumulator": return RailVehicleController.POWER_SOURCE_ACCUMULATOR
        "currentcollector": return RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
        "powercable": return RailVehicleController.POWER_SOURCE_POWERCABLE
        "heater": return RailVehicleController.POWER_SOURCE_HEATER
        "internal": return RailVehicleController.POWER_SOURCE_INTERNAL
        "main": return RailVehicleController.POWER_SOURCE_MAIN
        _: return RailVehicleController.POWER_SOURCE_NOT_DEFINED


## Power-type decode (RailVehicleController.TrainPowerType - the enum this class owns).
## LoadFIZ_PowerDecode: Mover.cpp:11668.
static func parse_power_type(value: String, default_value: int = RailVehicleController.POWER_TYPE_NONE) -> int:
    if not value:
        return default_value
    match value.to_lower():
        "biopower": return RailVehicleController.POWER_TYPE_BIO
        "mechpower": return RailVehicleController.POWER_TYPE_MECH
        "electricpower": return RailVehicleController.POWER_TYPE_ELECTRIC
        "steampower": return RailVehicleController.POWER_TYPE_STEAM
        _: return RailVehicleController.POWER_TYPE_NONE
