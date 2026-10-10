extends RefCounted
class_name LegacyCabinUnmodelledControls

## Controls of MmdSemanticCatalog the cab does not model. Most OnCommand_* of the original work
## without their gauge - the keys reach TTrain, which only updates a gauge that may be missing - so
## a cab with no main_on_bt (dynamic/pkp/e186_v2 has one main_sw instead), no dirkey, no
## shp_reset_bt, ... still takes their keys. The ones the catalog marks `requires_gauge` (the horns,
## sanding, ...) refuse the command without it, so such a cab takes neither their key nor their
## wiring. Each of the other controls is registered in CabinSystem
## with the wiring of its catalog entry (LegacyCabinForwardCommands) unless a cabin behaviour has
## registered it already; the cab logic takes the keys of those get_key_control_ids() names.
##
## A control is left out when a control of the cab already takes one of its keys: jointctrl and
## mainctrl share theirs, both would step the controller.

const ACTION_FIELDS:Array[String] = LegacyCabinControls.ACTION_FIELDS
## These take their keys - a knob too: its key holds it while pressed (LegacyCabinLogic), as
## OnCommand_independentbrakeincrease/decrease act with no gauge in the cab
const KEY_WIRED_KINDS:Array[StringName] = [&"button", &"switch", &"knob"]

var _cab_controls:LegacyCabinControls
var _cabin:RID
var _handlers:Dictionary[StringName, Callable] = {}
## The controls whose keys the cab logic takes
var _key_control_ids:Array[StringName] = []


func _init(cab_controls:LegacyCabinControls) -> void:
    _cab_controls = cab_controls


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    var taken_actions:Dictionary[String, bool] = _cab_controls.get_actions()

    for label:String in MmdSemanticCatalog.get_labels():
        var control_id:StringName = StringName(label)
        if _cab_controls.has_control(control_id):
            continue
        var entry:Dictionary = MmdSemanticCatalog.get_entry(label)
        if entry.get("requires_gauge", false):
            continue
        var fields:Dictionary = entry["fixed_fields"]
        var wiring:Dictionary = LegacyCabinForwardCommands.wiring(
                entry["widget_class"], fields, entry.get("target", CabinState.Target.OCCUPIED))
        var actions:Array = ACTION_FIELDS.map(func(field:String) -> String: return fields.get(field, ""))
        actions = actions.filter(func(action:String) -> bool: return not action == "")
        if not actions or actions.any(func(action:String) -> bool: return taken_actions.has(action)):
            continue
        var registered:bool = CabinSystem.has_control(cabin, control_id)
        if not registered and not wiring:
            continue
        for action:String in actions:
            taken_actions[action] = true
        if registered or wiring["kind"] in KEY_WIRED_KINDS:
            _key_control_ids.append(control_id)
        if registered:
            continue
        wiring["control_id"] = control_id
        _handlers[control_id] = LegacyCabinForwardCommands._handle.bind(wiring)
        CabinSystem.register_control(cabin, control_id, _handlers[control_id])


func unregister() -> void:
    for control_id:StringName in _handlers:
        CabinSystem.unregister_control(_cabin, control_id, _handlers[control_id])
    _handlers.clear()
    _key_control_ids.clear()


## The controls whose keys the cab logic takes, in the catalog's order
func get_key_control_ids() -> Array[StringName]:
    return _key_control_ids
