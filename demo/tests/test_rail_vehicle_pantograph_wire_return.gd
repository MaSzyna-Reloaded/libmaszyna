extends MaszynaStartupTest

## A pantograph that lost the wire and touches it again reads its voltage drawing nothing on that
## step, and draws from the next on: the original asks the wire with no current while the
## pantograph's last voltage is 0 ("HACK: ... without blowing up the supply", DynObj.cpp:3870-3874),
## and draws at the collector's voltage (PantographVoltage, DynObj.cpp:3200-3237). Drawn at a voltage
## of 0 instead, the vehicle is a short circuit to the substation, which trips.
## 104e-039 is started and moved off on the start-up line; its pantographs are let down for a moment
## shorter than a loss the line breaker holds over, and raised again.

## The two pantographs of the locomotive
const PANTOGRAPHS:Array[RailVehicleEnginePowerSource.PantographSelector] = [
    RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, RailVehicleEnginePowerSource.PANTOGRAPH_SECOND]
## Where the span over the vehicle is looked for: under it, across the track [m]
const SEARCH_HEIGHT:float = 4.0
const SEARCH_WIDTH:float = 2.0
## Steps with the pantographs let down: shorter than the 0.2 s a loss is held over (DynObj.cpp:3201-3208)
const DOWN_STEPS:int = 2
## Long enough for the pantographs to reach the wire again, and the time the substation is watched
## after [s]
const AFTER_SECONDS:float = 5.0


func test_touching_the_wire_again_does_not_overload_its_substation() -> void:
    await run_startup("startup_ep09_v1.scn", "104e-039", Kind.ELECTRIC_LOCOMOTIVE)
    if not is_passing():
        return
    var engine:RailVehicleEngine = _engine(powered)
    assert_ne(engine.get_current0(), 0.0, "moving off, it draws current")
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            carrier, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    for pantograph:RailVehicleEnginePowerSource.PantographSelector in PANTOGRAPHS:
        VehicleServer.vehicle_send_command(carrier, "pantograph", pantograph, false)
    if not await wait_simulated_until(func() -> bool: return is_zero_approx(_wire_voltage(power_source)),
            AFTER_SECONDS, "the pantographs off the wire"):
        return
    await step(DOWN_STEPS)
    var span:RID = _span_over(carrier)
    for pantograph:RailVehicleEnginePowerSource.PantographSelector in PANTOGRAPHS:
        VehicleServer.vehicle_send_command(carrier, "pantograph", pantograph, true)
    # from the moment they go up: a tripped substation comes back after its fuse's timeout
    var lowest:float = INF
    for _tick:int in ticks(AFTER_SECONDS):
        lowest = minf(lowest, TractionServer.wire_get_voltage(span, 0.0, 0.0))
        await step(1)

    assert_gt(_wire_voltage(power_source), 0.0, "the pantographs on the wire again")
    assert_gt(lowest, 0.0, "the substation fed the wire all along")


## The highest voltage the pantographs read
func _wire_voltage(power_source:RailVehicleEnginePowerSource) -> float:
    return maxf(absf(power_source.get_collector_pantograph_first_voltage()),
            absf(power_source.get_collector_pantograph_second_voltage()))


## The span over the vehicle
func _span_over(vehicle:RID) -> RID:
    var position:Vector3 = VehicleServer.vehicle_get_position(vehicle)
    return TractionServer.wire_find_above_with_height(Vector3(position.x, SEARCH_HEIGHT, position.z),
            Vector3.UP, Vector3.BACK, Vector3.LEFT, SEARCH_WIDTH, SEARCH_WIDTH).get("rid", RID())
