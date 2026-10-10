extends "mover_switches_section.gd"

## The revolutions and the oil pressure belong to a diesel, the current to an electric engine; each
## group is shown only for the engine that has it. The pantographs are the power source's
## (mover_switches_power_source).
var _diesel_engine:RailVehicleDieselEngine
var _electric_engine:RailVehicleElectricEngine


func _do_update():
    super._do_update()
    var engine:RailVehicleEngine = _component(VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    _diesel_engine = engine as RailVehicleDieselEngine
    _electric_engine = engine as RailVehicleElectricEngine
    _show_applicable(not engine == null)
    # A native enum is no Dictionary in GDScript - its names come from ClassDB
    var type_name:String = "-"
    if engine:
        for constant:String in ClassDB.class_get_enum_constants(&"RailVehicleEngine", &"EngineType"):
            if ClassDB.class_get_integer_constant(&"RailVehicleEngine", constant) == engine.get_type():
                type_name = constant
    %EngineType.text = tr("Type: %s") % type_name
    %Diesel.visible = not _diesel_engine == null
    %Electric.visible = not _electric_engine == null
    # Traction motors, and with them the overload relay, are on an electric and a diesel-electric
    %FuseReset.visible = not _electric_engine == null or engine is RailVehicleDieselElectricEngine


## The type caption is composed, not a msgid a Label translates itself
func _notification(what:int) -> void:
    if what == NOTIFICATION_TRANSLATION_CHANGED and is_node_ready():
        _do_update()


func _on_refresh_timer_timeout() -> void:
    if _diesel_engine:
        %EngineRPM.value = _diesel_engine.get_rpm()
        %OilPressure.value = _diesel_engine.get_oil_pump_pressure()
    if _electric_engine:
        %EngineCurrent.value = _electric_engine.get_motor_current()
