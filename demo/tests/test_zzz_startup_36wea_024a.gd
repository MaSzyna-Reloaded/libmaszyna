extends MaszynaStartupTest

## 36WEa-024A (PKP/IMPULS_V1, fixtures/scenery/startup_36wea-024a.scn - l053_poranek.scn's unit, its
## pantographs by individual switches) started from its cab with the keyboard and moved off; the
## driver walking through the gangways to the rear cab of the C car to drive back (report
## 2026-10-06: the driving aid asked to deactivate the cab just activated, and to raise pantograph
## A, which the C car has not got)

## Simulated seconds the driver is given to update after a step
const DRIVER_UPDATE_SECONDS:float = 5.0
## Cab changes the walk may take: three cabs a car, three cars
const MAX_CAB_CHANGES:int = 9


func test_starts_and_moves_off() -> void:
    await run_startup("startup_36wea-024a.scn", "36WEa-024a", Kind.ELECTRIC_MULTIPLE_UNIT)


## The original's driver is the train's, in the occupied cab (TTrain::CabChange(), Train.cpp:10324;
## MoveToVehicle(), Train.cpp:10928): the unit keeps running, and the cab activated is the right one
## (determine_consist_state(), Driver.cpp:6013-6022)
func test_the_driver_walks_to_the_rear_cab_with_the_player() -> void:
    await _walk_to_the_rear_cab_of_c()
    var player_vehicle:RID = PlayerServer.player_get_vehicle()
    var driver:RID = DriverSystem.vehicle_get_driver(player_vehicle)
    assert_eq(VehicleServer.person_get_cabin(driver), VehicleServer.person_get_cabin(PlayerServer.player_get_person()),
            "the driver sits in the player's cab")
    for car:RID in RailVehicleServer.vehicle_get_coupled(player_vehicle, RailVehicleController.COUPLER_END_FRONT,
            RailVehicleController.COUPLING_FLAG_COUPLER):
        var engine:RailVehicleEngine = _engine(car)
        if engine and _master(car):
            assert_true(engine.get_main_switch_enabled(), "%s's line breaker stays closed" % VehicleServer.vehicle_get_name(car))
    await key_tap(&"cab_activation_toggle")
    await step(ticks(DRIVER_UPDATE_SECONDS))
    assert_eq(_master(player_vehicle).get_cabin(), -1, "the rear cab is the active one")
    assert_false(MaszynaLegacyDriverHints.TEXTS[MaszynaLegacyDriverHints.Hint.CAB_DEACTIVATION] in _hints(),
            "no hint to deactivate the cab just activated: %s" % [_hints()])


## The C car's one pantograph is B (PhysicalLayout=2): from its cab the driving aid asks for B, which
## its switch raises (MASZYNA_ORIGINAL_QUIRKS.md, "Pantograph B of a vehicle with one")
func test_the_c_car_is_asked_for_its_own_pantograph() -> void:
    await _walk_to_the_rear_cab_of_c()
    # the unit's pantographs down: the A car's A and the C car's B
    VehicleServer.vehicle_send_command(occupied, "pantograph_valve_operate",
            RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE)
    VehicleServer.vehicle_send_command(PlayerServer.player_get_vehicle(), "pantograph_valve_operate",
            RailVehicleEnginePowerSource.PANTOGRAPH_SECOND, RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    var hints:Array[String] = _hints()
    assert_false(MaszynaLegacyDriverHints.TEXTS[MaszynaLegacyDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_ON] in hints,
            "no hint to raise pantograph A, which the C car has not got: %s" % [hints])
    assert_true(MaszynaLegacyDriverHints.TEXTS[MaszynaLegacyDriverHints.Hint.REAR_PANTOGRAPH_VALVE_ON] in hints,
            "a hint to raise its pantograph B: %s" % [hints])


## Started in the A car's front cab, stopped, the reverser at neutral; then the cab changes towards
## the back until the player sits in the C car's rear cab
func _walk_to_the_rear_cab_of_c() -> void:
    await run_startup("startup_36wea-024a.scn", "36WEa-024a", Kind.ELECTRIC_MULTIPLE_UNIT)
    while _master(occupied).get_main_position() > 0:
        await key_tap(&"main_controller_decrease")
    await key_tap(&"direction_decrease")
    for _change:int in MAX_CAB_CHANGES:
        var vehicle:RID = PlayerServer.player_get_vehicle()
        if VehicleServer.vehicle_get_name(vehicle) == "36WEa-024c" \
                and RailVehicleServer.vehicle_get_driver_cabin(vehicle) == RailVehicleServer.vehicle_get_rear_cabin(vehicle):
            break
        await key_tap(&"cabin_next")
        await step(ticks(1.0))
    var reached:RID = PlayerServer.player_get_vehicle()
    assert_eq(VehicleServer.vehicle_get_name(reached), "36WEa-024c", "the player reached the C car")
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(reached), RailVehicleServer.vehicle_get_rear_cabin(reached),
            "in its rear cab")
    await step(ticks(DRIVER_UPDATE_SECONDS))


func _hints() -> Array[String]:
    var texts:Array[String] = []
    var driver:RID = DriverSystem.vehicle_get_driver(PlayerServer.player_get_vehicle())
    for hint:Dictionary in DriverSystem.driver_get_state(driver).get("hints", []):
        texts.append(hint["text"])
    return texts
