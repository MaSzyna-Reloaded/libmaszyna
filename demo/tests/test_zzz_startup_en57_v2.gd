extends MaszynaStartupTest

## EN57-636ra (PKP/EN57_V2, fixtures/scenery/startup_en57-636ra.scn) started from its cab with the keyboard
## and moved off; its trailer cars brake with the load-weighing valve's empty-car pressure, and its
## FVel6 handle takes a whole position per key press

## The train brake handle's normalized setting for FVel6's position 3, the EP brake applied
## (pos_table, hamulce.cpp:34: -1 to 6)
const EP_BRAKE_LEVEL:float = 4.0 / 7.0
## Simulated seconds the cylinders are given to fill
const BRAKE_FILL_SECONDS:float = 10.0
## [bar]
const PRESSURE_TOLERANCE:float = 0.1
## [handle position]
const HANDLE_TOLERANCE:float = 0.001
## Cab changes a walk may take: three cabs a car, three cars
const MAX_CAB_CHANGES:int = 9
## Simulated seconds a cab change is given
const CAB_CHANGE_SECONDS:float = 1.0


func test_starts_and_moves_off() -> void:
    await run_startup("startup_en57-636ra.scn", "EN57-636ra", Kind.ELECTRIC_MULTIPLE_UNIT)


## MaxBPMass=52 with TareMaxBP=2.5: an empty 34 t car's cylinders stop at the empty-car pressure, not
## at MaxBP=4 (MBPM, Mover.cpp:10771; TEStEP2::PLC(), hamulce.cpp:1264) - at 4 bar its wheels locked
func test_empty_trailer_brakes_at_its_empty_car_pressure() -> void:
    await run_startup("startup_en57-636ra.scn", "EN57-636ra", Kind.ELECTRIC_MULTIPLE_UNIT)
    VehicleServer.vehicle_send_command(occupied, "brake_level_set", EP_BRAKE_LEVEL)
    await step(ticks(BRAKE_FILL_SECONDS))
    var brake:RailVehicleBrake = _brake(occupied)
    assert_eq(brake.cntrl_max_brake_pressure_mass, 52.0, "MaxBPMass read from the FIZ")
    assert_almost_eq(brake.get_air_pressure(), brake.max_tare_pressure, PRESSURE_TOLERANCE,
            "the cab car's cylinders at the empty-car pressure, below MaxBP %.1f" % brake.max_cylinder_pressure)


## A key press moves an FVel6 a position - the EP brake applies from position 1 (TFVel6::GetPF(),
## hamulce.cpp:4310) - where an FV4a moves while the key is held (Train.cpp:1960-1966)
func test_a_key_press_steps_the_brake_handle_a_position() -> void:
    await run_startup("startup_en57-636ra.scn", "EN57-636ra", Kind.ELECTRIC_MULTIPLE_UNIT)
    var brake:RailVehicleBrake = _brake(occupied)
    assert_eq(brake.handle_movement, RailVehicleBrake.BRAKE_HANDLE_MOVEMENT_STEPPED, "an FVel6 steps")
    var before:float = brake.get_controller_position()
    await key_tap(&"brake_level_increase")
    assert_almost_eq(brake.get_controller_position(), before + brake.handle_step, HANDLE_TOLERANCE,
            "one press, one position")



## The driver walks to the other end and back (report 2026-10-06): every cab activated on the way
## sends its direction to the unit (CabActivisation(), Mover.cpp:2907) - the machine room's is 0,
## Sign(0) sending it back (utilities.h:48) - so the reverser is at neutral again and the master
## controller of an EMU refuses (IncMainCtrl(), Mover.cpp:2390) until it is set: as the original
func test_the_controller_works_once_the_reverser_is_set_after_a_walk() -> void:
    await run_startup("startup_en57-636ra.scn", "EN57-636ra", Kind.ELECTRIC_MULTIPLE_UNIT)
    var master:RailVehicleMasterController = _master(powered)
    while master.get_main_position() > 0:
        await key_tap(&"main_controller_decrease")
    await _walk_to_cab(&"cabin_next", "EN57-636rb", true)
    await _walk_to_cab(&"cabin_previous", "EN57-636ra", false)
    assert_eq(VehicleServer.vehicle_get_controller(powered).get_direction(), 0, "the reverser at neutral after the walk")
    await key_tap(&"main_controller_increase")
    assert_eq(master.get_main_position(), 0, "the master controller refuses with no direction")
    await _direction_forward()
    await key_tap(&"main_controller_increase")
    assert_gt(master.get_main_position(), 0, "the master controller moves once the reverser is set")


## Cab changes by `action` until the player sits in `vehicle_name`'s rear or front cab
func _walk_to_cab(action:StringName, vehicle_name:String, rear:bool) -> void:
    for _change:int in MAX_CAB_CHANGES:
        var vehicle:RID = PlayerServer.player_get_vehicle()
        var wanted:RID = RailVehicleServer.vehicle_get_rear_cabin(vehicle) if rear \
                else RailVehicleServer.vehicle_get_front_cabin(vehicle)
        if VehicleServer.vehicle_get_name(vehicle) == vehicle_name and RailVehicleServer.vehicle_get_driver_cabin(vehicle) == wanted:
            return
        await key_tap(action)
        await step(ticks(CAB_CHANGE_SECONDS))
    fail_test("the player did not reach %s" % vehicle_name)
