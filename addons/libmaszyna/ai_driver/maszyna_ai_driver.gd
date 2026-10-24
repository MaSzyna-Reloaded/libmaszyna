@tool
extends DriverImplementation
class_name MaszynaAIDriver

## The original's AI driver (TController, Driver.cpp): the orders a scenario gives a train, in the
## original's vocabulary (TController::PutCommand(), Driver.cpp:4468-4906), and the list of orders
## it works through (OrderList, OrderNext(), OrderPush(), JumpToNextOrder(), Driver.cpp:2040,
## 5125-5236). It takes the orders and carries out those that are a sequence of controls -
## preparing the vehicle, putting it away, turning (PrepareEngine(), ReleaseEngine(), Activation(),
## Driver.cpp:2051, 2759, 2918) - through the cab, as a player does. Every step it decides on is
## cued (MaszynaDriverHints): taken while the computer drives, and kept in its list of hints
## either way, so a player driving its vehicle sees what to do. It acts in moments, one reaction
## time apart
## (DriverServer.driver_schedule_update()). One implementation serves every driver, their state is kept
## per driver RID.

## TOrders (Driver.h:29-45): the operations are bits, so a change of direction can sit on top of
## another order
enum Order {
    WAIT_FOR_ORDERS = 0,
    PREPARE_ENGINE = 1,
    RELEASE_ENGINE = 2,
    CHANGE_DIRECTION = 4,
    CONNECT = 8,
    DISCONNECT = 16,
    SHUNT = 32,
    LOOSE_SHUNT = 64,
    OBEY_TRAIN = 128,
    BANK = 256,
    JUMP_TO_FIRST_ORDER = 512,
}
## What PrepareEngine() waits for before the vehicle is ready (isready, Driver.cpp:2843-2851), as
## bits: a converter overload relay open, the line breaker open, the reverser at neutral (DirActive),
## the converter off, the air below MIN_MAIN_RESERVOIR_PRESSURE
enum EngineCheck {
    CONVERTER_OVERLOAD = 1,
    LINE_BREAKER = 2,
    DIRECTION = 4,
    CONVERTER = 8,
    AIR = 16,
}

## Orders a driver's list holds (maxorders, Driver.h:191)
const MAX_ORDERS:int = 64
## `Timetable:<name>` - the name is a file in the scenery's directory, `none` for no timetable
## (Driver.cpp:4494; a scenery's trainsets send it, MaszynaSceneryProvider.TIMETABLE_ORDER)
const TIMETABLE_PREFIX:String = "Timetable:"
const NO_TIMETABLE:String = "none"
const SCENERY_DIRECTORY:String = "scenery"
## The guard's departure message (tsGuardSignal, Driver.cpp:4466-4483): <timetable>.<ext>, heard
## beside the train, else <timetable>radio.<ext> on the train radio, looked up in the scenery, then
## in the sounds, with its transcript beside it. Only the radio one is played, and not from a .flac
## file (TODO.md)
const SOUNDS_DIRECTORY:String = "sounds"
const GUARD_RADIO_SUFFIX:String = "radio"
const GUARD_SOUND_EXTENSIONS:PackedStringArray = ["ogg", "flac", "wav"]
const OGG_EXTENSION:String = "ogg"
const WAV_EXTENSION:String = "wav"
## EU07_SOUND_HANDHELDRADIORANGE (sound.h:21): as far as the guard's radio reaches [m]
const GUARD_RADIO_RANGE:float = 3500.0
## fActionTime = -5.0 once the guard has spoken (Driver.cpp:6872, 6882)
const GUARD_HOLD_TIME:float = 5.0
## iRadioChannel before it is told one (Driver.h)
const RADIO_CHANNEL_DEFAULT:int = 1
## The shunting speed until an order gives another [km/h] (fShuntVelocity, Driver.h:431)
const DEFAULT_SHUNT_VELOCITY:float = 40.0
## A shunting speed is kept only when it is a speed (fabs(NewValue1) > 2.0, Driver.cpp:4667)
const MIN_SHUNT_VELOCITY:float = 2.0
## `Shunt <vehicles> <coupler>`: -1 all the vehicles; below -1.5 wait for the signal; below -2.5
## train or bank after (Driver.cpp:4762-4863)
const ALL_VEHICLES:float = -1.0
const WAIT_FOR_SIGNAL:float = -1.5
const TRAIN_AFTER_SHUNT:float = -2.5
## A timetable speed between these starts in shunting (OrdersInit(), Driver.cpp:5250-5251)
const SHUNT_START_VELOCITY_MAX:float = 0.02
## The orders a driver goes on with while its engine is not ready (handle_engine(), Driver.cpp:7239)
const DRIVING_ORDERS:int = (Order.CHANGE_DIRECTION | Order.CONNECT | Order.DISCONNECT | Order.SHUNT
        | Order.LOOSE_SHUNT | Order.OBEY_TRAIN | Order.BANK)
## Reaction times [s]: preparing a standing vehicle, and otherwise (PrepareTime, EasyReactionTime,
## Driver.cpp:150, 158)
const PREPARE_TIME:float = 2.0
const EASY_REACTION_TIME:float = 0.5
## Preparing a vehicle rolling faster than this [km/h] takes the easy reaction time (Driver.cpp:2765)
const ROLLING_START_SPEED:float = 5.0
## A vehicle slower than this [km/h] stands (EU07_AI_NOMOVEMENT, Driver.h:25)
const NO_MOVEMENT_SPEED:float = 0.05
## Coupling up, the way behind is not looked at while the vehicle to couple to is nearer than this
## [m] or than the first signal (check_route_behind(), Driver.cpp:8247-8250)
const CONNECT_SCAN_DISTANCE:float = 2000.0
## The main reservoir pressure the vehicle is ready to drive at (ScndPipePress, Driver.cpp:2894)
const MIN_MAIN_RESERVOIR_PRESSURE:float = 4.5
## What keeps the vehicle from being ready, as the stop reason's details (DriverState)
const ENGINE_CHECK_NAMES:Dictionary[EngineCheck, String] = {
    EngineCheck.CONVERTER_OVERLOAD: "converter overload",
    EngineCheck.LINE_BREAKER: "line breaker",
    EngineCheck.DIRECTION: "reverser",
    EngineCheck.CONVERTER: "converter",
    EngineCheck.AIR: "main reservoir below %.1f bar",
}
## A train keeps under its timetable's speed; any other order under the shunting speed
## (driveruipanels.cpp:78-81)
const TRAIN_ORDERS:int = Order.OBEY_TRAIN | Order.BANK
## A hint's text with a place for its parameter (`%.0f`)
const HINT_PARAMETER_MARK:String = "%"
## PrepareHeating() (Driver.cpp:5040-5045): a circuit counts as cold this far [C] over its lowest
## temperature while the water heater works
const HEATING_HYSTERESIS:float = 5.0
## Radio-Stop: the radio is switched off this long [s] after the train stopped, and on again this
## long after the stop is lifted (control_security_system(), Driver.cpp:6411-6431)
const RADIO_STOP_DELAY:float = 5.0
## Standing, for Radio-Stop's radio (Vel < 0.01, Driver.cpp:6411)
const RADIO_STOP_STANDING_SPEED:float = 0.01
## The horn before moving off sounds this long [s] (Driver.cpp:6374)
const START_HORN_DURATION:float = 0.3
## The horn the driver sounds by default, the low one (control_horns(), Driver.cpp:6355; iHornWarning,
## DynObj.cpp:1920)
const DEFAULT_HORN:int = MaszynaDriverHints.HORN_LOW
## Uncoupling: it presses the buffers at this speed [km/h] (Driver.cpp:7334)
const PRESSING_VELOCITY:float = 2.0
## Faster than this [km/h] the trainset is taken as gone from the stop, and its dispatch is over
## (the "force timer reset" HACK, Driver.cpp:7450-7452)
const DISPATCH_MAX_SPEED:float = 2.0
## Doors that close by themselves only on a speed have none set (Doors.auto_velocity == -1.f,
## Driver.cpp:4332)
const NO_AUTO_CLOSE_VELOCITY:float = -1.0
## The doors the driver works for the whole trainset, and the doors passengers close by hand
## (control_t, Driver.cpp:4282-4284, 4316-4318, 4333-4334)
const REMOTE_OPEN_CONTROLS:Array[RailVehicleDoors.Controls] = [
    RailVehicleDoors.CONTROLS_CONDUCTOR, RailVehicleDoors.CONTROLS_DRIVER,
]
const REMOTE_CLOSE_CONTROLS:Array[RailVehicleDoors.Controls] = [
    RailVehicleDoors.CONTROLS_CONDUCTOR, RailVehicleDoors.CONTROLS_DRIVER, RailVehicleDoors.CONTROLS_MIXED,
]
const MANUAL_CLOSE_CONTROLS:Array[RailVehicleDoors.Controls] = [
    RailVehicleDoors.CONTROLS_PASSENGER, RailVehicleDoors.CONTROLS_MIXED,
]
## Coupling up: it starts within this of the vehicle ahead, and couples within ATTACH_DISTANCE [m]
## (UpdateConnect(), Driver.cpp:7005-7040)
const CONNECT_DISTANCE:float = 20.0
const ATTACH_DISTANCE:float = 2.0
## UpdateConnect(): within this of the vehicle ahead an end takes the adapter it needs (Driver.cpp:6900)
const ADAPTER_DISTANCE:float = 10.0
## The couplings a `Shunt` may ask for (coupling::, MOVER.h:162); the high voltage and the power
## lines are nothing a shunter joins
const SHUNTER_COUPLINGS:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_COUPLER
        | RailVehicleController.COUPLING_FLAG_BRAKEHOSE | RailVehicleController.COUPLING_FLAG_CONTROL
        | RailVehicleController.COUPLING_FLAG_GANGWAY | RailVehicleController.COUPLING_FLAG_MAINHOSE
        | RailVehicleController.COUPLING_FLAG_HEATING) as RailVehicleController.CouplingFlags


## What one driver keeps
class AIDriverState:
    ## The driver itself
    var driver:RID = RID()
    var orders:PackedInt32Array = []
    var order_position:int = 0
    var order_top:int = 1
    ## iEngineActive - the vehicle is ready to drive (_prepare_engine()), and the EngineCheck
    ## flags that kept it from being ready on the last check
    var engine_active:bool = false
    var engine_missing:int = 0
    ## iDirection (the cab it drives from, +1 or -1) and iDirectionOrder (the one it was told to)
    var direction:int = 1
    var direction_order:int = 0
    ## SetVelocity/ShuntVelocity: the speed allowed and the one after it; stop_here - not to move
    ## towards a signal until told to (moveStopHere)
    ## at first it stands (Driver.h:392)
    var velocity:float = 0.0
    var velocity_next:float = -1.0
    var stop_here:bool = true
    var shunt_velocity:float = DEFAULT_SHUNT_VELOCITY
    ## iVehicleCount, iCoupler, fStopTime of `Shunt` and `Wait_for_orders`
    var vehicle_count:int = -2
    var coupler:RailVehicleController.CouplingFlags = RailVehicleController.COUPLING_FLAG_NONE
    var stop_time:float = 0.0
    ## iCouplingVehicle - the vehicle of the trainset that couples up and its end, while it does
    ## (moveConnect)
    var coupling_vehicle:RID = RID()
    var coupling_end:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
    ## movePress - it presses the buffers to uncouple; iDirectionBackup - the way it drove before,
    ## 0 for none
    var pressing:bool = false
    var direction_backup:int = 0
    ## vCommandLocation - where the last order about speed came from
    var command_position:Vector3 = Vector3.ZERO
    var radio_channel:int = -1
    ## tsGuardSignal and iGuardRadio - the guard's departure message, null for none, its
    ## transcript and the radio channel it is sent on; guard_signal_due - it is due once the train
    ## may go (moveGuardSignal)
    var guard_signal:SfxEvent = null
    var guard_transcript:Transcript = null
    var guard_radio:int = 0
    var guard_signal_due:bool = false
    ## The guard's handheld radio (RadioServer), a transmitter only, in the vehicle the driver is in
    var guard_handheld:RID
    ## m_lighthints - the light pattern asked for at the front and the rear, -1 for none
    var light_hints:Vector2i = Vector2i(-1, -1)
    ## fWarningDuration and the horn to sound
    var warning_duration:float = 0.0
    var warning_horn:int = 0
    ## IsHeatingTemperatureTooLow - a diesel's water or oil too cold to start (PrepareHeating())
    var heating_temperature_too_low:bool = false
    ## m_radiocontroltime [s] - Radio-Stop's radio switched off and on after a delay
    var radio_control_time:float = 0.0
    ## moveStartHorn, moveStartHornNow, moveStartHornDone: the horn before moving off is due, is to
    ## sound now, has sounded (Driver.h)
    var start_horn:bool = false
    var start_horn_now:bool = false
    var start_horn_done:bool = false
    ## The departure signal sounded at the door closing, the doors still to close on the next
    ## update (moveDepartureWarned, Doors(), Driver.cpp:4364-4378)
    var departure_warned:bool = false
    ## Its timetable and how far it got through it
    var timetable:MaszynaDriverTimetable = MaszynaDriverTimetable.new()
    ## ReactionTime - until its next update
    var reaction_time:float = PREPARE_TIME
    ## What it read of its trainset on its last update
    var trainset:MaszynaDriverTrainset = MaszynaDriverTrainset.new()
    ## The speed and acceleration it wants
    var speed:MaszynaDriverSpeed = MaszynaDriverSpeed.new()
    ## Its own train brake handle position and brake timing
    var braking:MaszynaDriverBraking = MaszynaDriverBraking.new()
    ## How it drives the engine of its vehicle - the kind of engine's own (create())
    var traction:MaszynaDriverTraction = MaszynaDriverTraction.create(RailVehicleEngine.NONE)
    ## What it reads of the tracks ahead
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()
    ## m_hints (Driver.h:468): the steps cued and not done yet, in the order they came - changed only
    ## by MaszynaDriverHints.cue() and update()
    var hints:Array[MaszynaDriverHints.Queued] = []

    func _init() -> void:
        orders.resize(MAX_ORDERS)


var _drivers:Dictionary[RID, AIDriverState] = {}


func _init() -> void:
    StationServer.dispatch_step_changed.connect(_on_dispatch_step_changed)
    VehicleServer.vehicle_command_received.connect(_on_vehicle_command_received)


## DirectionChange() (Driver.cpp:2624-2631) of the driver of a vehicle a player drives - the computer
## turns through its own orders: after a cab change, once the new cab is switched on, its last step
## (TTrain::CabChange(), Train.cpp:10335-10336; RailVehicleServer.person_change_cabin()), and after
## the reverser is moved off neutral (OnCommand_reverserforward/backward..., Train.cpp:2670-2842).
## Taken when the person moved, with the cab left still active, the driver read the way of the cab
## left (FINDINGS.md 2026-10-06). The way is the reverser's and the active cab's (CheckDirection(),
## DirAbsolute, else CabActive, Driver.cpp:2384-2392), its trainset read again on a change; with
## neither it keeps its way, the driver here never drives none
func _on_vehicle_command_received(vehicle:RID, command:String, _p1:Variant, _p2:Variant) -> void:
    var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
    if not controller or DriverServer.vehicle_is_control_active(vehicle):
        return
    var reverser:bool = (command == "direction_increase" or command == "direction_decrease") \
            and not controller.get_direction() == VehicleController.DIRECTION_NEUTRAL
    # a cab switched on by hand as well - a vehicle without the automatic activation
    # (AutomaticCabActivation=No, SA132) never sends the automatic one, and its driver kept the way
    # of the cab the player had left
    var activated:bool = command == "cab_activation_auto" or (command == "cab_activation" and _p1 == true)
    if not (activated or reverser):
        return
    var direction:int = controller.get_direction_absolute()
    if direction == VehicleController.DIRECTION_NEUTRAL:
        direction = _active_cab(vehicle)
    for state:AIDriverState in _drivers.values():
        if VehicleServer.person_get_vehicle(state.driver) == vehicle and not direction == VehicleController.DIRECTION_NEUTRAL \
                and not direction == state.direction:
            state.direction = direction
            _check_vehicles(state)


func _driver_attached(driver:RID) -> void:
    var state:AIDriverState = AIDriverState.new()
    state.driver = driver
    if _cabin_direction(driver) < 0:
        state.direction = -1
    # told to drive the way it faces until told otherwise (iDirectionOrder = CabActive,
    # Driver.cpp:1872) - never none: turning towards none puts the reverser at neutral and takes
    # that for the way it drives
    state.direction_order = state.direction
    state.timetable.changed.connect(DriverServer.driver_report_timetable_changed.bind(driver))
    state.guard_handheld = RadioServer.radio_create()
    _drivers[driver] = state
    DriverServer.driver_schedule_update(driver, 0.0)


func _driver_detached(driver:RID) -> void:
    var state:AIDriverState = _drivers.get(driver)
    if state:
        RadioServer.radio_free(state.guard_handheld)
    _drivers.erase(driver)


## A player left the cab: the driver drives the vehicle as it was left (TakeControl(),
## Driver.cpp:5700-5712) - its cab switched on, the way it drives guessed again
## (PrepareDirection(), Driver.cpp:5088-5116): standing, towards the cab driven from; moving, the way
## it moves - the reverser put that way, and the tracks read afresh
func _control_taken(driver:RID) -> void:
    var state:AIDriverState = _drivers.get(driver)
    var vehicle:RID = VehicleServer.person_get_vehicle(driver)
    if not state or not vehicle.is_valid():
        return
    # the cab the crew sits in switched on (CabActivisation(true), Driver.cpp:5705)
    MaszynaDriverHints.cue(_read_situation(state), MaszynaDriverHints.Hint.CAB_ACTIVATION)
    if VehicleServer.vehicle_get_speed(vehicle) < MaszynaDriverTrainset.NO_MOVEMENT_SPEED:
        # the active cab, else the one the crew sits in; a vehicle with neither keeps its way
        var cab:int = _active_cab(vehicle)
        if cab == 0:
            cab = _cabin_direction(driver)
        if not cab == 0:
            state.direction = cab
    else:
        state.direction = 1 if VehicleServer.vehicle_get_velocity(vehicle) >= 0.0 else -1
    state.direction_order = state.direction
    state.route.forget()
    # and the reverser put that way - a player may have left it the other (PrepareDirection())
    _prepare_direction(_read_situation(state))
    # the lights of its order (control_lights(), CheckVehicles(), Driver.cpp:5625, 5657)
    _check_vehicles(state)


## PrepareDirection() (Driver.cpp:5116-5121): the master controller down until the reverser may
## move, then the reverser the way the driver drives, relative to the cab
func _prepare_direction(situation:MaszynaDriverTraction.Situation) -> void:
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_FORWARD if situation.state.direction > 0 else MaszynaDriverHints.Hint.DIRECTION_BACKWARD)


## Doors() (Driver.cpp:4266-4356): the doors the driver works permitted and opened at the platform
## as the dispatch lets the passengers off and on, and closed once the train is let go - cued, so a
## player is hinted; a driver the computer is (AIControllFlag) also closes the doors passengers
## close by hand, in the cars done exchanging. Each car's doors are closed by the way they close,
## so a train is not held by a vehicle driven from that has no doors of its own. Doors that warn
## have the departure signal sounded first, and close on the driver's next update.
func _on_dispatch_step_changed(vehicle:RID, step:StationServer.DispatchStep) -> void:
    var state:AIDriverState = _drivers.get(DriverServer.vehicle_get_driver(vehicle))
    if not state:
        return
    var situation:MaszynaDriverTraction.Situation = _read_situation(state)
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_DOORS)
    match step:
        StationServer.DISPATCH_STEP_EXCHANGE:
            if not doors:
                return
            # the platform's side of the train, on the vehicle driven from as it stands
            var platform:RailVehicleLoad.PlatformSide = state.route.exchange_platform
            if state.trainset.directions[state.trainset.vehicles.find(vehicle)] < 0:
                platform = MaszynaStation.opposite_side(platform)
            var left:bool = not platform == RailVehicleLoad.PLATFORM_SIDE_RIGHT
            var right:bool = not platform == RailVehicleLoad.PLATFORM_SIDE_LEFT
            if doors.permit_required:
                if left:
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_ON)
                if right:
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_ON)
            if doors.open_method in REMOTE_OPEN_CONTROLS:
                if left:
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_LEFT_OPEN)
                if right:
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_RIGHT_OPEN)
        StationServer.DISPATCH_STEP_CLOSE_DOORS:
            _start_closing_doors(situation)


## Doors(false) (Driver.cpp:4357-4395): the doors that warn have the departure signal sounded
## first, and close on the driver's next update; the others close at once
func _start_closing_doors(situation:MaszynaDriverTraction.Situation) -> void:
    var state:AIDriverState = situation.state
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(situation.vehicle, VehicleComponentType.COMPONENT_DOORS)
    if doors and doors.close_warning and not doors.close_auto_close_warning and not state.departure_warned:
        state.departure_warned = true
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DEPARTURE_SIGNAL_ON)
        return
    _close_doors(situation)


## The doors closed once the train is let go (Doors(), Driver.cpp:4380-4395): the permits taken
## back and the doors the driver works closed - cued; a driver the computer is also closes the
## doors passengers close by hand, in the cars done exchanging
func _close_doors(situation:MaszynaDriverTraction.Situation) -> void:
    var state:AIDriverState = situation.state
    var vehicle:RID = situation.vehicle
    state.departure_warned = false
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_DOORS)
    # the doors closed first: closing them revokes the permits (Mover.cpp:8745-8749)
    if doors and doors.close_method in REMOTE_CLOSE_CONTROLS:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_RIGHT_CLOSE)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_LEFT_CLOSE)
    if doors and doors.permit_required:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_OFF)
    if not DriverServer.vehicle_is_control_active(vehicle):
        return
    for car:RID in state.trainset.vehicles:
        var car_doors:RailVehicleDoors = VehicleServer.vehicle_component_get(car, VehicleComponentType.COMPONENT_DOORS)
        if not car_doors:
            continue
        if not car == vehicle and car_doors.close_method in REMOTE_CLOSE_CONTROLS:
            MaszynaDriverHints.send(car, &"doors_left", false)
            MaszynaDriverHints.send(car, &"doors_right", false)
        if car_doors.close_auto_close_velocity == NO_AUTO_CLOSE_VELOCITY \
                and car_doors.close_method in MANUAL_CLOSE_CONTROLS \
                and RailVehicleServer.load_get_exchange_time(car) == 0.0:
            MaszynaDriverHints.send(car, &"doors_left_local", false)
            MaszynaDriverHints.send(car, &"doors_right_local", false)


## The seconds from `hours` to the departure of the driver's train, NAN without a timetable
## (DriverImplementation.get_seconds_until_departure())
func _get_seconds_until_departure(driver:RID, hours:float) -> float:
    var state:AIDriverState = _drivers.get(driver)
    if not state or not state.timetable.timetable.is_valid():
        return NAN
    return state.timetable.seconds_until_departure(hours)


## The order to show the player, translated (OrderCurrent(), Driver.cpp:1990-2035): a change of
## direction first, unless it is coupling; an uncoupling with the vehicles it leaves the engine with;
## empty for an order the original names not
static func order_text(order:int, vehicle_count:int, coupling:bool) -> String:
    if order & Order.CHANGE_DIRECTION and not coupling:
        return TranslationServer.translate("Change direction")
    match order & ~Order.CHANGE_DIRECTION:
        Order.WAIT_FOR_ORDERS:
            return TranslationServer.translate("Wait for orders")
        Order.PREPARE_ENGINE:
            return TranslationServer.translate("Start the engine")
        Order.RELEASE_ENGINE:
            return TranslationServer.translate("Shut down the engine")
        Order.CHANGE_DIRECTION:
            return TranslationServer.translate("Change direction")
        Order.CONNECT:
            return TranslationServer.translate("Couple to consist ahead")
        Order.DISCONNECT:
            if vehicle_count < 0:
                # done with uncoupling, the order changes shortly
                return TranslationServer.translate("Wait for orders")
            var vehicles:String = TranslationServer.translate("the engine")
            if vehicle_count == 1:
                vehicles = TranslationServer.translate("the engine plus the next vehicle")
            elif vehicle_count > 1:
                vehicles = TranslationServer.translate("the engine plus %d next vehicles") % vehicle_count
            return TranslationServer.translate("Uncouple %s") % vehicles
        Order.SHUNT:
            return TranslationServer.translate("Shunt according to signals")
        Order.LOOSE_SHUNT:
            return TranslationServer.translate("Loose shunt according to signals")
        Order.OBEY_TRAIN:
            return TranslationServer.translate("Drive according to signals and timetable")
        Order.BANK:
            return TranslationServer.translate("Bank consist ahead")
    return ""


## What the driver does and wants now, for the player (DriverImplementation.get_state()): its
## order, the speed it may drive at and the next limit with its distance (driveruipanels.cpp:73-122),
## why it stands, how long it is to stand yet, and its hints
func _get_state(driver:RID) -> DriverState:
    var state:AIDriverState = _drivers.get(driver)
    if not state:
        return null
    var order:int = state.orders[state.order_position]
    var shown:DriverState = DriverState.new()
    shown.task = order_text(order, state.vehicle_count, state.coupling_vehicle.is_valid())
    shown.waiting_for_orders = order == Order.WAIT_FOR_ORDERS
    shown.direction = state.direction as VehicleController.Direction
    shown.at_stop = state.route.at_passenger_stop
    shown.stop_time_left = maxf(state.stop_time, 0.0)
    shown.stop_reason = state.speed.stop_reason
    if state.speed.stop_reason == DriverState.STOP_REASON_NOT_READY:
        var details:PackedStringArray = []
        for check:EngineCheck in ENGINE_CHECK_NAMES:
            if state.engine_missing & check:
                var name:String = TranslationServer.translate(ENGINE_CHECK_NAMES[check])
                details.append(name % MIN_MAIN_RESERVOIR_PRESSURE if check == EngineCheck.AIR else name)
        shown.stop_reason_details = details
    var velocity_desired:float = state.speed.velocity_desired
    var limit:float = floorf(velocity_desired)
    var next_limit:float = limit
    var next_distance:float = INF
    # not allowed to move, any next limit is irrelevant
    if not limit == 0.0:
        var schedule_limit:float = MaszynaDriverSpeed.NO_LIMIT
        if order & TRAIN_ORDERS and state.timetable.velocity > 0.0:
            schedule_limit = floorf(state.timetable.velocity)
        elif not order & TRAIN_ORDERS:
            schedule_limit = floorf(state.shunt_velocity)
        # first the change after the current limit is passed
        var last_distance:float = state.route.velocity_limit_last_distance
        if last_distance > 0.0:
            next_limit = MaszynaDriverSpeed.min_speed(schedule_limit, floorf(state.route.velocity_limit_last))
            next_distance = last_distance
        # then the change ahead: the lower of the two, else the current limit lasts until it is passed
        var proximity:float = state.speed.proximity_distance
        var proximity_limit:float = MaszynaDriverSpeed.min_speed(schedule_limit, floorf(state.speed.velocity_next))
        var take_proximity:bool = last_distance < 0.0
        if proximity_limit < next_limit:
            take_proximity = proximity_limit < velocity_desired or proximity > next_distance
        # nothing read ahead (VelNext -1, the proximity the reach) is no new limit
        if proximity_limit < 0.0:
            take_proximity = false
        if take_proximity:
            next_limit = proximity_limit
            next_distance = proximity
        # a limit reaching past the reading tells nothing of its length, and a vehicle ahead is no
        # limit - the proximity is its distance, less the gap a train keeps (speed.gd, Driver.cpp:7371)
        var obstacle:float = state.route.obstacle.distance if state.route.obstacle else -1.0
        if next_distance >= MaszynaDriverRoute.LIMIT_BEYOND_RANGE:
            next_limit = limit
        elif obstacle >= 0.0 and (proximity == obstacle
                or proximity == obstacle - MaszynaDriverSpeed.DRIVER_DISTANCE):
            next_limit = limit
    shown.speed_limit = limit
    shown.next_speed_limit = next_limit
    shown.next_speed_limit_distance = INF if next_limit == limit else next_distance
    if VehicleServer.person_get_vehicle(driver).is_valid():
        var hints:Array[DriverHint] = []
        for listed:Dictionary in MaszynaDriverHints.get_list(_read_situation(state)):
            var hint:DriverHint = DriverHint.new()
            var text:String = TranslationServer.translate(listed["text"])
            hint.text = text % listed["parameter"] if text.contains(HINT_PARAMETER_MARK) else text
            hint.control = listed["control"]
            hint.gesture = listed["gesture"]
            hint.done = listed["done"]
            hints.append(hint)
        shown.hints = hints
    return shown


## What the driver keeps, by its own working fields (TController's): its orders and what they asked
## for, what it read of its trainset and of the way ahead - for the AI's tests to follow it step by
## step; what it shows the player is _get_state()
func get_internal_state(driver:RID) -> Dictionary:
    var state:AIDriverState = _drivers.get(driver)
    if not state:
        return {}
    return {
        "order": state.orders[state.order_position],
        "order_text": order_text(state.orders[state.order_position], state.vehicle_count,
                state.coupling_vehicle.is_valid()),
        "orders": state.orders.slice(0, state.order_top),
        "order_position": state.order_position,
        "direction": state.direction,
        "direction_order": state.direction_order,
        "velocity": state.velocity,
        "velocity_next": state.velocity_next,
        "stop_here": state.stop_here,
        "shunt_velocity": state.shunt_velocity,
        "vehicle_count": state.vehicle_count,
        "coupler": state.coupler,
        "stop_time": state.stop_time,
        "radio_channel": state.radio_channel,
        "light_hints": state.light_hints,
        "warning_duration": state.warning_duration,
        "warning_horn": state.warning_horn,
        "timetable": state.timetable.timetable,
        "station_index": state.timetable.station_index,
        "station_start": state.timetable.station_start,
        "next_stop": state.timetable.next_stop,
        "at_passenger_stop": state.route.at_passenger_stop,
        "trainset_vehicles": state.trainset.vehicles,
        "trainset_mass": state.trainset.mass,
        "trainset_ready": state.trainset.ready,
        "trainset_brake_pressure_max": state.trainset.brake_pressure_max,
        "trainset_braked": state.trainset.braked,
        "trainset_gravity_acceleration": state.trainset.gravity_acceleration,
        "trainset_acceleration": state.trainset.acceleration,
        "velocity_desired": state.speed.velocity_desired,
        "stop_reason": state.speed.stop_reason,
        "engine_active": state.engine_active,
        "engine_missing": state.engine_missing,
        "speed_velocity_next": state.speed.velocity_next,
        "velocity_limit_last": state.route.velocity_limit_last,
        "velocity_limit_last_distance": state.route.velocity_limit_last_distance,
        "timetable_velocity": state.timetable.velocity,
        "acceleration_desired": state.speed.acceleration_desired,
        "brake_position": state.braking.position,
        "route_velocity_next": state.route.velocity_next,
        "proximity_distance": state.speed.proximity_distance,
        "obstacle_distance": state.route.obstacle.distance if state.route.obstacle else -1.0,
        "signal_velocity_next": state.route.signal_velocity_next,
        "hints": MaszynaDriverHints.get_list(_read_situation(state))
                if VehicleServer.person_get_vehicle(driver).is_valid() else [],
    }


func _handle_command(driver:RID, command:String, value1:float, value2:float, position:Vector3) -> void:
    var state:AIDriverState = _drivers.get(driver)
    if not state:
        return
    var vehicle:RID = VehicleServer.person_get_vehicle(driver)
    # the original writes every order to its log (TController::PutCommand(), Driver.cpp:4470)
    GameLog.get_logger("ai").debug("%s: %s %s %s (order %s)" % [
            VehicleServer.vehicle_get_id(vehicle), command, value1, value2,
            state.orders[state.order_position]])
    if command.begins_with(TIMETABLE_PREFIX):
        _take_timetable(driver, state, command.trim_prefix(TIMETABLE_PREFIX), value1, value2, position)
        return
    match command:
        "SetVelocity":
            state.command_position = position
            _cue_start_horn(state, vehicle, value1)
            if not value1 == 0.0 and not state.orders[state.order_position] == Order.OBEY_TRAIN:
                if not state.engine_active:
                    _order_next(state, Order.PREPARE_ENGINE)
                _order_next(state, Order.OBEY_TRAIN)
                _order_check(state, vehicle)
            state.stop_here = value1 == 0.0
            state.velocity = value1
            state.velocity_next = value2
        "ShuntVelocity":
            state.command_position = position
            _cue_start_horn(state, vehicle, value1)
            if not state.engine_active:
                _order_next(state, Order.PREPARE_ENGINE)
            _order_next(state, Order.SHUNT)
            if not value1 == 0.0:
                state.vehicle_count = -2
            state.velocity = value1
            state.velocity_next = value2
            state.stop_here = value1 == 0.0
            if absf(value1) > MIN_SHUNT_VELOCITY:
                state.shunt_velocity = absf(value1)
        "Wait_for_orders":
            if value1 > 0.0 and value1 > state.stop_time:
                state.stop_time = value1
            else:
                state.orders[state.order_position] = Order.WAIT_FOR_ORDERS
        "Prepare_engine":
            _orders_clear(state)
            if value1 == 0.0:
                _order_next(state, Order.RELEASE_ENGINE)
            elif value1 > 0.0:
                _order_next(state, Order.PREPARE_ENGINE)
        "Change_direction":
            var previous:int = state.orders[state.order_position]
            if not state.engine_active:
                _order_next(state, Order.PREPARE_ENGINE)
            if value1 == 0.0:
                state.direction_order = -state.direction
            else:
                state.direction_order = _direction_towards(driver, position, value1)
            if not state.direction_order == state.direction:
                _order_next(state, Order.CHANGE_DIRECTION)
            if previous >= Order.SHUNT:
                _order_next(state, previous)
            elif previous == Order.WAIT_FOR_ORDERS:
                _order_next(state, Order.SHUNT)
            # moving, no horn after a stop (Driver.cpp:4724-4725)
            if VehicleServer.vehicle_get_speed(vehicle) >= MaszynaDriverTrainset.MOVEMENT_SPEED:
                state.start_horn = false
        "Obey_train", "Bank":
            if not state.engine_active:
                _order_next(state, Order.PREPARE_ENGINE)
            _order_next(state, Order.OBEY_TRAIN if command == "Obey_train" else Order.BANK)
            _order_check(state, vehicle)
        "Shunt", "Loose_shunt":
            _take_shunt(driver, state, command == "Loose_shunt", value1, value2)
        "Jump_to_first_order":
            _jump_to_first_order(state, vehicle)
        "Jump_to_order":
            if value1 == -1.0:
                _jump_to_next_order(state, vehicle)
            elif value1 >= 0.0 and value1 < MAX_ORDERS:
                # the first position only starts it, for the old sceneries (Driver.cpp:4881-4884)
                state.order_position = maxi(floori(value1), 1)
        "Warning_signal":
            if value1 > 0.0 and value2 > 0.0:
                state.warning_duration = value1
                state.warning_horn = int(value2)
                # the horn combination asked for (Driver.cpp:4860-4861)
                if vehicle.is_valid():
                    MaszynaDriverHints.cue(_read_situation(state), MaszynaDriverHints.Hint.HORN_ON, value2)
        "Radio_channel":
            if value1 >= 0.0:
                state.radio_channel = int(value1)
                # the guard's too (Driver.cpp:4797-4800)
                if state.guard_radio:
                    state.guard_radio = int(value1)
        "SetLights":
            # the scenery's pattern, lit at once on a train (Driver.cpp:4807-4816)
            state.light_hints = Vector2i(int(value1), int(value2))
            if state.orders[state.order_position] & Order.OBEY_TRAIN:
                _check_vehicles(state)


## One moment of the driver (TController::Update(), handle_engine(), handle_orders(),
## UpdateChangeDirection(), Driver.cpp:6895-7260): what its current order asks of the cab
func _update(driver:RID) -> void:
    var state:AIDriverState = _drivers.get(driver)
    var vehicle:RID = VehicleServer.person_get_vehicle(driver)
    if not state or not vehicle.is_valid():
        return
    # the time since the last update is the reaction time it was scheduled with
    var elapsed:float = state.reaction_time
    # the horn sounds while there is time left (fWarningDuration -= dt, Driver.cpp:5993)
    state.warning_duration -= elapsed
    state.trainset.update(vehicle, state.direction, _has_diesel_engine(vehicle))
    # what happened to the trainset from outside - a line breaker tripped by a loss of voltage, a
    # relay a player opened - takes the engine's readiness away, and handle_engine() gets it ready
    # again (determine_consist_state(), Driver.cpp:6100-6104)
    if state.trainset.line_breaker_open or state.trainset.converter_overload_relay_open \
            or (state.trainset.converter_present and not state.trainset.converter_enabled):
        state.engine_active = false
    # what its brakes can do - the table again when the trainset or the kind of order changed
    state.braking.read_trainset(
            vehicle, state.orders[state.order_position], state.trainset, DriverServer.vehicle_is_control_active(vehicle))
    # DirectionalVel(), Driver.h:312: the speed, negative when it runs against the way it drives
    var directional_speed:float = VehicleServer.vehicle_get_speed(vehicle) \
            * signf(state.direction * VehicleServer.vehicle_get_velocity(vehicle))
    # shunting, the speed allowed is the shunting speed (pick_optimal_speed(), Driver.cpp:7320-7327)
    if not state.orders[state.order_position] & (Order.OBEY_TRAIN | Order.BANK):
        state.velocity = state.shunt_velocity
    # the tracks ahead, and the orders the signals and memories there give (check_route_ahead())
    state.route.update(
            vehicle, state.orders[state.order_position], state.stop_here, state.velocity, directional_speed,
            MaszynaDriverSpeed.EASY_ACCELERATION, state.trainset.velocity_max, state.trainset,
            state.timetable, SimulationServer.time_of_day, state.shunt_velocity, state.speed.velocity_desired,
            state.coupling_vehicle.is_valid(), state.braking, not DriverServer.vehicle_is_control_active(vehicle))
    state.velocity = state.route.signal_velocity
    # uncoupling: stand, then press the buffers at walking pace (pick_optimal_speed(), Driver.cpp:7330-7343)
    if state.orders[state.order_position] & Order.DISCONNECT and state.vehicle_count >= 0:
        var pressing:bool = state.pressing and state.direction == state.direction_order
        state.velocity = PRESSING_VELOCITY if pressing else 0.0
        state.velocity_next = 0.0
    # what its passenger stop asked of the orders (TableUpdateStopPoint(), Driver.cpp:1258-1370)
    for stop_order:MaszynaDriverRoute.StopOrder in state.route.stop_orders:
        match stop_order:
            MaszynaDriverRoute.StopOrder.HOLD:
                state.stop_here = true
            MaszynaDriverRoute.StopOrder.START_HORN:
                state.start_horn = true
            MaszynaDriverRoute.StopOrder.NO_START_HORN:
                state.start_horn = false
                state.start_horn_now = false
            MaszynaDriverRoute.StopOrder.GO:
                state.stop_here = false
            MaszynaDriverRoute.StopOrder.OBEY_TRAIN:
                _order_next(state, Order.OBEY_TRAIN)
            MaszynaDriverRoute.StopOrder.TURN_THEN_TRAIN, MaszynaDriverRoute.StopOrder.TURN_THEN_SHUNT:
                if not state.orders[(state.order_position + 1) % MAX_ORDERS] == Order.CHANGE_DIRECTION:
                    _order_push(state, Order.CHANGE_DIRECTION)
                    _order_push(state,
                            Order.OBEY_TRAIN if stop_order == MaszynaDriverRoute.StopOrder.TURN_THEN_TRAIN
                            else Order.SHUNT)
            MaszynaDriverRoute.StopOrder.NEXT_ORDER:
                _jump_to_next_order(state, vehicle)
            MaszynaDriverRoute.StopOrder.LOAD_EXCHANGE:
                # the station's passengers, and the train's dispatch waits for them (Driver.cpp:1233-1241)
                MaszynaStation.update_load(state.trainset, state.timetable, state.route.exchange_platform)
                StationServer.dispatch_start(vehicle, state.trainset.vehicles)
            MaszynaDriverRoute.StopOrder.GUARD_SIGNAL:
                # the timetable lets it go: its doors close once the passengers are done
                StationServer.dispatch_depart(vehicle)
                # on the radio channel of the station left (Driver.cpp:1103-1112), once the train
                # may go (Driver.cpp:1313-1316)
                var left_channel:int = state.timetable.radio_channel(state.timetable.station_index - 1)
                if state.guard_radio and left_channel > 0:
                    state.guard_radio = left_channel
                state.guard_signal_due = state.guard_signal != null
    for command:Array in state.route.commands:
        _handle_command(driver, command[0], command[1], command[2], command[3])
    # the guard's message, once the way is clear and the stop waited out - to a player's train as
    # much as to its own (UpdateObeyTrain(), Driver.cpp:6850-6884)
    if state.guard_signal_due and state.orders[state.order_position] == Order.OBEY_TRAIN \
            and state.speed.velocity_desired > 0.0:
        state.guard_signal_due = false
        RadioServer.radio_set_vehicle(state.guard_handheld, vehicle)
        RadioServer.radio_transmit(state.guard_handheld, state.guard_radio, GUARD_RADIO_RANGE, RadioServer.KIND_MESSAGE,
                {"sound": state.guard_signal, "transcript": state.guard_transcript})
        state.traction.hold(GUARD_HOLD_TIME)
    # gone from the stop, its dispatch is over (Driver.cpp:7449-7452)
    if absf(directional_speed) > DISPATCH_MAX_SPEED:
        StationServer.dispatch_cancel(vehicle)
    state.speed.pick(
            state.orders[state.order_position], state.engine_active, state.stop_here,
            StationServer.dispatch_get_step(vehicle), state.velocity,
            state.shunt_velocity, state.timetable.velocity,
            directional_speed, state.trainset, state.route, EASY_REACTION_TIME, state.braking,
            RailVehicleServer.trainset_get_type(vehicle))
    state.reaction_time = state.speed.reaction_time
    # check_route_behind() (Driver.cpp:8238-8266, at the end of the speed's pick, Driver.cpp:7298):
    # the way ahead closed, a shunting driver turns back to a signal behind that lets it go - a
    # shunter facing the end of its siding never moved (docs/findings-archive.md, 2026-10-03
    # scenarios that did not start)
    var order:int = state.orders[state.order_position]
    var coupling:bool = order & Order.CONNECT and state.route.obstacle \
            and state.route.obstacle.distance < minf(CONNECT_SCAN_DISTANCE, state.route.get_first_semaphore_distance())
    if state.route.velocity_next == 0.0 and not state.coupling_vehicle.is_valid() and not coupling:
        var behind:MaszynaDriverRoute.BackwardCommand = state.route.backward_scan(order, state.trainset)
        if not behind == MaszynaDriverRoute.BackwardCommand.NONE:
            # a memory's command there is taken without moving (uncoupling at the end of a siding)
            if behind == MaszynaDriverRoute.BackwardCommand.COMMAND:
                state.stop_here = true
            state.direction_order = -state.direction
            state.orders[state.order_position] = order | Order.CHANGE_DIRECTION
    # the engine it drives decides how (IncSpeed()'s switch on the engine type, Driver.cpp:3409)
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var engine_type:RailVehicleEngine.EngineType = engine.get_type() if engine else RailVehicleEngine.NONE
    if not state.traction.engine_type == engine_type:
        state.traction = MaszynaDriverTraction.create(engine_type)
    var situation:MaszynaDriverTraction.Situation = _read_situation(state)
    state.traction.read(situation, elapsed)
    # the steps the vehicle now shows done leave the hints (update_hints(), Driver.cpp:4987); a
    # player drives it as much as the computer: every decision is cued, and taken only by a driver
    # the computer is - its orders follow the vehicle the player gets ready too, as the timetable's
    # stops need them
    MaszynaDriverHints.update(situation)
    var in_control:bool = DriverServer.vehicle_is_control_active(vehicle)
    # the cab the crew sits in switched on, another one off (determine_consist_state(),
    # Driver.cpp:6013-6022)
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
    var powered:bool = _power24_available(vehicle)
    if master and master.get_cabin() == 0 and powered:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CAB_ACTIVATION)
    elif master and not controller.cntrl_automatic_cab_activation and (master.get_cabin() == -_cabin_direction(driver)
            or not master.get_cabin_controleable() or not powered):
        # the deactivation done where it can be: the cab with the control, else the player is sent
        # back to it (CabDeactivisation() takes only IsCabMaster(), Mover.cpp:2684)
        var here:bool = master.get_cabin_controleable() or not powered
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CAB_DEACTIVATION
                if here else MaszynaDriverHints.Hint.CAB_DEACTIVATION_OTHER_END)
        # the independent brake left applied there let off on the same way back - it brakes its car
        # whatever cab drives (Mover.cpp:4025), and leaving the driving cab deactivates it
        # (Train.cpp:10950-10954), so a second way back empties the pipe
        if not here:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_RELEASE_OTHER_END)
    # the doors warned of at their closing close now (Doors(), Driver.cpp:4374-4378)
    if state.departure_warned:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DEPARTURE_SIGNAL_OFF)
        _close_doors(situation)
    # the timetable's radio channel of the next station, driving a train (TableUpdateStopPoint(),
    # Driver.cpp:1123-1135)
    if state.orders[state.order_position] & (Order.OBEY_TRAIN | Order.BANK) \
            and state.timetable.station_index < state.timetable.get_entry_count():
        var channel:int = state.timetable.radio_channel(state.timetable.station_index)
        if channel > 0:
            if state.guard_radio:
                state.guard_radio = channel
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RADIO_CHANNEL, channel, func() -> void:
                state.radio_channel = channel
                MaszynaDriverHints.send(vehicle, &"radio_channel_set", channel))
    # waiting for the passengers (adjust_desired_speed_for_limits(), Driver.cpp:7550-7555)
    if StationServer.dispatch_get_step(vehicle) == StationServer.DISPATCH_STEP_EXCHANGE:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WAIT_LOAD_EXCHANGE)
    if state.route.waiting_for_departure:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WAIT_DEPARTURE_TIME)
    _control_security_system(situation, elapsed)
    if state.engine_active:
        # control_horns() (Driver.cpp:6348-6368): the horn sounds while there is time left, and
        # stops after
        var horns:MaszynaMoverRailVehicleHorns = VehicleServer.vehicle_component_get(
                vehicle, VehicleComponentType.COMPONENT_HORNS) as MaszynaMoverRailVehicleHorns
        if horns and state.warning_duration > 0.0 and horns.get_horn() == 0:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HORN_ON, DEFAULT_HORN)
        elif horns and state.warning_duration <= 0.0 and not horns.get_horn() == 0:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HORN_OFF)
        # moving, the horn is due before the next start (Driver.cpp:6362-6367)
        if VehicleServer.vehicle_get_speed(vehicle) > MaszynaDriverTrainset.MOVEMENT_SPEED:
            state.start_horn_done = false
            state.start_horn = true
        # the horn before moving off, once the power comes on (Driver.cpp:6369-6377)
        var master_controller:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
                state.trainset.controlling, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
        if state.start_horn_now and state.trainset.ready and master_controller \
                and master_controller.get_main_position() > master_controller.get_main_no_power_position():
            state.warning_duration = START_HORN_DURATION
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.HORN_ON, DEFAULT_HORN)
            state.start_horn_done = true
            state.start_horn_now = false
        MaszynaDriverPantographs.control(
                situation, MaszynaDriverBraking.is_emu(vehicle), state.traction.action_time <= 0.0)
        # the delayed actions: the lights of its order kept up (Driver.cpp:4933-4936)
        if state.traction.action_time > 0.0:
            MaszynaDriverLights.control(situation)
    # the controllers held by time back to holding, the power and the brakes, the controllers
    # held by time set to work until the next update (UpdateSituation(), Driver.cpp:5016-5027) -
    # the controllers held by time are the computer's own (CheckTimeControllers(),
    # SetTimeControllers(), Driver.cpp:4050, 4262)
    if in_control:
        state.traction.check_time_controllers(situation)
        state.braking.check_time_controllers(situation)
    # the doors closed before the power comes on, told to go (increase_tractive_force(),
    # Driver.cpp:8052-8057): a door of the trainset open, or the driver's permit given
    var own_doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    var doors_open:bool = own_doors != null and (own_doors.get_left_open_permit() or own_doors.get_right_open_permit())
    for car:RID in state.trainset.vehicles:
        var car_doors:RailVehicleDoors = VehicleServer.vehicle_component_get(car, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
        doors_open = doors_open or (car_doors != null and (car_doors.get_left_open() or car_doors.get_right_open()))
    if doors_open and not state.departure_warned and state.speed.velocity_desired > 0.0 \
            and state.speed.acceleration_desired > MaszynaDriverSpeed.NO_ACCELERATION \
            and state.traction.action_time >= 0.0:
        _start_closing_doors(situation)
    # differs from the original, which takes the permits back only as its own driver departs
    # (Driver.cpp:4389-4392): an EIM controller asked for power gives none while a door permit stands
    # (eimic_max, Mover.cpp:6758-6769) - the permits a deactivated cab grants (InactiveCabFlag,
    # Mover.cpp:2690-2694) left a player in the other cab at full power, standing, told nothing
    if own_doors and (own_doors.get_left_open_permit() or own_doors.get_right_open_permit()) \
            and _asks_for_power(situation):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_OFF)
    # the same for the independent brake of a car left: it brakes that car whatever the cab driving
    # (the player sent there by the standing driver's hints before going to the other cab)
    if _asks_for_power(situation):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_RELEASE_OTHER_END)
    state.braking.control_main_pipe(situation)
    if state.traction.prepare(situation):
        state.traction.control(situation)
        state.braking.control(situation, elapsed)
    if in_control:
        state.traction.set_time_controllers(situation)
        state.braking.set_time_controllers(situation)
    var standing:bool = VehicleServer.vehicle_get_speed(vehicle) < NO_MOVEMENT_SPEED
    _handle_engine(situation)
    if state.orders[state.order_position] == Order.RELEASE_ENGINE and standing:
        if _release_engine(situation):
            _jump_to_next_order(state, vehicle)
    # coupling and uncoupling hinted to a player too, the couplers joined and undone by the
    # computer only (UpdateConnect(), UpdateDisconnect(), Driver.cpp:7016, 7101-7106); turning is the
    # computer's own (UpdateChangeDirection(), Driver.cpp:6899)
    match state.orders[state.order_position]:
        Order.CONNECT:
            _update_connect(situation)
        Order.DISCONNECT:
            _update_disconnect(situation)
    if state.orders[state.order_position] & Order.CHANGE_DIRECTION and standing:
        if in_control:
            _activation(state)
        if state.direction == state.direction_order:
            _prepare_engine(_read_situation(state))
            _jump_to_next_order(state, vehicle)
    DriverServer.driver_schedule_update(driver, state.reaction_time)


## What one decision of the driver works with (MaszynaDriverTraction.Situation), read afresh -
## the cab after the crew moved to another is the new one
func _read_situation(state:AIDriverState) -> MaszynaDriverTraction.Situation:
    var vehicle:RID = VehicleServer.person_get_vehicle(state.driver)
    var situation:MaszynaDriverTraction.Situation = MaszynaDriverTraction.Situation.new()
    situation.state = state
    situation.traction = state.traction
    situation.vehicle = vehicle
    situation.cabin = VehicleServer.person_get_cabin(state.driver)
    situation.controlling = state.trainset.controlling
    situation.order = state.orders[state.order_position]
    situation.speed = state.speed
    situation.trainset = state.trainset
    situation.route = state.route
    situation.braking = state.braking
    # DirectionalVel(), Driver.h:312: the speed, negative when it runs against the way it drives
    situation.directional_speed = VehicleServer.vehicle_get_speed(vehicle) \
            * signf(state.direction * VehicleServer.vehicle_get_velocity(vehicle))
    situation.pressing = state.pressing
    situation.coupling = state.coupling_vehicle.is_valid() and bool(situation.order & Order.CONNECT)
    return situation


## handle_engine() (Driver.cpp:7223-7244): the engine's orders - a vehicle somebody powered up gets
## ready to drive, and the driving orders follow once it is
func _handle_engine(situation:MaszynaDriverTraction.Situation) -> void:
    var state:AIDriverState = situation.state
    # the original's HACK (Driver.cpp:7226-7231)
    if state.orders[state.order_position] == Order.WAIT_FOR_ORDERS and not state.engine_active \
            and _power24_available(situation.vehicle):
        _order_next(state, Order.PREPARE_ENGINE)
    if state.orders[state.order_position] == Order.PREPARE_ENGINE:
        if _prepare_engine(situation):
            _jump_to_next_order(state, situation.vehicle)
    if state.orders[state.order_position] & DRIVING_ORDERS and not state.engine_active:
        _prepare_engine(situation)


## PrepareEngine() (Driver.cpp:2759-2916): the steps that get the vehicle ready, cued on every
## update until it is; what the cab does not have yet is left out (TODO.md, "Drivers")
func _prepare_engine(situation:MaszynaDriverTraction.Situation) -> bool:
    var state:AIDriverState = situation.state
    var vehicle:RID = situation.vehicle
    var speed:float = VehicleServer.vehicle_get_speed(vehicle)
    state.reaction_time = PREPARE_TIME if speed < ROLLING_START_SPEED else EASY_REACTION_TIME
    var controlling:RID = state.trainset.controlling
    # every vehicle under control, as the update's readiness test reads them - not the controlling
    # one alone: an EMU's other motor car with its line breaker open left the vehicle "not ready"
    # with nothing missing (IsAnyConverterOverloadRelayOpen, IsAnyLineBreakerOpen, Driver.cpp:2828,
    # 2834, 2893)
    var converter_overload:bool = state.trainset.converter_overload_relay_open
    var mains:bool = not state.trainset.line_breaker_open
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BATTERY_ON)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CAB_ACTIVATION)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RADIO_ON)
    if _has_diesel_engine(vehicle):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.OIL_PUMP_ON)
        # PrepareHeating() (Driver.cpp:5032-5085): the water heater wanted while the water or the
        # oil is too cold, or always when it switches itself off - the water pump working first
        state.heating_temperature_too_low = false
        var diesel:RailVehicleDieselEngine = VehicleServer.vehicle_component_get(
                state.trainset.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleDieselEngine
        if diesel:
            var margin:float = HEATING_HYSTERESIS if diesel.get_water_heater_active() else 0.0
            state.heating_temperature_too_low = (
                    (diesel.cooling_water_min_temperature > 0.0
                        and diesel.get_main_circuit_water_temperature() < diesel.cooling_water_min_temperature + margin)
                    or (diesel.cooling_water_aux_min_temperature > 0.0
                        and diesel.get_auxiliary_circuit_water_temperature() < diesel.cooling_water_aux_min_temperature + margin)
                    or (diesel.cooling_oil_min_temperature > 0.0
                        and diesel.get_oil_temperature() < diesel.cooling_oil_min_temperature + margin))
            if diesel.cooling_heater_max_temperature > 0.0 or state.heating_temperature_too_low:
                if not diesel.get_water_pump_active():
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_PUMP_BREAKER_ON)
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_PUMP_ON)
                if diesel.get_water_pump_active():
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_HEATER_BREAKER_ON)
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_HEATER_ON)
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_CIRCUITS_LINK_ON)
            else:
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_CIRCUITS_LINK_OFF)
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_HEATER_OFF)
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_HEATER_BREAKER_OFF)
                if not diesel.water_pump_start_mode == RailVehicleController.START_MODE_BATTERY:
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_PUMP_OFF)
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_PUMP_BREAKER_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WAIT_TEMPERATURE_TOO_LOW if state.heating_temperature_too_low else MaszynaDriverHints.Hint.FUEL_PUMP_ON)
    # the pantographs' air, and both up (Driver.cpp:2782-2811)

    MaszynaDriverPantographs.prepare(situation, MaszynaDriverBraking.is_emu(vehicle))
    _prepare_direction(situation)
    # the main circuit, the converter and the air are the engine's the controls drive - an EMU's
    # motor car (mvControlling, Driver.cpp:2827-2878)
    if converter_overload:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.COMPRESSOR_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CONVERTER_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.PRIMARY_CONVERTER_OVERLOAD_RESET)
    if not mains:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MAIN_CIRCUIT_GROUND_RESET)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRACTION_MOTOR_OVERLOAD_RESET)
        # a diesel with a gearbox starts at its idle position, or it stalls (Driver.cpp:2840-2843)
        var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
                vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
        if engine and engine.get_type() == RailVehicleEngine.DIESEL:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_IDLE)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.LINE_BREAKER_CLOSE)
    else:
        if not converter_overload:
            # a converter only where there is one; the compressor once it is on (Driver.cpp:2851-2857)
            if state.trainset.converter_present \
                    and MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CONVERTER_ON):
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.COMPRESSOR_ON)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WAIT_PRESSURE_TOO_LOW)
            # the traction motors' blowers that run at all (Driver.cpp:2860-2866)
            var configuration:MaszynaMoverEngineConfiguration = MaszynaDriverTraction.engine_configuration(
                    VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine)
            if configuration and not configuration.motor_blowers_speed == 0.0:
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.FRONT_MOTOR_BLOWERS_ON)
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.REAR_MOTOR_BLOWERS_ON)
        # the driver's level taken from the handle, then the handle off running released - the
        # original's inner condition ends in a stray `;`, so the control pressure does not matter
        # (Driver.cpp:2867-2884)
        situation.braking.take_handle_position(vehicle)
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        if brake and not brake.get_controller_position() == brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE):
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRAIN_BRAKE_RELEASE, 0.0,
                    situation.braking.release_train_brake)
        var spring_brake:RailVehicleSpringBrake = RailVehicleServer.vehicle_component_get(
                vehicle, RailVehicleComponentType.COMPONENT_SPRING_BRAKE) as RailVehicleSpringBrake
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SPRING_BRAKE_ON if spring_brake and spring_brake.get_active() else MaszynaDriverHints.Hint.SPRING_BRAKE_OFF)
    var missing:int = 0
    if converter_overload:
        missing |= EngineCheck.CONVERTER_OVERLOAD
    if not mains:
        missing |= EngineCheck.LINE_BREAKER
    if VehicleServer.vehicle_get_controller(vehicle).get_direction() == VehicleController.DIRECTION_NEUTRAL:
        missing |= EngineCheck.DIRECTION
    if state.trainset.converter_present and not state.trainset.converter_enabled:
        missing |= EngineCheck.CONVERTER
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            controlling, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    # no air wanted without a main reservoir (VeselVolume == 0.0, Driver.cpp:2897)
    if brake and not is_zero_approx(brake.tank_volume_main) \
            and brake.get_compressor_pressure() <= MIN_MAIN_RESERVOIR_PRESSURE:
        missing |= EngineCheck.AIR
    state.engine_missing = missing
    state.engine_active = missing == 0
    return state.engine_active


## ReleaseEngine() (Driver.cpp:2918-3012): the vehicle put away, standing, step by step; done
## when it is dead
func _release_engine(situation:MaszynaDriverTraction.Situation) -> bool:
    var state:AIDriverState = situation.state
    var vehicle:RID = situation.vehicle
    state.reaction_time = PREPARE_TIME
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RELEASER_OFF)
    # held by its own brake on the flat, by the train brake on a slope
    if absf(state.trainset.gravity_acceleration) < MaszynaDriverBraking.FLAT_GRAVITY:
        state.braking.apply_independent_brake_only(situation)
    else:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRAIN_BRAKE_APPLY)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_NONE)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_RIGHT_CLOSE)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DOOR_LEFT_CLOSE)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CONSIST_HEATING_OFF)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.COMPRESSOR_OFF)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CONVERTER_OFF)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.LINE_BREAKER_OPEN)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_OFF)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_OFF)
    var controlling:RailVehicleEngine = VehicleServer.vehicle_component_get(
            state.trainset.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    if not (controlling and controlling.get_main_switch_enabled()):
        if _has_diesel_engine(vehicle):
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_HEATER_OFF)
            var diesel:RailVehicleDieselEngine = controlling as RailVehicleDieselEngine
            if diesel and not diesel.water_pump_start_mode == RailVehicleController.START_MODE_BATTERY:
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WATER_PUMP_OFF)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.FUEL_PUMP_OFF)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.OIL_PUMP_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.LIGHTS_OFF, 0.0, func() -> void: MaszynaDriverLights.off(situation))
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SPRING_BRAKE_ON)
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        if brake and (brake.cntrl_local_brake_type == RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL
                or brake.cntrl_manual_brake_present):
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MANUAL_BRAKE_ON)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RADIO_OFF)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BATTERY_OFF)
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var released:bool = VehicleServer.vehicle_get_controller(vehicle).get_direction() == VehicleController.DIRECTION_NEUTRAL \
            and not (engine and engine.get_main_switch_enabled()) \
            and not _power24_available(vehicle)
    if released:
        state.engine_active = false
        _order_next(state, Order.WAIT_FOR_ORDERS)
    return released


## Activation() (Driver.cpp:2051-2145), the computer's own: turning a standing vehicle - the controls
## zeroed, the crew to the cab facing the new way (CabOccupied = iDirection), that cab switched on
## and its reverser forward. A move to another vehicle of the trainset is not ported (TODO.md).
func _activation(state:AIDriverState) -> void:
    var driver:RID = state.driver
    var vehicle:RID = VehicleServer.person_get_vehicle(driver)
    state.direction = state.direction_order
    var situation:MaszynaDriverTraction.Situation = _read_situation(state)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_NONE)
    # the crew straight to the cab of the new way, as the original sets CabOccupied = iDirection
    # (Driver.cpp:2067-2068, 2120) - the cab switched off by the vehicle, not through the cab
    if not _cabin_direction(driver) == state.direction:
        VehicleServer.vehicle_send_command(vehicle, "cab_activation", false)
        if state.direction > 0:
            RailVehicleServer.person_move_to_front_cabin(driver)
        else:
            RailVehicleServer.person_move_to_rear_cabin(driver)
    situation = _read_situation(state)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CAB_ACTIVATION)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_FORWARD)


## UpdateConnect() (Driver.cpp:6993-7055): within CONNECT_DISTANCE of the vehicle ahead the front
## vehicle of the trainset starts coupling up; within ATTACH_DISTANCE the shunter of a driver the
## computer is joins an element a time - the vehicle's `coupler_connect`, as the player's crew does,
## a player couples by hand - and once every element
## asked for is joined, the next order follows. Within ADAPTER_DISTANCE an end that is no automatic
## coupler facing an automatic one takes its adapter first, and couples once it has it
## (couplingadapterattach, Driver.cpp:6895-6912; driverhints.cpp:1215-1224).
func _update_connect(situation:MaszynaDriverTraction.Situation) -> void:
    var state:AIDriverState = situation.state
    var vehicle:RID = situation.vehicle
    if not state.coupling_vehicle.is_valid():
        if state.route.obstacle and state.route.obstacle.distance <= CONNECT_DISTANCE and state.trainset.vehicles:
            state.coupling_vehicle = state.trainset.vehicles[0]
            state.coupling_end = (RailVehicleController.COUPLER_END_FRONT if state.trainset.front_direction > 0
                    else RailVehicleController.COUPLER_END_REAR)
        return
    if not _is_coupled_as_asked(state.coupling_vehicle, state.coupling_end, state.coupler):
        var neighbour:RailVehicleNeighbour = RailVehicleServer.vehicle_find_vehicle(
                state.coupling_vehicle, state.coupling_end, MaszynaDriverRoute.OBSTACLE_RANGE)
        # the coupling end's neighbour as the vehicle keeps it (Neighbours[], Driver.cpp:7002): near,
        # between the couplers less their adapters, not between the primary axles (DynObj.cpp:7577)
        if neighbour and neighbour.distance < MaszynaDriverRoute.OBSTACLE_COUPLER_RANGE:
            neighbour.distance = RailVehicleServer.vehicle_get_coupler_distance(
                    state.coupling_vehicle, neighbour.vehicle_rid) \
                    - RailVehicleServer.vehicle_get_coupler_adapter_length(state.coupling_vehicle, state.coupling_end) \
                    - RailVehicleServer.vehicle_get_coupler_adapter_length(neighbour.vehicle_rid, neighbour.end)
        var compatible:bool = true
        if neighbour and neighbour.distance < ADAPTER_DISTANCE \
                and not RailVehicleServer.vehicle_is_coupler_automatic(state.coupling_vehicle, state.coupling_end) \
                and RailVehicleServer.vehicle_is_coupler_automatic(neighbour.vehicle_rid, neighbour.end):
            compatible = false
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.COUPLING_ADAPTER_ATTACH, state.coupling_end, func() -> void:
                MaszynaDriverHints.send(state.coupling_vehicle, "coupler_adapter_attach", state.coupling_end))
        if compatible and neighbour and neighbour.distance < ATTACH_DISTANCE \
                and DriverServer.vehicle_is_control_active(vehicle):
            MaszynaDriverHints.send(state.coupling_vehicle, "coupler_connect", state.coupling_end)
    # the command joins at once: coupled now, it drives on
    if _is_coupled_as_asked(state.coupling_vehicle, state.coupling_end, state.coupler):
        state.coupler = RailVehicleController.COUPLING_FLAG_NONE
        state.coupling_vehicle = RID()
        _jump_to_next_order(state, vehicle)


## UpdateDisconnect() (Driver.cpp:7101-7235): leaving all but `vehicle_count` vehicles from the
## driver's. The train braked and the direction turned (2nd stage); the buffers pressed, the brakes
## of the vehicles released and the coupler undone by the shunter - the vehicles' `brake_releaser`
## and `coupler_disconnect`, as the player's crew does - (3rd); the direction restored and the next
## order (4th, 5th) - hinted to a player too, the coupler undone by the computer only.
func _update_disconnect(situation:MaszynaDriverTraction.Situation) -> void:
    var state:AIDriverState = situation.state
    var vehicle:RID = situation.vehicle
    var in_control:bool = DriverServer.vehicle_is_control_active(vehicle)
    if state.vehicle_count >= 0:
        # a player may have uncoupled already, or before the order: as many units left as are to
        # stay, it is done (Driver.cpp:7104-7111) - the units from the driver's vehicle to the end,
        # vehicles joined for good counted as one (unit_count(), Driver.cpp:7079-7099)
        if not in_control:
            var vehicles:Array[RID] = state.trainset.vehicles
            var units:int = 1
            for index:int in range(vehicles.find(vehicle) + 1, vehicles.size()):
                if not _is_coupled_by(vehicles[index], _end_towards_front(state.trainset, index),
                        RailVehicleController.COUPLING_FLAG_PERMANENT):
                    units += 1
            if units <= state.vehicle_count + 1:
                state.vehicle_count = -2
                return
        if not state.direction == state.direction_order:
            _reverse(situation)
        # pressing and uncoupling only once the trainset was read the way it now drives: in the
        # update that turned, it is still the old front, and the walk from the locomotive found its
        # free coupler and took the uncoupling as done (FINDINGS.md, 2026-09-27)
        if state.pressing and state.direction == state.direction_order and state.trainset.direction == state.direction:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_RELEASE)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BUFFERS_COMPRESS, 0.0, state.traction.press.bind(situation))
            # from the driver's vehicle into the ones pressed, as many as stay; a unit counts once
            var vehicles:Array[RID] = state.trainset.vehicles
            var index:int = vehicles.find(vehicle)
            var count:int = state.vehicle_count
            var decoupled:RID = RID()
            var end:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
            while index >= 0:
                var current:RID = vehicles[index]
                end = _end_towards_front(state.trainset, index)
                if _is_coupled_by(current, end, RailVehicleController.COUPLING_FLAG_PERMANENT):
                    count += 1
                if not current == vehicle:
                    # released, to be pressed together
                    MaszynaDriverHints.send(current, "brake_releaser", true)
                if count == 0:
                    decoupled = current
                    break
                index -= 1
                count -= 1
            if not decoupled.is_valid():
                # nothing there to uncouple
                state.vehicle_count = -2
            else:
                # refused until the buffers are pressed enough: it presses on
                if in_control:
                    MaszynaDriverHints.send(decoupled, "coupler_disconnect", end)
                if not _is_coupled_by(decoupled, end, RailVehicleController.COUPLING_FLAG_COUPLER):
                    state.vehicle_count = -2
                    # an adapter the front vehicle's end was fitted with comes off (couplingadapterremove,
                    # Driver.cpp:7068-7070; driverhints.cpp:1226-1232)
                    var front:RID = state.trainset.vehicles[0]
                    if RailVehicleServer.vehicle_get_coupler_adapter_model(front, end):
                        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.COUPLING_ADAPTER_REMOVE, end, func() -> void:
                            MaszynaDriverHints.send(front, "coupler_adapter_remove", end))
        if not state.pressing:
            if state.direction_backup == 0:
                state.direction_backup = state.direction
            if not state.trainset.braked:
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRAIN_BRAKE_APPLY)
            else:
                state.direction_order = -state.direction
                state.pressing = true
    if state.vehicle_count < 0:
        if not state.direction_backup == 0:
            state.direction_order = state.direction_backup
            state.direction_backup = 0
        if not state.direction == state.direction_order:
            _reverse(situation)
        if state.direction == state.direction_order:
            state.pressing = false
            _jump_to_next_order(state, vehicle)
            # no horn before moving off (Driver.cpp:7217)
            state.start_horn = false


## SetVelocity()'s horn before moving off (Driver.cpp:2700-2710): a train standing, told to go, with
## the horn due and not sounded yet - and not coupling up
func _cue_start_horn(state:AIDriverState, vehicle:RID, velocity:float) -> void:
    var driving:int = (Order.SHUNT | Order.LOOSE_SHUNT | Order.OBEY_TRAIN | Order.BANK | Order.CONNECT
            | Order.PREPARE_ENGINE)
    if state.orders[state.order_position] & driving and state.start_horn and not state.start_horn_done \
            and not state.coupling_vehicle.is_valid() \
            and VehicleServer.vehicle_get_speed(vehicle) < MaszynaDriverTrainset.MOVEMENT_SPEED \
            and (velocity >= MaszynaDriverTrainset.MOVEMENT_SPEED or velocity < 0.0):
        state.start_horn_now = true


## directionother (driverhints.cpp:935-946): the reverser the other way from the same cab, the
## master controller down first; the driver's direction follows once the reverser has moved
## (DirectionChange())
func _reverse(situation:MaszynaDriverTraction.Situation) -> void:
    var state:AIDriverState = situation.state
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_OTHER)
    if VehicleServer.vehicle_get_controller(situation.vehicle).get_direction() \
            == state.direction_order * _active_cab(situation.vehicle):
        state.direction = state.direction_order


## control_security_system() (Driver.cpp:6380-6431): the cab signal and the vigilance acknowledged
## while they flash, the reverser forward first if it stands at neutral - not on an EMU; after a
## Radio-Stop the radio off a while after the train stood, and on again a while after the stop is
## lifted, `elapsed` [s] after the last update. The train brake the security system applied is
## released by the driving (MaszynaDriverBraking).
func _control_security_system(situation:MaszynaDriverTraction.Situation, elapsed:float) -> void:
    var state:AIDriverState = situation.state
    var vehicle:RID = situation.vehicle
    var security:RailVehicleSecuritySystem = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_SECURITY) as RailVehicleSecuritySystem
    if security:
        var neutral:bool = VehicleServer.vehicle_get_controller(vehicle).get_direction() == VehicleController.DIRECTION_NEUTRAL \
                and not MaszynaDriverBraking.is_emu(vehicle)
        if security.get_cabsignal_blinking() and security.get_separate_acknowledge():
            if neutral:
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_FORWARD)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SHP_SYSTEM_RESET)
        if security.get_blinking():
            if neutral:
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DIRECTION_FORWARD)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SECURITY_SYSTEM_RESET)
    var radio:RailVehicleRadio = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio
    if radio == null:
        return
    if radio.get_radio_stop_active() and radio.get_enabled() \
            and VehicleServer.vehicle_get_speed(vehicle) < RADIO_STOP_STANDING_SPEED:
        if state.radio_control_time > RADIO_STOP_DELAY:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RADIO_OFF)
        else:
            state.radio_control_time += elapsed
    if state.engine_active and not radio.get_enabled() and not radio.get_radio_stop_active():
        state.radio_control_time = minf(state.radio_control_time, RADIO_STOP_DELAY)
        if state.radio_control_time < 0.0:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RADIO_ON)
        else:
            state.radio_control_time -= elapsed


static func _has_diesel_engine(vehicle:RID) -> bool:
    return VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_ENGINE) is RailVehicleDieselEngine


## The cab the driver sits in, as the original numbers it (CabOccupied, iDirection): the front 1,
## the rear -1, the machine room or none 0 (Train.cpp:8684)
static func _cabin_direction(driver:RID) -> int:
    match RailVehicleServer.cabin_get_kind(VehicleServer.person_get_cabin(driver)):
        RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT:
            return 1
        RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR:
            return -1
    return 0


## Whether the master controller of an EIM controller stands where it adds power: a universal
## controller's position with a target above none (UniCtrlList[].SetCtrlVal), any other past its
## positions of no power
static func _asks_for_power(situation:MaszynaDriverTraction.Situation) -> bool:
    if MaszynaDriverTraction.eim_control_type(situation) == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
        return false
    var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(situation.controlling)
    if master == null:
        return false
    var universal:MaszynaMoverRailVehicleUniversalController = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as MaszynaMoverRailVehicleUniversalController
    var position:int = master.get_main_position()
    if universal and position < universal.positions.size():
        return (universal.positions[position] as MaszynaMoverUniversalControllerListItem).target_value > 0.0
    return position > master.get_main_no_power_position()


## The vehicle's active cab (CabActive) - none on a vehicle without a master controller
static func _active_cab(vehicle:RID) -> int:
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    return master.get_cabin() if master else 0


## The vehicle's low voltage there - none on a vehicle without a power supply
static func _power24_available(vehicle:RID) -> bool:
    var power_supply:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    return power_supply != null and power_supply.get_power24_available()


## `Timetable:<name> <velocity> <minutes>` (Driver.cpp:4494-4576): the timetable, the first station
## to drive to, the direction towards where the order came from and the orders it makes
func _take_timetable(
    driver:RID, state:AIDriverState, name:String, velocity:float, minutes:float, position:Vector3
) -> void:
    var timetable:RID = RID()
    if not name == NO_TIMETABLE:
        var directory:String = MaszynaGameDirectory.get_game_dir().path_join(SCENERY_DIRECTORY)
        timetable = MaszynaTimetableFactory.load_timetable(directory, name)
    state.timetable.take(timetable, RailVehicleServer.vehicle_get_trainset(VehicleServer.person_get_vehicle(driver)),
            roundf(minutes))
    # the guard of the timetable, speaking on the radio (Driver.cpp:4424, 4474-4483)
    state.guard_signal = null
    state.guard_transcript = null
    state.guard_radio = 0
    state.guard_signal_due = false
    if timetable.is_valid():
        var path:String = ""
        var radio:bool = false
        for suffix:String in ["", GUARD_RADIO_SUFFIX]:
            for directory:String in [SCENERY_DIRECTORY, SOUNDS_DIRECTORY]:
                for extension:String in GUARD_SOUND_EXTENSIONS:
                    var base_dir:String = MaszynaGameDirectory.get_game_dir().path_join(directory)
                    var filename:String = "%s%s.%s" % [name, suffix, extension]
                    var candidate:String = base_dir.path_join(MaszynaDataPath.resolve(base_dir, filename))
                    if not path and FileAccess.file_exists(candidate):
                        path = candidate
                        radio = suffix == GUARD_RADIO_SUFFIX
        var stream:AudioStream = null
        if radio and path.get_extension() == OGG_EXTENSION:
            stream = AudioStreamOggVorbis.load_from_file(path)
        elif radio and path.get_extension() == WAV_EXTENSION:
            stream = AudioStreamWAV.load_from_file(path)
        if stream:
            var clip:SfxClip = SfxClip.new()
            clip.stream = stream
            var clips:Array[SfxClip] = [clip]
            state.guard_signal = SfxEvent.new()
            state.guard_signal.clips = clips
            var emitter:Array[SfxEvent] = [state.guard_signal]
            MmdSoundEventBuilder.shape_emitter(emitter, null, 0.0)
            # its caption beside it (openal_buffer::fetch_caption(), audio.cpp:84-94)
            state.guard_transcript = MaszynaSoundCaption.from_sound_file(path.get_basename())
            state.guard_radio = state.radio_channel if state.radio_channel > 0 else RADIO_CHANNEL_DEFAULT
    if not position == Vector3.ZERO:
        state.direction_order = _direction_towards(driver, position, velocity)
    _orders_init(state, VehicleServer.person_get_vehicle(driver), absf(velocity))


## The orders a timetable makes (OrdersInit(), Driver.cpp:5238-5319): start the engine, then shunt
## without a timetable, or drive it - turning where a station says `@` - and shunt after
func _orders_init(state:AIDriverState, vehicle:RID, velocity:float) -> void:
    _orders_clear(state)
    _order_push(state, Order.PREPARE_ENGINE)
    var count:int = state.timetable.get_entry_count()
    if count == 0:
        _order_push(state, Order.SHUNT)
    else:
        if velocity > 0.0 and velocity < SHUNT_START_VELOCITY_MAX:
            _order_push(state, Order.SHUNT)
        else:
            _order_push(state, Order.OBEY_TRAIN)
        for index:int in count:
            if StationServer.timetable_entry_is_direction_change(state.timetable.timetable, index):
                # a train of wagons leaves its locomotive; a push-pull set only turns - see TODO.md
                _order_push(state, Order.DISCONNECT)
                _order_push(state, Order.SHUNT)
                if index < count - 1:
                    _order_push(state, Order.OBEY_TRAIN)
        _order_push(state, Order.SHUNT)
    if velocity == 0.0:
        state.velocity = 0.0
        return
    state.stop_here = not (velocity >= 1.0 or velocity < SHUNT_START_VELOCITY_MAX)
    if not state.stop_here:
        # told to go: it draws up close to the next passenger stop (Driver.cpp:5305-5309)
        state.timetable.draw_up_close()
    _jump_to_first_order(state, vehicle)
    state.velocity = velocity if velocity >= 1.0 else 0.0


## `Shunt`/`Loose_shunt <vehicles> <coupler>` (Driver.cpp:4749-4867): couple up by the coupler, leave
## the vehicles counted, and shunt - or drive as a train - after
func _take_shunt(driver:RID, state:AIDriverState, loose:bool, vehicles:float, coupler:float) -> void:
    state.stop_here = false
    if not state.engine_active:
        _order_next(state, Order.PREPARE_ENGINE)
    if not coupler == 0.0:
        state.coupler = floori(absf(coupler)) as RailVehicleController.CouplingFlags
        _order_next(state, Order.CONNECT)
        if vehicles >= 0.0:
            # after coupling, pull away the vehicles counted: turn first, they are behind
            state.direction_order = -state.direction
            _order_push(state, Order.CHANGE_DIRECTION)
            _order_push(state, Order.DISCONNECT)
            if coupler > 0.0 and loose:
                # after leaving them, carry on pushing the way it came
                state.direction_order = state.direction
                _order_push(state, Order.CHANGE_DIRECTION)
        elif coupler < 0.0:
            state.direction_order = -state.direction
            _order_next(state, Order.CHANGE_DIRECTION)
    elif vehicles >= 0.0:
        var vehicle:RID = VehicleServer.person_get_vehicle(driver)
        var forward_end:RailVehicleController.CouplerEnd = (RailVehicleController.COUPLER_END_FRONT
                if state.direction > 0 else RailVehicleController.COUPLER_END_REAR)
        var behind:bool = _is_coupled_by(
                vehicle, RailVehicleController.opposite_end(forward_end), RailVehicleController.COUPLING_FLAG_COUPLER)
        var ahead:bool = _is_coupled_by(vehicle, forward_end, RailVehicleController.COUPLING_FLAG_COUPLER)
        if not behind and ahead:
            # the vehicles are in front: turn first, then leave them
            state.direction_order = -state.direction
            _order_next(state, Order.CHANGE_DIRECTION)
            _order_push(state, Order.DISCONNECT)
        elif behind:
            _order_next(state, Order.DISCONNECT)
        else:
            # nothing on either side: stand where it is (a use the old sceneries make of it)
            state.velocity = 0.0
            state.stop_here = true
    if vehicles < WAIT_FOR_SIGNAL:
        state.stop_here = true
    var next:int
    if vehicles < TRAIN_AFTER_SHUNT:
        next = Order.BANK if loose else Order.OBEY_TRAIN
    else:
        next = Order.LOOSE_SHUNT if loose else Order.SHUNT
    _order_next(state, next)
    state.vehicle_count = floori(vehicles)


## Forwards (+1) or back (-1) along the vehicle, towards where the order came from - or away, for a
## negative value (Driver.cpp:4707-4715)
func _direction_towards(driver:RID, position:Vector3, value:float) -> int:
    var transform:Transform3D = RailVehicleServer.vehicle_get_transform(VehicleServer.person_get_vehicle(driver))
    var towards:Vector3 = position - transform.origin
    var front:Vector3 = -transform.basis.z
    return 1 if (towards.x * front.x + towards.z * front.z) * value > 0.0 else -1


## Whether something is joined at the vehicle's end by every one of `flags` - the walk out through
## that end starts beyond it
static func _is_coupled_by(vehicle:RID, end:RailVehicleController.CouplerEnd, flags:RailVehicleController.CouplingFlags) -> bool:
    var coupled:Array[RID] = RailVehicleServer.vehicle_get_coupled(vehicle, end, flags)
    return not coupled.is_empty() and not coupled[0] == vehicle


## Whether every coupling `coupler` asks for (the original's bits) that the end and its neighbour
## can join is joined; asked for none, it is. The original's Attach() sets the couplings asked for
## whatever the couplers allow (Mover.cpp:576-583) and so ends its UpdateConnect() (Driver.cpp:7028);
## a coupling neither coupler has (a heating line `Shunt -3 -99` asks of an SN61) is not waited for.
## With no neighbour nothing can be joined, and nothing is
static func _is_coupled_as_asked(vehicle:RID, end:RailVehicleController.CouplerEnd,
        coupler:RailVehicleController.CouplingFlags) -> bool:
    var asked:RailVehicleController.CouplingFlags = (coupler & SHUNTER_COUPLINGS) as RailVehicleController.CouplingFlags
    if asked == RailVehicleController.COUPLING_FLAG_NONE:
        return true
    var joinable:RailVehicleController.CouplingFlags = (asked
            & RailVehicleServer.vehicle_get_coupler_joinable_flags(vehicle, end)) as RailVehicleController.CouplingFlags
    return not joinable == RailVehicleController.COUPLING_FLAG_NONE and _is_coupled_by(vehicle, end, joinable)


## The end of the trainset's vehicle at `index` towards its front
static func _end_towards_front(trainset:MaszynaDriverTrainset, index:int) -> RailVehicleController.CouplerEnd:
    if index == 0:
        return (RailVehicleController.COUPLER_END_FRONT if trainset.front_direction > 0
                else RailVehicleController.COUPLER_END_REAR)
    var vehicle:RID = trainset.vehicles[index]
    var coupled:Array[RID] = RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)
    var position:int = coupled.find(vehicle)
    # beyond the front end come first
    return (RailVehicleController.COUPLER_END_FRONT if position > 0 and coupled[position - 1] == trainset.vehicles[index - 1]
            else RailVehicleController.COUPLER_END_REAR)


func _orders_clear(state:AIDriverState) -> void:
    state.order_position = 0
    state.order_top = 1
    state.orders.fill(Order.WAIT_FOR_ORDERS)


## The order to do next: in place of what it does now, or after the operations under way
## (OrderNext(), Driver.cpp:5187-5207)
func _order_next(state:AIDriverState, order:int) -> void:
    if state.orders[state.order_position] == order:
        return
    if state.order_position == 0:
        state.order_position = 1
    state.order_top = state.order_position
    if order >= Order.SHUNT:
        while not state.orders[state.order_top] == Order.WAIT_FOR_ORDERS and state.orders[state.order_top] < Order.SHUNT:
            state.order_top += 1
    else:
        while state.orders[state.order_top] and state.orders[state.order_top] < Order.SHUNT and not state.orders[state.order_top] == order:
            state.order_top += 1
    state.orders[state.order_top] = order
    state.order_top += 1


## An order added after the rest (OrderPush(), Driver.cpp:5209-5220)
func _order_push(state:AIDriverState, order:int) -> void:
    if state.order_position == state.order_top and state.orders[state.order_position] < Order.SHUNT:
        state.order_top += 1
    if not state.orders[state.order_top] == order:
        state.orders[state.order_top] = order
        state.order_top += 1


func _jump_to_next_order(state:AIDriverState, vehicle:RID) -> void:
    var current:int = state.orders[state.order_position]
    if not current == Order.WAIT_FOR_ORDERS:
        if current & Order.CHANGE_DIRECTION and not current == Order.CHANGE_DIRECTION:
            # a change of direction on top of another order goes first
            state.orders[state.order_position] = current & ~Order.CHANGE_DIRECTION
            _order_check(state, vehicle)
            return
        state.order_position = (state.order_position + 1) % MAX_ORDERS
    _order_check(state, vehicle)


## CheckVehicles() (Driver.cpp:2451-2595): the lights set by a driver the computer is
## (AIControllFlag) once its vehicle is ready to drive (iEngineActive) - a player is hinted by the
## driver's update (control_lights()); the door locks on, and the consist's heating as the train
## needs it
func _check_vehicles(state:AIDriverState) -> void:
    var vehicle:RID = VehicleServer.person_get_vehicle(state.driver)
    if not vehicle.is_valid():
        return
    var situation:MaszynaDriverTraction.Situation = _read_situation(state)
    if DriverServer.vehicle_is_control_active(vehicle) and state.engine_active:
        MaszynaDriverLights.check_vehicles(situation)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CONSIST_DOOR_LOCKS_ON)
    # the heating a unit always wants, and a passenger train with heated cars behind its engines
    # joined by the heating line (Driver.cpp:2302, 2471-2484, 2575-2593)
    var trainset:MaszynaDriverTrainset = state.trainset
    var train_type:MaszynaMoverRailVehicleController.TrainType = (
            VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).train_type
    var unit:bool = MaszynaDriverBraking.is_emu(vehicle) or train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_DMU
    var passenger_train:bool = not unit \
            and RailVehicleServer.trainset_get_type(vehicle) == RailVehicleServer.TRAINSET_TYPE_PASSENGER \
            and trainset.vehicles.size() > trainset.controlled_engines
    var heating_line:bool = not trainset.controlled_engines == 1 or (trainset.vehicles and RailVehicleServer.vehicle_get_coupled(
            trainset.vehicles[0], RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_HEATING) \
            .has(trainset.vehicles[-1]))
    var controlled:Array[RID] = RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL)
    var heaters:bool = false
    for other:RID in trainset.vehicles:
        if not other in controlled and (VehicleServer.vehicle_get_controller(other) as RailVehicleController).heating_power > 0.0:
            heaters = true
    var needed:bool = unit or (bool(state.orders[state.order_position] & (Order.OBEY_TRAIN | Order.BANK))
            and heating_line and passenger_train and heaters)
    var heating:RailVehicleHeating = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_HEATING) as RailVehicleHeating
    if heating and not heating.get_allowed() == needed:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.CONSIST_HEATING_ON if needed else MaszynaDriverHints.Hint.CONSIST_HEATING_OFF)


func _jump_to_first_order(state:AIDriverState, vehicle:RID) -> void:
    state.order_position = 1
    state.order_top = maxi(state.order_top, 1)
    _order_check(state, vehicle)


## What a new order changes at once (OrderCheck(), Driver.cpp:5161-5185): the lights of the order
## (CheckVehicles(), Driver.cpp:5084-5092) - the doors it checks belong to the driving (TODO.md)
func _order_check(state:AIDriverState, vehicle:RID) -> void:
    DriverServer.driver_report_order_changed(state.driver)
    var current:int = state.orders[state.order_position]
    if not current == Order.OBEY_TRAIN:
        state.light_hints = Vector2i(MaszynaDriverLights.NO_HINT, MaszynaDriverLights.NO_HINT)
    if current & (Order.SHUNT | Order.LOOSE_SHUNT | Order.CONNECT | Order.OBEY_TRAIN | Order.BANK):
        _check_vehicles(state)
    if current & Order.CHANGE_DIRECTION:
        state.direction_order = -state.direction
    elif current == Order.OBEY_TRAIN:
        state.timetable.mind_stops()
    elif current == Order.CONNECT:
        state.timetable.pass_stops()
    elif current == Order.DISCONNECT:
        # uncoupling the locomotive: nothing stays with it
        state.vehicle_count = maxi(state.vehicle_count, 0)
    elif current == Order.WAIT_FOR_ORDERS:
        _orders_clear(state)
