extends "mover_switches_section.gd"

## The load of the vehicle and its exchange at a platform: what it carries, what is still to get off
## and on, and how long that takes; the buttons let a group off or on at both sides
## (RailVehicleServer.load_remove()/load_add())

## The group the buttons let off or on
const LOAD_STEP:float = 10.0

var _load:RailVehicleLoad


func _ready() -> void:
    super._ready()
    %Unload.text = tr("Unload %d") % LOAD_STEP
    %Load.text = tr("Load %d") % LOAD_STEP


func _do_update():
    super._do_update()
    _load = _component(VehicleComponentType.COMPONENT_LOAD) as RailVehicleLoad
    _show_applicable(not _load == null)


func _on_refresh_timer_timeout() -> void:
    if not _load:
        return
    var state:Dictionary = VehicleServer.vehicle_dump_state(target_vehicle)
    %LoadName.text = _load.get_load_name() if _load.get_load_name() else tr("Empty")
    %LoadAmount.text = "%d / %d" % [_load.get_load_amount(), _load.max_load]
    %Unloading.text = "%d" % state.get("load_exchange_unload", 0.0)
    %Loading.text = "%d" % state.get("load_exchange_load", 0.0)
    %ExchangeTime.text = "%d s" % ceili(_load.get_load_exchange_time())


func _on_unload_pressed() -> void:
    RailVehicleServer.load_remove(target_vehicle, LOAD_STEP, RailVehicleLoad.PLATFORM_SIDE_BOTH)


func _on_load_pressed() -> void:
    RailVehicleServer.load_add(target_vehicle, LOAD_STEP, RailVehicleLoad.PLATFORM_SIDE_BOTH)
