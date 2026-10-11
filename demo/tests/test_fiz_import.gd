extends MaszynaGutTest

const FIXTURE_PATH := "res://tests/fixtures/test_vehicle.fiz"

var vehicle: VehiclePhysicsNode
var controller: VehicleController


## A .fiz is a vehicle's description (a VehicleController with its components) now; a VehiclePhysicsNode is what builds a vehicle from one, and the
## controller it owns is not a node - it lives and dies with the vehicle.
func before_each():
    vehicle = RailVehiclePhysicsNode.new()
    add_child(vehicle)
    vehicle.set_controller(FizVehicleBuilder.build_description_at(FIXTURE_PATH))
    controller = VehicleServer.vehicle_get_controller(vehicle.get_vehicle_rid())
    await wait_idle_frames(2)


## The description is data: the vehicle is built from a copy of it, and the description itself is
## never a vehicle - no component joined to it, no command registered on it.
func test_the_description_is_not_a_vehicle() -> void:
    var description: VehicleController = FizVehicleBuilder.build_description_at(FIXTURE_PATH)
    assert_true(description.components.size() > 0, "it carries its components")
    for component: VehicleComponent in description.components:
        assert_null(component.get_controller(), "%s joined no vehicle" % component.get_class())
    assert_eq(description.get_commands(), PackedStringArray(), "and registers no command")
    assert_ne(VehicleServer.vehicle_get_controller(vehicle.get_vehicle_rid()), description, "the vehicle is a copy")


func after_each():
    controller = null
    remove_child(vehicle)
    vehicle.free()


func test_param_and_dimensions():
    assert_eq(controller.mass, 74000.0)
    assert_eq(controller.reduced_mass, 2000.0)
    assert_eq(controller.max_velocity, 90.0)
    assert_eq(controller.power, 590.0)
    assert_eq(controller.category, VehicleController.CATEGORY_TRAIN)
    assert_eq(controller.train_type, RailVehicleController.TRAIN_TYPE_DEFAULT)
    assert_eq(controller.dimensions_length, 16.6)
    assert_eq(controller.dimensions_height, 4.28)
    assert_eq(controller.dimensions_width, 3.07)
    assert_eq(controller.dimensions_drag_coefficient, 0.5)


func test_cntrl_general_subset():
    assert_true(controller.cntrl_automatic_cab_activation)
    assert_eq(controller.cntrl_ground_relay_start_mode, RailVehicleController.START_MODE_MANUAL)
    var power_supply: RailVehiclePowerSupply = controller.get_rail_component(
            RailVehicleComponentType.COMPONENT_POWER_SUPPLY)
    assert_not_null(power_supply, "Cntrl.'s battery and converter keys describe a power supply")
    assert_eq(power_supply.cntrl_battery_start_mode, RailVehicleController.START_MODE_MANUAL)
    assert_eq(power_supply.cntrl_converter_start_mode, RailVehicleController.START_MODE_AUTOMATIC)
    assert_eq(power_supply.cntrl_converter_start_delay, 10.0)
    assert_eq(power_supply.battery_voltage, 0.0, "no Light: LMaxVoltage - a battery without voltage")


## The battery's voltage is Light: LMaxVoltage (LoadFIZ_Light, Mover.cpp:11035) - without it the
## low voltage never comes and nothing of the vehicle starts
func test_the_battery_voltage_comes_from_light() -> void:
    var description: VehicleController = FizVehicleBuilder.build_description_at(
            "res://tests/fixtures/dynamic/pkp/303e_v1/303e-ep.fiz")
    var power_supply: RailVehiclePowerSupply = description.get_rail_component(
            RailVehicleComponentType.COMPONENT_POWER_SUPPLY)
    assert_not_null(power_supply, "the EP07's Light: describes its battery")
    if not power_supply:
        return
    assert_eq(power_supply.battery_voltage, 110.0, "LMaxVoltage=110")


## A vehicle with a cab - a master controller - has the cab's horns and radio, and fills the state
## of every one of them
func test_a_vehicle_with_a_cab_has_its_components() -> void:
    assert_not_null(controller.get_rail_component(RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER))
    assert_not_null(controller.get_component(VehicleComponentType.COMPONENT_HORNS))
    assert_not_null(controller.get_component(VehicleComponentType.COMPONENT_RADIO))
    var state: Dictionary = controller.get_state()
    for key: String in ["controller_main_position", "cabin", "tachometer_speed", "distance_counter",
            "horn_low_active", "radio_enabled", "radio_stop_active", "battery_enabled"]:
        assert_true(state.has(key), "the dump has %s" % key)


## A wagon's FIZ describes no master controller - its Cntrl. carries the brake keys and the MCPN=1
## real wagons write - no engine, so neither they nor their state are there
func test_a_wagon_has_no_cab_and_no_engine() -> void:
    var wagon := RailVehiclePhysicsNode.new()
    add_child_autofree(wagon)
    wagon.set_controller(FizVehicleBuilder.build_description_at("res://tests/fixtures/test_wagon.fiz"))
    var wagon_controller: VehicleController = VehicleServer.vehicle_get_controller(wagon.get_vehicle_rid())
    await wait_idle_frames(2)
    assert_null(wagon_controller.get_rail_component(RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER))
    assert_null(wagon_controller.get_component(VehicleComponentType.COMPONENT_HORNS))
    assert_null(wagon_controller.get_component(VehicleComponentType.COMPONENT_RADIO))
    assert_null(wagon_controller.get_component(VehicleComponentType.COMPONENT_ENGINE))
    var state: Dictionary = wagon_controller.get_state()
    assert_true(state.has("mass_total"), "the wagon publishes its own state")
    for key: String in ["controller_main_position", "master_controller_position", "cabin",
            "tachometer_speed", "horn_low_active", "radio_enabled", "radio_stop_active",
            "relay_novolt", "circuit_rlist_size", "current0"]:
        assert_false(state.has(key), "and no %s" % key)


func test_wheels():
    var wheels: RailVehicleWheels = controller.get_component(VehicleComponentType.COMPONENT_WHEELS)
    assert_not_null(wheels)
    assert_eq(wheels.powered_wheel_diameter, 1.1)
    assert_eq(wheels.front_rolling_wheel_diameter, 1.1) # defaults to powered diameter
    assert_eq(wheels.track_width, 1.435)
    assert_eq(wheels.axle_arrangement, "Bo'Bo'")
    assert_eq(wheels.bogie_axle_spacing, 2.6)
    assert_eq(wheels.bogie_pivot_spacing, 7.524)


func test_brake_and_bpt_table():
    var brake: RailVehicleBrake = controller.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES)
    assert_not_null(brake)
    assert_eq(brake.brake_force_max, 250.0)
    assert_eq(brake.max_cylinder_pressure, 3.8)
    assert_eq(brake.cylinder_count, 4)
    assert_eq(brake.rig_effectiveness, 0.85)
    assert_eq(brake.valve_type, RailVehicleBrake.BRAKE_VALVE_W_LU_L)
    assert_eq(brake.cntrl_brake_system, RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC)
    assert_eq(brake.cntrl_brake_ctrl_position_count, 6)
    assert_eq(brake.cntrl_brake_delay_1, 15.0)
    assert_eq(brake.cntrl_brake_handle_type, RailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A)
    assert_true(brake.cntrl_manual_brake_present)

    var bpt: Array = brake.brake_pressure_table
    assert_eq(bpt.size(), 3)
    var row0: RailVehicleBrakePressureTableItem = bpt[0]
    assert_eq(row0.handle_position, -1)
    assert_eq(row0.pipe_pressure, 0.0)
    assert_eq(row0.brake_cylinder_pressure, -1.0)
    var row2: RailVehicleBrakePressureTableItem = bpt[2]
    assert_eq(row2.handle_position, 3)
    assert_eq(row2.pipe_pressure, 3.5)


func test_doors():
    var doors: RailVehicleDoors = controller.get_component(VehicleComponentType.COMPONENT_DOORS)
    assert_not_null(doors)
    assert_eq(doors.open_time, 3.0)
    assert_eq(doors.max_shift, 3.0) # DoorMaxShiftR
    assert_eq(doors.type, RailVehicleDoors.TYPE_ROTATE)
    assert_eq(doors.voltage, RailVehicleDoors.VOLTAGE_24)


func test_buff_coupl():
    var coupler: RailVehicleBuffCoupl = controller.get_rail_component(RailVehicleComponentType.COMPONENT_BUFFERS)
    assert_not_null(coupler)
    assert_eq(coupler.coupler_type, RailVehicleBuffCoupl.COUPLER_TYPE_SCREW)
    assert_eq(coupler.coupler_stiffness_k, 2.5) # kC in kN/m, converted to N/m by RailVehicleBuffCoupl
    assert_eq(coupler.coupler_max_tension_tolerance, 1000.0) # FmaxC in kN
    assert_eq(coupler.buffer_location, RailVehicleBuffCoupl.BUFFER_LOCATION_BOTH)
    assert_eq(coupler.allowed_flag, 63)
    # no PowerFlag in the fixture: 24V and 110V pass, as TCoupling::PowerFlag (MOVER.h:1215)
    assert_eq(coupler.power_flag, RailVehicleController.COUPLING_FLAG_POWER_24V | RailVehicleController.COUPLING_FLAG_POWER_110V)



## WiperList: as e186_v2 writes it - bounded by Size= and closed by "endL" instead of "endwl"
func test_wiper_list_reaches_the_vehicle():
    assert_eq(controller.get_config().get("wipers_switch_position_max", -1), 3)
    controller.send_command("wipers_switch_increase")
    assert_eq(controller.get_state().get("wipers_switch_position", -1), 1)


# LoadFIZ_LightsList / readLightsList (Mover.cpp:11531, 8558): each row is the light bits of cabin
# A's end and cabin B's (enum light, MOVER.h:189)
func test_lights_list():
    var lighting: RailVehicleLighting = controller.get_component(VehicleComponentType.COMPONENT_LIGHTING)
    assert_true(lighting.lights_wrap_selector)
    assert_eq(lighting.lights_default_selector_position, 2)
    assert_eq(lighting.lights_list.size(), 2)
    var first: RailVehicleLightListItem = lighting.lights_list[0]
    assert_true(first.cabin_a_head_light, "4 - the upper headlight")
    assert_true(first.cabin_b_left_red_signal, "34 - both red markers")
    assert_true(first.cabin_b_right_red_signal)
    assert_false(first.cabin_b_end_signals)
    var second: RailVehicleLightListItem = lighting.lights_list[1]
    assert_true(second.cabin_a_left_white_signal, "17 - both lower headlights")
    assert_true(second.cabin_a_right_white_signal)
    assert_true(second.cabin_b_end_signals, "64 - the end-of-train plates")


## BuffCoupl1./BuffCoupl2. are two components, one per end - both reach the vehicle (the model
## once kept one per type and left the rear coupler at the Mover's 1000 N default)
func test_two_coupler_sections_reach_both_ends() -> void:
    var two_couplers := RailVehiclePhysicsNode.new()
    add_child_autofree(two_couplers)
    two_couplers.set_controller(FizVehicleBuilder.build_description_at("res://tests/fixtures/test_vehicle_two_couplers.fiz"))
    await wait_idle_frames(2)

    var couplers:Array = VehicleServer.vehicle_get_controller(two_couplers.get_vehicle_rid()).find_rail_components(RailVehicleComponentType.COMPONENT_BUFFERS)
    assert_eq(couplers.size(), 2)
    var locations:Array = couplers.map(func(c: RailVehicleBuffCoupl) -> int: return c.buffer_location)
    assert_has(locations, RailVehicleBuffCoupl.BUFFER_LOCATION_FRONT)
    assert_has(locations, RailVehicleBuffCoupl.BUFFER_LOCATION_BACK)



## A bare coupler is sized by the engine's Ftmax (LoadFIZ_BuffCoupl, Mover.cpp:10663-10672). Its
## BuffCoupl. comes before Engine:, as in the game's data - the second configuration pass reads the
## engine whatever the order. Doors at 110 V and the speed recorder's dial reach the vehicle too.
func test_bare_coupler_sized_by_the_engine_after_it() -> void:
    var bare := RailVehiclePhysicsNode.new()
    add_child_autofree(bare)
    bare.set_controller(FizVehicleBuilder.build_description_at("res://tests/fixtures/test_vehicle_bare_coupler.fiz"))
    await wait_idle_frames(2)
    var built: VehicleController = VehicleServer.vehicle_get_controller(bare.get_vehicle_rid())

    var coupler: RailVehicleBuffCoupl = built.get_rail_component(RailVehicleComponentType.COMPONENT_BUFFERS)
    # FmaxC = 100 * Mass + 2 * Ftmax, both ends of a BuffCoupl. entry
    var expected_force: float = 100.0 * 74000.0 + 2.0 * 392000.0
    assert_almost_eq(coupler.get_coupler_max_force(RailVehicleController.COUPLER_END_FRONT), expected_force, 0.001)
    assert_almost_eq(coupler.get_coupler_max_force(RailVehicleController.COUPLER_END_REAR), expected_force, 0.001)

    var doors: RailVehicleDoors = built.get_component(VehicleComponentType.COMPONENT_DOORS)
    assert_eq(doors.voltage, RailVehicleDoors.VOLTAGE_110)
    var master_controller: RailVehicleMasterController = built.get_rail_component(RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER)
    assert_eq(master_controller.tachometer_max_speed, 150.0)
