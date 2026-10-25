@tool
extends RefCounted
class_name MaszynaDriverLights

## The original driver's lights (TController::CheckVehicles(), control_lights(), Driver.cpp:2349-2525,
## 6389-6428; the headcode hints, driverhints.cpp:1238-1308): every vehicle of the trainset put out,
## then the headcode of the order lit on the front vehicle's leading end and the last vehicle's
## trailing end. A lamp is lit by the vehicle's `light` command on its own end, as RaLightsSet()
## writes iLights (DynObj.cpp:7261-7355).
##
## Not ported: the model's lamp inventory (iInventory) is not known, so a rear end shows its red
## markers where the original would pick plates for a model without them - see TODO.md, "Drivers".
## A vehicle without RailVehicleLighting stands for an empty inventory.

## The original's lamp bits (light::, MOVER.h:299-323) and the lamps of the `light` command they
## name; the other bits have no lamp here
const HEADLIGHT_LEFT:int = 1
const REDMARKER_LEFT:int = 2
const HEADLIGHT_UPPER:int = 4
const HEADLIGHT_RIGHT:int = 16
const REDMARKER_RIGHT:int = 32
const REAR_END_SIGNALS:int = 64
const LAMPS:Dictionary[int, String] = {
    HEADLIGHT_LEFT: "headlight_left",
    REDMARKER_LEFT: "redmarker_left",
    HEADLIGHT_UPPER: "headlight_upper",
    HEADLIGHT_RIGHT: "headlight_right",
    REDMARKER_RIGHT: "redmarker_right",
}
## Pc1, the train's head (headcodepc1, driverhints.cpp:1239)
const PC1:int = HEADLIGHT_LEFT | HEADLIGHT_RIGHT | HEADLIGHT_UPPER
## Pc2, a train on the wrong track (headcodepc2, driverhints.cpp:1259)
const PC2:int = REDMARKER_LEFT | HEADLIGHT_RIGHT | HEADLIGHT_UPPER
## Pc5, the end of the train: red markers or plates, whichever the vehicle shows (headcodepc5,
## driverhints.cpp:1284; RaLightsSet(), DynObj.cpp:7266-7288)
const PC5:int = REDMARKER_LEFT | REDMARKER_RIGHT | REAR_END_SIGNALS
const RED_MARKERS:int = REDMARKER_LEFT | REDMARKER_RIGHT
## A vehicle with more power than this [kW] is an engine (Power > 1.0, DynObj.cpp:7272)
const POWERED:float = 1.0
## No pattern asked for (m_lighthints, Driver.h)
const NO_HINT:int = -1
## An end left as it is (RaLightsSet()'s -1, DynObj.cpp:7261)
const KEEP:int = -1
## The vehicle's couplers (end::front, end::rear) and the prefix of their lamps' names
const END_PREFIXES:PackedStringArray = ["front_", "rear_"]
## The orders that light a headcode (TOrders, Driver.h:29-45)
const SHUNTING_ORDERS:int = (MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT
        | MaszynaAIDriver.Order.CONNECT)


## CheckVehicles()'s lights for a driver the computer is (AIControllFlag): every vehicle put out
## (RaLightsSet(0, 0), Driver.cpp:2451), then those of the order (control())
static func check_vehicles(situation:MaszynaDriverTraction.Situation) -> void:
    for other:RID in _trainset(situation):
        set_end(other, RailVehicleController.COUPLER_END_FRONT, 0)
        set_end(other, RailVehicleController.COUPLER_END_REAR, 0)
    control(situation)


## control_lights() (Driver.cpp:6486-6520): Pc1 or Pc2 at the head of a train and Pc5 at its end,
## or the patterns the scenery asked for (SetLights) - those set only by a driver the computer is;
## Tb1 shunting, one white lamp at each end, diagonally. Any other order lights nothing.
static func control(situation:MaszynaDriverTraction.Situation) -> void:
    var hints:Vector2i = situation.state.light_hints
    var acting:bool = DriverServer.vehicle_is_control_active(situation.vehicle)
    if situation.order & MaszynaAIDriver.Order.OBEY_TRAIN:
        if hints.x == NO_HINT:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HEADCODE_PC1, 0.0,
                    func() -> void: _light(situation, PC1, KEEP))
        elif hints.x == PC2:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HEADCODE_PC2, 0.0,
                    func() -> void: _light(situation, PC2, KEEP))
        elif acting:
            _light(situation, hints.x, KEEP)
        if hints.y == NO_HINT:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HEADCODE_PC5, 0.0,
                    func() -> void: _light(situation, KEEP, PC5))
        elif acting:
            _light(situation, KEEP, hints.y)
    elif situation.order & SHUNTING_ORDERS:
        var tb1:Vector2i = _tb1(situation)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HEADCODE_TB1, 0.0,
                func() -> void: _light(situation, tb1.x, tb1.y))


## The lightsoff hint's action (Lights(0, 0), driverhints.cpp:1306): the trainset's head and tail
## put out
static func off(situation:MaszynaDriverTraction.Situation) -> void:
    _light(situation, 0, 0)


## The light selector's preset (0-based) nearest to the one it stands at whose lamps show a lamp
## code hint's pattern - the first column at the end driven from, the second at the other, whichever
## the cab (SetLights(), RaLightsSet(), DynObj.cpp:7322-7380); -1 on a vehicle without a selector or
## without such a preset
static func light_preset(situation:MaszynaDriverTraction.Situation, hint:MaszynaDriverHints.Hint) -> int:
    var lighting:RailVehicleLighting = VehicleServer.vehicle_component_get(
            situation.vehicle, VehicleComponentType.COMPONENT_LIGHTING) as RailVehicleLighting
    var presets:Array = lighting.get_lights_list() if lighting else []
    var current:int = lighting.get_position() - 1 if lighting else 0
    var found:int = -1
    for index:int in presets.size():
        var item:RailVehicleLightListItem = presets[index]
        var head:int = _preset_pattern(item, true)
        var rear:int = _preset_pattern(item, false)
        var shows_hint:bool = false
        match hint:
            MaszynaDriverHints.Hint.HEADCODE_PC1:
                shows_hint = head == PC1
            MaszynaDriverHints.Hint.HEADCODE_PC2:
                shows_hint = head == PC2
            MaszynaDriverHints.Hint.HEADCODE_PC5:
                shows_hint = (rear & RED_MARKERS) == RED_MARKERS
            MaszynaDriverHints.Hint.HEADCODE_TB1:
                var tb1:Vector2i = _tb1(situation)
                shows_hint = head == tb1.x and rear == tb1.y
            MaszynaDriverHints.Hint.LIGHTS_OFF:
                shows_hint = head == 0 and rear == 0
        if shows_hint and (found < 0 or absi(index - current) < absi(found - current)):
            found = index
    return found


## A preset's lamps of its first (`head`) or second column, as the original's bits (the LightsList
## row, MaszynaMoverRailVehicleLighting)
static func _preset_pattern(item:RailVehicleLightListItem, head:bool) -> int:
    if head:
        return (HEADLIGHT_UPPER if item.cabin_a_head_light else 0) | (HEADLIGHT_LEFT if item.cabin_a_left_white_signal else 0) \
                | (REDMARKER_LEFT if item.cabin_a_left_red_signal else 0) | (HEADLIGHT_RIGHT if item.cabin_a_right_white_signal else 0) \
                | (REDMARKER_RIGHT if item.cabin_a_right_red_signal else 0) | (REAR_END_SIGNALS if item.cabin_a_end_signals else 0)
    return (HEADLIGHT_UPPER if item.cabin_b_head_light else 0) | (HEADLIGHT_LEFT if item.cabin_b_left_white_signal else 0) \
            | (REDMARKER_LEFT if item.cabin_b_left_red_signal else 0) | (HEADLIGHT_RIGHT if item.cabin_b_right_white_signal else 0) \
            | (REDMARKER_RIGHT if item.cabin_b_right_red_signal else 0) | (REAR_END_SIGNALS if item.cabin_b_end_signals else 0)


## Whether the trainset shows a lamp code hint's lamps and no other (has_signal_pc1_on(),
## has_signal_on(): iLights == pattern, DynObj.cpp:7507-7553) - a wrong signal is no lamp code
static func shows(situation:MaszynaDriverTraction.Situation, hint:MaszynaDriverHints.Hint) -> bool:
    var vehicles:Array[RID] = _trainset(situation)
    var ends:Array[RailVehicleController.CouplerEnd] = _ends(vehicles, situation.state.direction)
    var head:int = _lit(vehicles[0], ends[0])
    var rear:int = _lit(vehicles[-1], ends[1])
    match hint:
        MaszynaDriverHints.Hint.HEADCODE_PC1:
            return head == PC1
        MaszynaDriverHints.Hint.HEADCODE_PC2:
            return head == PC2
        MaszynaDriverHints.Hint.HEADCODE_PC5:
            return rear == _end_of_train(vehicles[-1], PC5)
        MaszynaDriverHints.Hint.HEADCODE_TB1:
            var tb1:Vector2i = _tb1(situation)
            return head == tb1.x and rear == tb1.y
    return head == 0 and rear == 0


## One end of a vehicle showing `pattern` (the original's bits): only the lamps that differ are
## switched. A vehicle without lamps lights none (head & iInventory[end], DynObj.cpp:7293).
static func set_end(vehicle:RID, end:RailVehicleController.CouplerEnd, pattern:int) -> void:
    var lighting:RailVehicleLighting = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_LIGHTING) as RailVehicleLighting
    if not lighting:
        return
    for bit:int in LAMPS:
        var lamp:String = END_PREFIXES[end] + LAMPS[bit]
        var lit:bool = bool(pattern & bit)
        if not lighting.light_is_enabled(lamp) == lit:
            MaszynaDriverHints.send(vehicle, "light", lamp, lit)


## Lights(head, rear) (Driver.cpp:2561-2565): `head` on the front vehicle's leading end, `rear` on
## the last vehicle's trailing end - KEEP leaves one as it is (RaLightsSet(..., -1))
static func _light(situation:MaszynaDriverTraction.Situation, head:int, rear:int) -> void:
    var vehicles:Array[RID] = _trainset(situation)
    var ends:Array[RailVehicleController.CouplerEnd] = _ends(vehicles, situation.state.direction)
    if not head == KEEP:
        set_end(vehicles[0], ends[0], head)
    if not rear == KEEP:
        set_end(vehicles[-1], ends[1], _end_of_train(vehicles[-1], rear))


## The front vehicle's leading end and the last vehicle's trailing end - each the end nothing is
## coupled to, a lone vehicle's the way the driver drives
static func _ends(vehicles:Array[RID], direction:int) -> Array[RailVehicleController.CouplerEnd]:
    var leading:RailVehicleController.CouplerEnd = (RailVehicleController.COUPLER_END_FRONT if direction >= 0
            else RailVehicleController.COUPLER_END_REAR)
    var trailing:RailVehicleController.CouplerEnd = RailVehicleController.opposite_end(leading)
    if vehicles.size() > 1:
        leading = (RailVehicleController.COUPLER_END_FRONT
                if _is_free(vehicles[0], RailVehicleController.COUPLER_END_FRONT) else RailVehicleController.COUPLER_END_REAR)
        trailing = (RailVehicleController.COUPLER_END_FRONT
                if _is_free(vehicles[-1], RailVehicleController.COUPLER_END_FRONT) else RailVehicleController.COUPLER_END_REAR)
    return [leading, trailing]


## Tb1's lamps, head and rear: the right one at the head driving forward, the left one otherwise
## (headcodetb1, driverhints.cpp:1270-1276)
static func _tb1(situation:MaszynaDriverTraction.Situation) -> Vector2i:
    if not VehicleServer.vehicle_get_controller(situation.vehicle).get_direction() == VehicleController.DIRECTION_BACKWARD:
        return Vector2i(HEADLIGHT_RIGHT, HEADLIGHT_LEFT)
    return Vector2i(HEADLIGHT_LEFT, HEADLIGHT_RIGHT)


## The original's bits of the lamps lit at a vehicle's end; a vehicle without lamps shows none
static func _lit(vehicle:RID, end:RailVehicleController.CouplerEnd) -> int:
    var lighting:RailVehicleLighting = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_LIGHTING) as RailVehicleLighting
    var lit:int = 0
    if not lighting:
        return lit
    for bit:int in LAMPS:
        if lighting.light_is_enabled(END_PREFIXES[end] + LAMPS[bit]):
            lit |= bit
    return lit


## Pc5 resolved (RaLightsSet(), DynObj.cpp:7266-7288): an engine with no direction set shows
## plates - no lamp here - any other vehicle its red markers
static func _end_of_train(vehicle:RID, pattern:int) -> int:
    if not pattern == PC5:
        return pattern
    var controller:VehicleController = VehicleServer.vehicle_get_controller(vehicle)
    if controller.power > POWERED and controller.get_direction() == VehicleController.DIRECTION_NEUTRAL:
        return 0
    return RED_MARKERS


## The trainset from the front the way the driver drives, read afresh - after coupling or uncoupling
## it is already the new one (pVehicles[], CheckVehicles(), Driver.cpp:2358-2389)
static func _trainset(situation:MaszynaDriverTraction.Situation) -> Array[RID]:
    var end:RailVehicleController.CouplerEnd = (RailVehicleController.COUPLER_END_FRONT if situation.state.direction >= 0
            else RailVehicleController.COUPLER_END_REAR)
    return RailVehicleServer.vehicle_get_coupled(situation.vehicle, end, RailVehicleController.COUPLING_FLAG_COUPLER)


## Nothing is coupled at the vehicle's end - the walk out through it starts at the vehicle itself
static func _is_free(vehicle:RID, end:RailVehicleController.CouplerEnd) -> bool:
    return RailVehicleServer.vehicle_get_coupled(vehicle, end, RailVehicleController.COUPLING_FLAG_COUPLER)[0] == vehicle
