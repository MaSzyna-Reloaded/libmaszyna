extends MaszynaGutTest

## Contracts a vehicle's components keep with whoever reads or commands them
## (REQUIRED_CLEANING_BEFORE_MERGE_TO_UPSTREAM.md RC-006, RC-010, RC-014, RC-019).

const MAIN_POSITIONS:int = 4
const BATTERY_VOLTAGE:float = 110.0


## RC-014: a component whose vehicle is not simulated publishes nothing, rather than zeroes
## (VehicleComponent::is_simulation_ready())
func test_a_component_outside_a_simulated_vehicle_publishes_no_state() -> void:
    var components:Array[VehicleComponent] = [
        MoverRailVehicleElectricSeriesEngine.new(), MoverRailVehicleElectricInductionEngine.new(),
        MoverRailVehicleDieselElectricEngine.new(), MoverRailVehicleRadio.new(),
    ]
    for component:VehicleComponent in components:
        assert_eq(component.get_state(), {}, "%s publishes nothing outside a vehicle" % component.get_class())


## RC-010: a controller without its backend - a description, or one released - takes an operation
## and does nothing, as its getters already answer with nothing
func test_a_controller_without_its_backend_does_nothing() -> void:
    var description:MoverRailVehicleController = MoverRailVehicleController.new()
    description.compartment_lights(true)
    description.cab_activation(true)
    description.cab_controls_reset()
    description.cabin_leave()
    description.cabin_enter()
    description.ground_relay_reset()
    description.antislip()
    description.main_controller_increase(1)
    description.direction_increase()
    assert_eq(description.get_direction(), VehicleController.DIRECTION_NEUTRAL, "nothing was done")


## RC-006: a component taken out of the vehicle takes all its commands with it
func test_a_disabled_radio_leaves_no_command_behind() -> void:
    var description:VehicleController = MoverRailVehicleController.new()
    description.add_component(MoverRailVehicleRadio.new())
    var vehicle:RID = build_vehicle("RadioCommandsTest", description).get_rid()
    var radio:RailVehicleRadio = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio
    assert_true(VehicleServer.vehicle_has_command(vehicle, "radio_call1"), "the radio's call is a command")
    radio.enabled = false
    # a component's enabling lands on its vehicle's next step (RC-045)
    await step(1)
    for command:String in ["radio", "radio_call1", "radio_call3"]:
        assert_false(VehicleServer.vehicle_has_command(vehicle, command), "%s gone with the radio" % command)


## RC-019: the master controller's position is the driver's; configuring the universal controller
## again does not put it back
func test_reconfiguring_the_universal_controller_keeps_the_master_controller_where_it_is() -> void:
    var description:VehicleController = MoverRailVehicleController.new()
    description.add_component(build_power_supply(BATTERY_VOLTAGE))
    var master:RailVehicleMasterController = MoverRailVehicleMasterController.new()
    master.main_position_count = MAIN_POSITIONS
    description.add_component(master)
    description.add_component(MoverRailVehicleUniversalController.new())
    var vehicle:RID = build_vehicle("UniversalControllerTest", description, 0.0,
            MaszynaDynamicData.DriverType.DRIVER_HEAD).get_rid()
    master = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    VehicleServer.vehicle_send_command(vehicle, "battery", true)
    await step(1)
    # no cab active, no step of the controller (IncMainCtrl(), FINDINGS.md 09-28)
    VehicleServer.vehicle_send_command(vehicle, "cab_activation", true)
    VehicleServer.vehicle_send_command(vehicle, "main_controller_increase", 1)
    var position:int = master.get_main_position()
    assert_gt(position, 0, "the controller moved")
    var universal:VehicleComponent = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER)
    universal.apply_config()
    assert_eq(master.get_main_position(), position, "the controller stays where the driver put it")
