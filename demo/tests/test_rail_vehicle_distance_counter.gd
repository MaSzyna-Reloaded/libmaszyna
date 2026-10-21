extends MaszynaGutTest

## The cab's distance counter (TTrain::m_distancecounter, add_distance - Train.cpp:1552, 10309), the
## master controller's own: off until started, metres counted towards the occupied cab, and off for
## good when the low voltage goes

## An ELF's head car: a master controller and a battery
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/elf_v1/34we-a.fiz"
## A rolling vehicle's start [km/h]
const VELOCITY:float = 20.0
const COUNTED_SECONDS:float = 1.0
## The counter against the velocity the steps rolled at [m]
const TOLERANCE:float = 0.5

var train:VehicleController


func before_each() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH)
    train = build_vehicle("TestDistanceCounter", description, VELOCITY,
            MaszynaVehicleData.DriverType.DRIVER_HEAD)
    await step(2)
    train.send_command("battery", true)
    await step(2)


func _distance() -> float:
    return float(train.get_state()["distance_counter"])


func test_off_until_started() -> void:
    assert_eq(_distance(), -1.0)


func test_counts_the_metres_rolled_towards_the_occupied_cab() -> void:
    train.send_command("distance_counter_activate", true)
    var rolled:float = 0.0
    for _tick:int in ticks(COUNTED_SECONDS):
        rolled += train.get_velocity() * TICK
        await step(1)
    assert_gt(_distance(), 0.0, "counted forwards from the front cab")
    assert_almost_eq(_distance(), rolled, TOLERANCE)


func test_off_for_good_when_the_low_voltage_goes() -> void:
    train.send_command("distance_counter_activate", true)
    await step(2)
    train.send_command("battery", false)
    await step(2)
    train.send_command("battery", true)
    await step(2)
    assert_eq(_distance(), -1.0, "not counting again by itself")
