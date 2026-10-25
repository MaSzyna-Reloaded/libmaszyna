@tool
extends RefCounted
class_name MaszynaDriverPantographs

## The original driver's pantographs (the pantograph part of TController::PrepareEngine(),
## control_pantographs(), Driver.cpp:2782-2811, 6219-6345): the air for them from the small
## compressor until the main reservoir takes over, both raised to start, and on the move the one at
## the rear the way it drives - or the vehicle's suggested setup - the front one lowered once the
## rear one carries the current. Every step is cued (MaszynaDriverHints): a driver the
## computer is takes it, a player is hinted.
##
## Not ported: running without current or with the pantographs down where the track says so
## (fOverhead2, iOverheadDown), and the front one raised too when standing long at a stop
## (AIHintPantUpIfIdle, IdleTime) - see TODO.md, "Drivers".

## The pantographs rise from this tank pressure [bar], an EMU's from EMU_RAISING_PRESSURE, with
## RAISING_MARGIN to spare (Driver.cpp:2786-2790)
const RAISING_PRESSURE:float = 3.5
const EMU_RAISING_PRESSURE:float = 2.5
const RAISING_MARGIN:float = 0.1
## The main reservoir feeds the pantographs once it holds this [bar] (Driver.cpp:6226)
const MAIN_FEEDING_PRESSURE:float = 4.3
## Moving faster than this [km/h] the front pantograph comes down and the suggested setup is used,
## and the second pantograph's hint of the start no longer matters (Driver.cpp:2812-2813, 6280, 6311)
const SETUP_SPEED:float = 5.0


## PrepareEngine()'s pantographs (Driver.cpp:2782-2811): the small compressor while the tank is
## short of air - its three-way valve turned to it first, unless it turns by itself - and off once
## the tank has enough or the main reservoir feeds it; both raised - whichever cars of the unit
## have them - the one at the back of the way it drives hinted only until it moves off
static func prepare(situation:MaszynaDriverTraction.Situation, emu:bool) -> void:
    var unit:RID = situation.trainset.pantograph_unit
    if not unit.is_valid():
        return
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            unit, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            unit, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    var tank:float = power_source.get_collector_pantograph_tank_pressure()
    if tank < (EMU_RAISING_PRESSURE if emu else RAISING_PRESSURE) + RAISING_MARGIN:
        if not power_source.cntrl_pantograph_auto_valve:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY)
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.PANTOGRAPH_COMPRESSOR_ON)
        if power_source.get_collector_pantograph_compressor_enabled():
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.WAIT_PANTOGRAPH_PRESSURE_TOO_LOW)
    elif power_source.get_collector_pantograph_compressor_valve() \
            or tank <= (brake.get_compressor_pressure() if brake else 0.0):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.PANTOGRAPH_COMPRESSOR_OFF)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.PANTOGRAPHS_VALVE_ON)
    var forward:bool = situation.state.direction >= 0
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_ON,
            SETUP_SPEED if forward else 0.0)
    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_ON,
            0.0 if forward else SETUP_SPEED)


## control_pantographs() (Driver.cpp:6219-6345), on every update of a driver with its engine ready:
## the main reservoir to the pantographs once it holds enough; on the move the rear one up, the
## front one down when both carry the current, or the vehicle's suggested setup
static func control(situation:MaszynaDriverTraction.Situation, emu:bool, waiting:bool) -> void:
    var vehicle:RID = situation.vehicle
    var unit:RID = situation.trainset.pantograph_unit
    if not unit.is_valid():
        return
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            unit, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    if not power_source.cntrl_pantograph_auto_valve \
            and (brake.get_compressor_pressure() if brake else 0.0) > MAIN_FEEDING_PRESSURE:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN)
    var speed:float = VehicleServer.vehicle_get_speed(vehicle)
    if speed <= MaszynaDriverTrainset.NO_MOVEMENT_SPEED or waiting:
        return
    var hints:RailVehicleAIHints = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_AI_HINTS) as RailVehicleAIHints
    var setup:RailVehicleAIHints.PantographState = hints.pantograph_state if hints else RailVehicleAIHints.PANTOGRAPH_STATE_AUTOMATIC
    if not setup == RailVehicleAIHints.PANTOGRAPH_STATE_AUTOMATIC:
        if speed > SETUP_SPEED:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_ON
                    if setup & RailVehicleAIHints.PANTOGRAPH_STATE_FRONT else MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_OFF)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_ON
                    if setup & RailVehicleAIHints.PANTOGRAPH_STATE_REAR else MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_OFF)
        return
    # the regular layout: a lone vehicle, an EMU, an ET41 (Driver.cpp:6243-6246)
    var train_type:MaszynaMoverRailVehicleController.TrainType = (
            VehicleServer.vehicle_get_controller(unit) as RailVehicleController).train_type
    var regular:bool = RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL).size() == 1 \
            or emu or train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_ET41
    var voltage:float = power_source.get_collector_voltage()
    var front_voltage:float = power_source.get_collector_pantograph_first_voltage()
    var rear_voltage:float = power_source.get_collector_pantograph_second_voltage()
    var on_rear:bool = situation.state.direction >= 0 and regular
    # more than one by CollectorsNo, which a layout of B alone makes 2 (Mover.cpp:11636-11637)
    var both:bool = power_source.current_collector_number_of_collectors > 1
    # the one at the rear up, unless another one works and it is the only one
    var raised_voltage:float = rear_voltage if on_rear else front_voltage
    if raised_voltage == 0.0 and (voltage == 0.0 or both):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_ON
                if on_rear else MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_ON)
    # having gathered speed, the other one down once the first carries the current
    if speed > SETUP_SPEED and both and not front_voltage == 0.0 and not rear_voltage == 0.0:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_OFF
                if on_rear else MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_OFF)
