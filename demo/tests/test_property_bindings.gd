extends MaszynaGutTest

const BOUND_CLASSES: Array[StringName] = [
    &"RailVehicleBrakePressureTableItem",
    &"RailVehicleCompressorListItem",
    &"VehicleCurvePointItem",
    &"RailVehicleDimmerListItem",
    &"E3DModel",
    &"E3DSubModel",
    &"RailVehicleLightListItem",
    &"RailVehicleLoadListItem",
    &"RailVehicleMotorParameter",
    &"RailVehicleRelayListItem",
    &"RailVehicleThrottlePositionItem",
    &"RailVehicleAIHints",
    &"RailVehicleBrake",
    &"RailVehicleBuffCoupl",
    &"VehicleController",
    &"RailVehicleDieselElectricEngine",
    &"RailVehicleDieselEngine",
    &"RailVehicleDoors",
    &"RailVehicleElectricEngine",
    &"RailVehicleElectricInductionEngine",
    &"RailVehicleElectricSeriesEngine",
    &"RailVehicleElectroPneumaticDynamicBrake",
    &"RailVehicleEngine",
    &"RailVehicleEnginePowerSource",
    &"RailVehicleHeating",
    &"RailVehicleHorns",
    &"RailVehicleLighting",
    &"RailVehicleLoad",
    &"RailVehicleSecuritySystem",
    &"RailVehicleSpeedControl",
    &"RailVehicleSpringBrake",
    &"RailVehicleSwitches",
    &"RailVehicleUniversalController",
    &"RailVehicleWheels",
    &"RailVehicleWipers",
    &"RailVehicleUniversalControllerListItem",
    &"RailVehicleWWListItem",
    &"RailVehicleWiperListItem",
]


func test_bound_properties_use_canonical_names_and_accessors() -> void:
    for bound_class in BOUND_CLASSES:
        var properties: Array[Dictionary] = ClassDB.class_get_property_list(bound_class, true)
        for property in properties:
            var usage: int = int(property["usage"])
            if bool(usage & PROPERTY_USAGE_GROUP) or bool(usage & PROPERTY_USAGE_SUBGROUP):
                continue

            var property_name: StringName = StringName(property["name"])
            var setter: StringName = ClassDB.class_get_property_setter(bound_class, property_name)
            var getter: StringName = ClassDB.class_get_property_getter(bound_class, property_name)
            if not setter or not getter:
                continue

            assert_false(String(property_name).contains("/"), "%s.%s contains a slash" % [bound_class, property_name])
            assert_eq(
                setter,
                StringName("set_" + String(property_name)),
                "%s.%s has an inconsistent setter" % [bound_class, property_name],
            )
            assert_eq(
                getter,
                StringName("get_" + String(property_name)),
                "%s.%s has an inconsistent getter" % [bound_class, property_name],
            )


## A property is configuration - stored and settable. The live state is read through the typed
## getters and the vehicle's dump (VehicleServer.vehicle_dump_state()), never through a property,
## so a saved vehicle carries no live values.
func test_properties_are_stored_configuration() -> void:
    for bound_class in BOUND_CLASSES:
        for property in ClassDB.class_get_property_list(bound_class, true):
            var usage: int = int(property["usage"])
            if bool(usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP | PROPERTY_USAGE_CATEGORY)):
                continue
            var property_name: StringName = StringName(property["name"])
            assert_true(bool(usage & PROPERTY_USAGE_STORAGE), "%s.%s is not stored" % [bound_class, property_name])
            assert_ne(ClassDB.class_get_property_setter(bound_class, property_name), &"",
                    "%s.%s has no setter" % [bound_class, property_name])


func test_properties_are_available_through_direct_gdscript_access() -> void:
    var brake: RailVehicleBrake = MoverRailVehicleBrake.new()
    brake.brake_force_max = 85.0
    assert_eq(brake.brake_force_max, 85.0)

    var power_source: RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
    power_source.power_cable_source = RailVehicleController.POWER_TYPE_STEAM
    assert_eq(power_source.power_cable_source, RailVehicleController.POWER_TYPE_STEAM)

    var lights: RailVehicleLightListItem = RailVehicleLightListItem.new()
    lights.cabin_a_left_white_signal = false
    lights.cabin_a_right_white_signal = true
    assert_false(lights.cabin_a_left_white_signal)
    assert_true(lights.cabin_a_right_white_signal)


func test_group_paths_do_not_change_public_property_names() -> void:
    var current_group: String = ""
    var current_subgroup: String = ""
    var properties: Array[Dictionary] = ClassDB.class_get_property_list(&"RailVehicleEnginePowerSource", true)
    for property in properties:
        var usage: int = int(property["usage"])
        if bool(usage & PROPERTY_USAGE_GROUP):
            current_group = property["name"]
            current_subgroup = ""
        elif bool(usage & PROPERTY_USAGE_SUBGROUP):
            current_subgroup = property["name"]
        elif property["name"] == &"power_cable_source":
            assert_eq(current_group, "Power Cable")
            assert_eq(current_subgroup, "")
            return

    fail_test("power_cable_source was not found")


## Authored configuration has to survive into the built vehicle. It used to be authored as a
## scene of component nodes; a vehicle is described by its controller and components now, stored
## as they are - and this asserts the same thing through that description.
func test_authored_configuration_reaches_the_built_vehicle() -> void:
    var brake: RailVehicleBrake = MoverRailVehicleBrake.new()
    brake.valve_type = 20
    brake.brake_force_max = 85.0
    brake.compressor_cab_a_min_pressure = 7.0
    var engine: RailVehicleDieselEngine = MoverRailVehicleDieselElectricEngine.new()
    engine.oil_pump_pressure_minimum = 0.15
    var security: RailVehicleSecuritySystem = MoverRailVehicleSecuritySystem.new()
    security.aware_system_active = true
    security.emergency_brake_delay = 2.5

    var description:RailVehicleController = MoverRailVehicleController.new()
    description.vehicle_id = "PropertyBindingsTest"
    description.mass = 74000.0
    var components: Array[VehicleComponent] = [brake, engine, security]
    description.components = components

    var vehicle := RailVehiclePhysicsNode.new()
    add_child_autofree(vehicle)
    vehicle.set_controller(description)

    var train: VehicleController = VehicleServer.vehicle_get_controller(vehicle.get_vehicle_rid())
    brake = train.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES)
    engine = train.get_component(VehicleComponentType.COMPONENT_ENGINE)
    var security_system: RailVehicleSecuritySystem = train.get_rail_component(RailVehicleComponentType.COMPONENT_SECURITY)

    assert_ne(brake, components[0], "the vehicle is built from a copy - the description stays shared")
    assert_eq(train.mass, 74000.0, "the vehicle's own properties too")
    assert_eq(brake.valve_type, 20)
    assert_eq(brake.brake_force_max, 85.0)
    assert_eq(brake.compressor_cab_a_min_pressure, 7.0)
    assert_almost_eq(engine.oil_pump_pressure_minimum, 0.15, 0.000001)
    assert_true(security_system.aware_system_active)
    assert_eq(security_system.emergency_brake_delay, 2.5)
