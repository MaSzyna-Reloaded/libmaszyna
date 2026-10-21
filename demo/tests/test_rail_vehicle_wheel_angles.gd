extends MaszynaGutTest

## The wheel angles, the wheels component's own (TDynamicObject::Update, DynObj.cpp:3776-3792): the
## rolling wheels turn with the velocity, by the arc over their own diameter - a smaller wheel
## turns further - and stand still with the vehicle

const FRONT_DIAMETER:float = 0.8
const REAR_DIAMETER:float = 1.0
## A rolling vehicle's start [km/h]
const VELOCITY:float = 20.0
const TOLERANCE:float = 0.001

var wheels:RailVehicleWheels


func _build(initial_velocity:float) -> void:
    var description:RailVehicleController = MaszynaMoverRailVehicleController.new()
    description.mass = RAIL_VEHICLE_MASS
    var configured:RailVehicleWheels = MaszynaMoverRailVehicleWheels.new()
    configured.front_rolling_wheel_diameter = FRONT_DIAMETER
    configured.rear_rolling_wheel_diameter = REAR_DIAMETER
    description.add_component(configured)
    var train:VehicleController = build_vehicle("TestWheelAngles", description, initial_velocity)
    # the vehicle's own wheels, made from the description
    wheels = train.get_component(VehicleComponentType.COMPONENT_WHEELS) as RailVehicleWheels
    await step(2)


func test_rolling_wheels_turn_by_their_diameters() -> void:
    await _build(VELOCITY)
    var front:float = wheels.get_angle_front_deg()
    var rear:float = wheels.get_angle_rear_deg()
    await step(1)
    var front_turn:float = wheels.get_angle_front_deg() - front
    var rear_turn:float = wheels.get_angle_rear_deg() - rear
    assert_gt(rear_turn, 0.0, "the wheels turn while the vehicle rolls")
    assert_almost_eq(front_turn / rear_turn, REAR_DIAMETER / FRONT_DIAMETER, TOLERANCE,
            "the smaller wheel turns further")


func test_the_wheels_stand_with_the_vehicle() -> void:
    await _build(0.0)
    var front:float = wheels.get_angle_front_deg()
    await step(3)
    assert_eq(wheels.get_angle_front_deg(), front)
