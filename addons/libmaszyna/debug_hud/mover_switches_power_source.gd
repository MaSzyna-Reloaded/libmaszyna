extends "mover_switches_section.gd"

## What the vehicle's traction is fed from (RailVehicleEnginePowerSource): its kind and, for a
## current collector, the pantographs - their valves, tank and compressor, and the voltage they
## bring. The HUD hands it the vehicle carrying the pantographs (CabinState.Target.PANTOGRAPH_UNIT),
## which in a unit may be a car without an engine of its own.
var _power_source:RailVehicleEnginePowerSource


func _do_update():
    super._do_update()
    _power_source = _rail_component(RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    _show_applicable(not _power_source == null)
    # A native enum is no Dictionary in GDScript - its names come from ClassDB
    var source_name:String = "-"
    if _power_source:
        for constant:String in ClassDB.class_get_enum_constants(&"RailVehicleController", &"TrainPowerSource"):
            if ClassDB.class_get_integer_constant(&"RailVehicleController", constant) == _power_source.source_type:
                source_name = constant
    %SourceType.text = tr("Source: %s") % source_name
    %Collector.visible = not _power_source == null \
            and _power_source.source_type == RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR


## The source caption is composed, not a msgid a Label translates itself
func _notification(what:int) -> void:
    if what == NOTIFICATION_TRANSLATION_CHANGED and is_node_ready():
        _do_update()


func _on_refresh_timer_timeout() -> void:
    if _power_source:
        %PantographTankPressure.value = _power_source.get_collector_pantograph_tank_pressure()
        %CollectorVoltage.value = _power_source.get_collector_voltage()
        %PantographFirstVoltage.value = _power_source.get_collector_pantograph_first_voltage()
        %PantographSecondVoltage.value = _power_source.get_collector_pantograph_second_voltage()
        %TrainsetHighVoltage.value = _power_source.get_collector_trainset_high_voltage()
