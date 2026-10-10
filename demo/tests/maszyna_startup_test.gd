extends MaszynaGutTest
class_name MaszynaStartupTest

## A vehicle of a scenery fixture started from cold and moved off the way a player does it: the
## player in its cab, every step a key of the project's input map (MaszynaPlayer._unhandled_input),
## every step checked on the car it acts on through the components' getters. The first step that
## does not come about fails the test with what the three cars show - the step the start stopped at.
## A test of one vehicle is a script of its own (test_zzz_startup_<vehicle>.gd) calling
## run_startup(); .claude/skills/testing/SKILL.md "Start-up test of a vehicle".

## How a vehicle starts - the steps a driver takes, from the cab, to move it off
enum Kind {
    ## a battery, pantographs, a line breaker and a converter (EU07, EP07, EP09, ET22, E6ACT)
    ELECTRIC_LOCOMOTIVE,
    ## the same, driven from a control car: the line breaker, the converter and the air are the motor
    ## car's (vehicle_find_powered), the pantographs their carrier's (36WE, ED78, EN57)
    ELECTRIC_MULTIPLE_UNIT,
    ## oil and fuel pumps, then the engine started by the main switch (SM42, SU45, ST44)
    DIESEL_ELECTRIC,
    ## the same, its engine driving the wheels through a clutch and a gearbox (SM03, SR61)
    DIESEL_MECHANICAL,
    ## a diesel railcar or unit (SA134)
    DIESEL_MULTIPLE_UNIT,
}
## The engines of each kind (RailVehicleEngine.get_type() of the powered car)
const KIND_ENGINES:Dictionary[Kind, Array] = {
    Kind.ELECTRIC_LOCOMOTIVE: [RailVehicleEngine.ELECTRIC_SERIES_MOTOR, RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR],
    Kind.ELECTRIC_MULTIPLE_UNIT: [RailVehicleEngine.ELECTRIC_SERIES_MOTOR, RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR],
    Kind.DIESEL_ELECTRIC: [RailVehicleEngine.DIESEL_ELECTRIC],
    Kind.DIESEL_MECHANICAL: [RailVehicleEngine.DIESEL],
    Kind.DIESEL_MULTIPLE_UNIT: [RailVehicleEngine.DIESEL, RailVehicleEngine.DIESEL_ELECTRIC],
}
## How the cab raises the pantographs (the manual, sterowanie.html)
enum Pantographs {
    ## one switch each, O and P
    SWITCHES,
    ## chosen by O/P in the machine room, raised together from the cab by Ctrl+Shift+O
    ## (pantselected_sw: ET41, EP08)
    SELECTED,
    ## a preset of the selector, Shift+P, set to the valves by its lever (pantselect_sw, pantvalves_sw:
    ## EP05, EU05, ET40, 36WE) - the lever has no key, the hand moves it
    SELECTOR,
}
## The start modes of a device the driver switches on
const MANUAL_STARTS:Array[int] = [
    RailVehicleController.START_MODE_MANUAL, RailVehicleController.START_MODE_MANUAL_WITH_AUTO_FALLBACK]
## Simulated seconds a pantograph switch is held
const PANTOGRAPH_SWITCH_HOLD:float = 2.0
## Presets the selector is turned through at most to find one raising a pantograph
const MAX_PANTOGRAPH_PRESETS:int = 8
## The pantograph controls of a selector cab (LegacyCabinPantographPresets)
const VALVES_LEVER:StringName = &"pantvalves_sw"
## The kinds driven as a unit (FIZ Type=ezt, dmu)
const UNIT_KINDS:Array[Kind] = [Kind.ELECTRIC_MULTIPLE_UNIT, Kind.DIESEL_MULTIPLE_UNIT]
const UNIT_TRAIN_TYPES:int = RailVehicleController.TRAIN_TYPE_EZT | RailVehicleController.TRAIN_TYPE_DMU
## Simulated seconds a driver gives each notch of a controller without resistor steps
const CONTROLLER_NOTCH_SECONDS:float = 1.0
## Simulated seconds the line contactors may take beyond the controller's InitialCtrlDelay at its
## first power position (Mover.cpp:6394-6401) - a relay step of the unit's cars on top
const LINE_CONTACTORS_MARGIN:float = 5.0

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## Simulated seconds a step may take
const STEP_TIMEOUT:float = 120.0
## Simulated seconds the air may take - the main reservoir and the brake pipe filling from cold; a
## compressor driven by an idling diesel is slow (EngineRPMRatio, Mover.cpp:4424)
const AIR_TIMEOUT:float = 300.0
## Simulated seconds main_switch_toggle (M) is held beyond the line breaker's InitialCtrlDelay
## (LegacyCabinMainSwitch closes it on the release once the delay has run)
const MAIN_SWITCH_HOLD_MARGIN:float = 1.0
## The main reservoir the driver waits for before releasing the brakes (Driver.cpp PrepareEngine
## ready check, the game's maszyna_legacy_ai_driver.gd EngineCheck.AIR) [bar]
const READY_AIR_PRESSURE:float = 4.5
## The universal brake buttons of a cab (universalbrake1_bt..3_bt) and the flag of theirs that
## unlocks the brake pipe (TUniversalBrake::ub_UnlockPipe, hamulce.h:151)
const UNIVERSAL_BRAKE_BUTTONS:Array[int] = [1, 2, 3]
const UNLOCK_PIPE:int = 0x02
## A released train brake: the brake pipe near its 5 bar running pressure [bar]
const RELEASED_PIPE_PRESSURE:float = 4.5
## ... or this share of the vehicle's own running pressure, whichever is lower
const RELEASED_PIPE_SHARE:float = 0.9
## A released brake: its cylinders empty [bar]
const RELEASED_CYLINDER_PRESSURE:float = 0.1
## Moved off [km/h]
const MOVED_OFF_SPEED:float = 1.0
## Master controller steps the driver gives at most before it pulls
const MAX_CONTROLLER_STEPS:int = 10

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _player:MaszynaPlayer
## The car whose cab the player sits in
var occupied:RID
## The car whose engine it controls (RailVehicleServer.vehicle_find_powered)
var powered:RID
## The car whose pantographs it raises (RailVehicleServer.vehicle_find_pantograph_carrier)
var carrier:RID


func after_each() -> void:
    if _player:
        _player.free()
        _player = null
    if _scenery:
        get_tree().process_frame.disconnect(_acknowledge_security)
        _scenery.free()
        _scenery = null
        UserSettings.save_maszyna_game_dir(_previous_game_dir)
        # at once, not running down - the next start-up begins from the clock a lone one finds
        SimulationServer.simulation_reset_speed()


## Loads the scenery fixture, puts the player in `vehicle`'s cab and starts it as a `kind`
func run_startup(scenery:String, vehicle:String, kind:Kind, pantographs:Pantographs = Pantographs.SWITCHES) -> void:
    if not await enter_vehicle(scenery, vehicle):
        return
    var engine_type:int = _engine(powered).get_type() if _engine(powered) else RailVehicleEngine.NONE
    assert_has(KIND_ENGINES[kind], engine_type, "%s has the engine of %s" % [vehicle, Kind.keys()[kind]])
    var train_type:int = (VehicleServer.vehicle_get_controller(occupied) as RailVehicleController).train_type
    assert_eq(kind in UNIT_KINDS, bool(train_type & UNIT_TRAIN_TYPES), "%s is driven as a unit or not" % vehicle)
    if not is_passing():
        return
    if not _power_supply(occupied).get_power24_available() and not await _key_taken(&"battery_toggle", func() -> bool:
            # the battery may be the powered car's, of a unit
            return _power_supply(occupied).get_battery_enabled() or _power_supply(powered).get_battery_enabled() \
                    or _power_supply(occupied).get_power24_available()):
        return
    if not await _until("battery: low voltage", func() -> bool: return _power_supply(occupied).get_power24_available()):
        return
    if _master(occupied).get_cabin() == 0 and not await _key_taken(&"cab_activation_toggle", func() -> bool:
            return not _master(occupied).get_cabin() == 0):
        return
    if not await _until("cab activation", func() -> bool: return not _master(occupied).get_cabin() == 0):
        return
    if kind in [Kind.ELECTRIC_LOCOMOTIVE, Kind.ELECTRIC_MULTIPLE_UNIT]:
        var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
                carrier, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
        # a vehicle fed otherwise (EL16 from its battery) has no pantographs to raise
        if power_source and power_source.source_type == RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR:
            if power_source.get_collector_pantograph_tank_pressure() < power_source.get_collector_min_pantograph_tank_pressure():
                # the main reservoir cannot fill it: the three-way valve to the small compressor, unless it
                # switches on its own (maszyna_legacy_driver_pantographs.gd prepare())
                if not power_source.cntrl_pantograph_auto_valve and not power_source.get_collector_pantograph_compressor_valve():
                    await key_tap(&"pantograph_compressor_valve_toggle")
                await key_press(&"pantograph_compressor_activate")
                var filled:bool = await _until("pantograph compressor: tank pressure", func() -> bool:
                        return power_source.get_collector_pantograph_tank_pressure() \
                                >= power_source.get_collector_min_pantograph_tank_pressure())
                await key_release(&"pantograph_compressor_activate")
                if not filled:
                    return
            var raised:Callable = func() -> bool:
                return power_source.get_collector_pantograph_first_active() or power_source.get_collector_pantograph_second_active()
            match pantographs:
                Pantographs.SWITCHES, Pantographs.SELECTED:
                    # the machine room is the next cab back from the front one
                    if pantographs == Pantographs.SELECTED:
                        await _change_cab(&"cabin_next")
                    # held a moment: an impulse switch works the valve only while it is held
                    if not power_source.get_collector_pantograph_first_active():
                        await key_hold(&"pantograph_front_toggle", PANTOGRAPH_SWITCH_HOLD)
                    if not power_source.get_collector_pantograph_second_active():
                        await key_hold(&"pantograph_rear_toggle", PANTOGRAPH_SWITCH_HOLD)
                    if pantographs == Pantographs.SELECTED:
                        await _change_cab(&"cabin_previous")
                        await key_hold(&"pantograph_toggle_selected", PANTOGRAPH_SWITCH_HOLD)
                Pantographs.SELECTOR:
                    for _preset:int in MAX_PANTOGRAPH_PRESETS:
                        if raised.call():
                            break
                        await key_tap(&"pantograph_select_next")
                        # the valves lever up and let go, as the hand does it (CabinLogic.increase())
                        var logic:CabinLogic = CabinSystem.vehicle_get_cab_logic(occupied)
                        logic.increase(VALVES_LEVER)
                        await step(ticks(PANTOGRAPH_SWITCH_HOLD))
                        logic.release(VALVES_LEVER)
                        await step(ticks(PANTOGRAPH_SWITCH_HOLD))
            if not await _until("pantographs: line voltage", func() -> bool: return power_source.get_collector_voltage() > 0.0):
                return
        if not await _direction_forward():
            return
        if not _engine(powered).get_main_switch_closable():
            await key_tap(&"fuse_reset")
            await key_tap(&"converter_fuse_reset")
        if not await _until("relays reset: line breaker closable", func() -> bool: return _engine(powered).get_main_switch_closable()):
            return
        await key_hold(&"main_switch_toggle", (_engine(powered) as RailVehicleElectricEngine).get_line_breaker_initial_delay()
                + MAIN_SWITCH_HOLD_MARGIN)
        if not await _until("line breaker", func() -> bool: return _engine(powered).get_main_switch_enabled()):
            return
    else:
        var diesel:RailVehicleDieselEngine = _engine(powered) as RailVehicleDieselEngine
        # only a pump the driver starts has its switch worked - an automatic one starts with the engine
        # (the FIZ's OilPump/FuelPump start), one the vehicle has not (disabled) not at all
        var oil_ready:Callable = func() -> bool:
            return not diesel.oil_pump_start_mode in MANUAL_STARTS or diesel.get_oil_pump_enabled() \
                    or diesel.get_oil_pump_active()
        var fuel_ready:Callable = func() -> bool:
            return not diesel.fuel_pump_start_mode in MANUAL_STARTS or diesel.get_fuel_pump_enabled() \
                    or diesel.get_fuel_pump_active()
        if not oil_ready.call():
            await key_tap(&"oil_pump_toggle")
        if not fuel_ready.call():
            await key_tap(&"fuel_pump_toggle")
        if not await _until("oil and fuel pumps", func() -> bool: return oil_ready.call() and fuel_ready.call()):
            return
        if not await _direction_forward():
            return
        # an SN61 starts with its master controller at the first position, then goes on to idle
        # (the manual, sterowanie.html)
        var sn61:bool = _sn61()
        if sn61:
            await key_tap(&"main_controller_increase")
        # the starter held until the engine fires
        await key_press(&"main_switch_toggle")
        var started:bool = await _until("engine started", func() -> bool:
                return diesel.get_main_switch_enabled() and diesel.get_rpm() > 0.0)
        await key_release(&"main_switch_toggle")
        if not started:
            return
        # idle is the first position with a clutch engaged (Mn > 0), not position 0: that one gives
        # no fuel (R = 0) and the engine dies (driverhints.cpp:489, Driver.cpp:5778)
        if sn61:
            while diesel.get_clutch_desired() == RailVehicleThrottlePositionItem.CLUTCH_BEHAVIOR_NONE:
                await key_tap(&"main_controller_increase")
    # once the line breaker or the engine is on: the converter, then the compressor - for an
    # electric and a diesel alike (Driver.cpp:2799-2806)
    # a vehicle without a converter (EL16: ConverterStart=Disabled) runs on its battery alone
    if not _power_supply(powered).cntrl_converter_start_mode == RailVehicleController.START_MODE_DISABLED:
        if not _power_supply(powered).get_converter_enabled():
            await key_tap(&"converter_toggle")
        if not await _until("converter: 110 V", func() -> bool:
                return _power_supply(powered).get_converter_enabled() and _power_supply(occupied).get_power110_available()):
            return
    var compressing:RailVehicleBrake = _brake(powered)
    # a vehicle without a compressor (a draisine's hand brake) has no air to wait for
    if compressing.compressor_cab_a_max_pressure > 0.0:
        if not compressing.get_compressor_enabled():
            await key_tap(&"compressor_toggle")
        if not await _until("compressor: main reservoir", func() -> bool:
                return compressing.get_compressor_pressure() > READY_AIR_PRESSURE, AIR_TIMEOUT):
            return
    var security:RailVehicleSecuritySystem = _security(occupied)
    if security and not await _until("security system not braking", func() -> bool: return not security.get_braking()):
        return
    var brake:RailVehicleBrake = _brake(occupied)
    while brake.get_manual_position() > 0:
        await key_tap(&"manual_brake_decrease")
    var spring_brake:RailVehicleSpringBrake = RailVehicleServer.vehicle_component_get(
            occupied, RailVehicleComponentType.COMPONENT_SPRING_BRAKE) as RailVehicleSpringBrake
    if spring_brake and spring_brake.get_active():
        await key_tap(&"spring_brake_toggle")
    if not await _until("manual and spring brakes released", func() -> bool:
            return brake.get_manual_position() == 0 and not (spring_brake and spring_brake.get_active())):
        return
    # a vehicle its scenery driver held may stand on the independent brake (Driver.cpp:8166-8180)
    if brake.get_local_position_normalized() > 0.0:
        await key_press(&"local_brake_decrease")
        var released:bool = await _until("independent brake released", func() -> bool:
                return brake.get_local_position_normalized() == 0.0)
        await key_release(&"local_brake_decrease")
        if not released:
            return
    # an individual brake (a draisine) has no brake pipe: its hand brake was the brake
    if not brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL:
        # the pipe's running pressure is the vehicle's (HiPP)
        var released_pipe:float = minf(RELEASED_PIPE_PRESSURE, brake.pipe_pressure_max * RELEASED_PIPE_SHARE)
        await key_tap(&"brake_level_drive")
        # an FV4a locks the brake pipe below 2.75 bar until the distributor releases (lock_old,
        # Mover.cpp:4033-4041): the driver holds the releaser (odluźniacz) while the pipe fills
        if brake.get_pipe_pressure() < released_pipe:
            # a newer vehicle locks the pipe and unlocks it by a universal brake button
            # (ub_UnlockPipe, Mover.cpp:3683, held by the hand - it has no key) or the handle below
            # HandleUnlock - its charging position, Num . (lock_new, Mover.cpp:4031)
            var logic:CabinLogic = CabinSystem.vehicle_get_cab_logic(occupied)
            var unlock_button:StringName = &""
            for button:int in UNIVERSAL_BRAKE_BUTTONS:
                if int(brake.get("universal_brake_button_%d" % button)) & UNLOCK_PIPE:
                    unlock_button = StringName("universalbrake%d_bt" % button)
                    break
            if brake.get_main_pipe_locked():
                if unlock_button:
                    logic.press(unlock_button)
                else:
                    await key_press(&"brake_level_charging")
            await key_press(&"brake_release")
            # the lock lets go only above LockPipeOff, higher than where it closes (LockPipeOn,
            # Mover.cpp:4030) - let go below it, the pipe locks again
            var filled:bool = await _until("brake pipe filled, the releaser held", func() -> bool:
                    return brake.get_pipe_pressure() > released_pipe and not brake.get_main_pipe_locked(), AIR_TIMEOUT)
            await key_release(&"brake_release")
            if unlock_button:
                logic.release(unlock_button)
            else:
                await key_release(&"brake_level_charging")
            await key_tap(&"brake_level_drive")
            if not filled:
                return
        if not await _until("train brake released", func() -> bool:
                return brake.get_pipe_pressure() > released_pipe and brake.get_air_pressure() < RELEASED_CYLINDER_PRESSURE,
                AIR_TIMEOUT):
            return
    # a gearbox takes its first gear before the throttle (Num /, the manual) - an SN61's gears are
    # its master controller's positions
    if kind == Kind.DIESEL_MECHANICAL and not _sn61():
        await key_tap(&"second_controller_increase")
    # a notch at a time, the next one once the controller has taken it: the line contactors close
    # only with the controller held at its first power position for InitialCtrlDelay
    # (Mover.cpp:6394-6401)
    var master:RailVehicleMasterController = _master(powered)
    var series_motor:bool = _engine(powered).get_type() == RailVehicleEngine.ELECTRIC_SERIES_MOTOR
    var position_max:int = int(VehicleServer.vehicle_dump_config(powered).get("master_controller_position_max", 0))
    for _step:int in MAX_CONTROLLER_STEPS:
        if VehicleServer.vehicle_get_speed(occupied) > MOVED_OFF_SPEED:
            break
        var position:int = master.get_main_position()
        if not await _key_taken(&"main_controller_increase", func() -> bool:
                return master.get_main_position() > position or position >= position_max):
            return
        # a series motor waits for its line contactors on the first power position, as long as its
        # InitialCtrlDelay; every other notch, and every notch of an induction motor or a diesel, is
        # given its time
        if series_motor and master.get_main_actual_position() == 0:
            if not await _until("line contactors on the first power position", func() -> bool:
                    return master.get_main_actual_position() > 0, master.initial_delay + LINE_CONTACTORS_MARGIN):
                return
        else:
            await step(ticks(CONTROLLER_NOTCH_SECONDS))
    await _until("moved off", func() -> bool: return VehicleServer.vehicle_get_speed(occupied) > MOVED_OFF_SPEED)




## Loads the scenery fixture and puts the player in `vehicle`'s cab, as the game does - nothing of
## the vehicle touched; false when it never came
func enter_vehicle(scenery:String, vehicle:String) -> bool:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    _player = load("res://addons/libmaszyna/player/player.tscn").instantiate()
    _player.start_vehicle_id = vehicle
    add_child(_player)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = scenery
    _scenery.scenery_loaded.connect(_player._on_scenery_loaded)
    add_child(_scenery)
    get_tree().process_frame.connect(_acknowledge_security)
    # the scenery-loaded signal lets the player take its vehicle, as the game does.
    # its trainsets coupled by then (RailVehicleServer.trainset_place()) - a cab activated in a car not
    # coupled yet never reaches the unit's motor car (SendCtrlToNext, Mover.cpp:2663)
    if not await wait_loaded(_scenery.scenery_loaded, scenery):
        return false
    occupied = VehicleServer.vehicle_get_rid_by_name(vehicle)
    if not occupied.is_valid():
        fail_test("%s is not in %s" % [vehicle, scenery])
        return false
    if not await _until("the player in the cab", func() -> bool: return PlayerServer.player_get_vehicle() == occupied):
        return false
    powered = RailVehicleServer.vehicle_find_powered(occupied)
    carrier = RailVehicleServer.vehicle_find_pantograph_carrier(occupied)
    if not carrier.is_valid():
        carrier = powered
    return true


## The player goes to the next or the previous cab of the vehicle (the machine room lies between)
func _change_cab(action:StringName) -> void:
    var left:RID = RailVehicleServer.vehicle_get_driver_cabin(occupied)
    await key_tap(action)
    await _until("%s taken" % action, func() -> bool:
            return not RailVehicleServer.vehicle_get_driver_cabin(occupied) == left)


## Forward from the cab the player sits in: the reverser counts from its cab, the vehicle's own
## direction is DirActive * CabActive (Mover.cpp:669) - a rear cab drives forward at 1 as well
func _direction_forward() -> bool:
    if VehicleServer.vehicle_get_controller(occupied).get_direction() == VehicleController.DIRECTION_NEUTRAL:
        await key_tap(&"direction_increase")
    return await _until("direction forward", func() -> bool:
            return VehicleServer.vehicle_get_controller(occupied).get_direction() == VehicleController.DIRECTION_FORWARD)


## Waits (simulated time) for `done`; a step that does not come about fails the test and says what
## the cars show
func _until(step_name:String, done:Callable, timeout:float = STEP_TIMEOUT) -> bool:
    for _tick:int in ticks(timeout):
        if done.call():
            return true
        await step(1)
    if done.call():
        return true
    fail_test("start-up stopped at %s after %.1f simulated s: %s" % [step_name, timeout, _cars_text()])
    return false


## What the cars show - the cab's own gauges, the driving aid's and the brake's
func _cars_text() -> String:
    var cars:Array[String] = []
    var seen:Array[RID] = []
    for car:RID in [occupied, powered, carrier]:
        if car in seen:
            continue
        seen.append(car)
        var state:Dictionary = VehicleServer.vehicle_dump_state(car)
        var line:String = VehicleServer.vehicle_get_controller(car).vehicle_id + ":"
        for key:String in ["battery_enabled", "power24_available", "cabin", "direction",
                "current_collector/pantograph_first_active", "current_collector/voltage",
                "main_switch_enabled", "main_switch_closable", "converter_enabled", "power110_available",
                "current_collector/pantograph_tank_pressure", "current_collector/min_pantograph_tank_pressure",
                "current_collector/pantograph_compressor_enabled", "current_collector/pantograph_compressor_valve",
                "current_collector/valve_enabled", "current_collector/valve_active",
                "current_collector/pantograph_first_valve_enabled", "current_collector/pantograph_second_valve_enabled",
                "oil_pump_enabled", "oil_pump_active", "oil_pump_disabled", "fuel_pump_enabled", "fuel_pump_active",
                "fuel_pump_disabled", "engine_rpm_count",
                "compressor_pressure", "compressor_enabled", "compressor_allowed", "feed_pipe_pressure", "main_pipe_locked", "brake_is_cut_off", "pipe_pressure",
                "brake_air_pressure", "brake_controller_position",
                "brake_local_position_normalized", "spring_brake/active", "controller_main_position",
                "controller_main_actual_position", "engine_current", "tractive_force", "speed",
                "blinking", "vigilance_blinking", "cabsignal_blinking", "braking",
                "brake_emergency_valve_flow", "brake_main_valve_flow", "brake_handle_release_flow",
                "brake_handle_emergency_flow", "brake_handle_braking_flow", "alarm_chain_pulled",
                "brake_releaser_active", "brake_operation_mode", "brake_control_pressure"]:
            if state.has(key):
                line += " %s=%s" % [key, state[key]]
        cars.append(line)
    return "; ".join(cars)


## The key's own control moves on the next tick - what it starts may take its time, the key itself
## not: a key that did not take fails the step here, not after a wait for what it never started
func _key_taken(action:StringName, taken:Callable) -> bool:
    await key_tap(action)
    await step(1)
    if taken.call():
        return true
    fail_test("start-up stopped at %s: the key did not take: %s" % [action, _cars_text()])
    return false


## The driver's reflex, every frame: the security system blinking is acknowledged at once - left
## alone it brakes in emergency and empties the brake pipe (the battery arms it, Mover.cpp:131);
## the one of the vehicle the player is in, as the key reaches only that one
func _acknowledge_security() -> void:
    var vehicle:RID = PlayerServer.player_get_vehicle()
    var security:RailVehicleSecuritySystem = _security(vehicle) if vehicle.is_valid() else null
    if not security:
        return
    if security.get_cabsignal_blinking():
        _key_now(&"security_cabsignal_acknowledge")
    if security.get_blinking():
        _key_now(&"security_acknowledge")


func key_press(action:StringName) -> void:
    await _key(action, true)


func key_release(action:StringName) -> void:
    await _key(action, false)


## Pressed for one tick and let go: the cab's logic takes a key on the simulation's step
## (CabinSystem), so a press that ends before the next tick is never seen
func key_tap(action:StringName) -> void:
    await key_press(action)
    await step(1)
    await key_release(action)


func key_hold(action:StringName, seconds:float) -> void:
    await key_press(action)
    await step(ticks(seconds))
    await key_release(action)


func _key(action:StringName, pressed:bool) -> void:
    var event:InputEventAction = InputEventAction.new()
    event.action = action
    event.pressed = pressed
    Input.parse_input_event(event)
    Input.flush_buffered_events()
    await wait_idle_frames(1)


## A key pressed and let go within the frame - the reflex, which cannot wait for the next one
func _key_now(action:StringName) -> void:
    for pressed:bool in [true, false]:
        var event:InputEventAction = InputEventAction.new()
        event.action = action
        event.pressed = pressed
        Input.parse_input_event(event)
    Input.flush_buffered_events()




func _engine(car:RID) -> RailVehicleEngine:
    return VehicleServer.vehicle_component_get(car, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine


func _power_supply(car:RID) -> RailVehiclePowerSupply:
    return RailVehicleServer.vehicle_component_get(car, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) \
            as RailVehiclePowerSupply


func _master(car:RID) -> RailVehicleMasterController:
    return RailVehicleServer.vehicle_component_get(car, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) \
            as RailVehicleMasterController


func _sn61() -> bool:
    return bool((VehicleServer.vehicle_get_controller(powered) as RailVehicleController).train_type
            & RailVehicleController.TRAIN_TYPE_SN61)


func _security(car:RID) -> RailVehicleSecuritySystem:
    return RailVehicleServer.vehicle_component_get(car, RailVehicleComponentType.COMPONENT_SECURITY) \
            as RailVehicleSecuritySystem


func _brake(car:RID) -> RailVehicleBrake:
    return RailVehicleServer.vehicle_component_get(car, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
