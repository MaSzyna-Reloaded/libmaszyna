extends RefCounted
class_name BrakeSoundModel

## Brake sounds of one vehicle bank: the original per-frame play/stop/gain/pitch logic of the cab
## (TTrain::update_sounds(), Train.cpp:8474-8641) and of the vehicle (TDynamicObject's brake
## section, DynObj.cpp:4545-4760), with the sound-only state those keep - the filters, the last
## pressures, the hand-made fade-outs. It reads the vehicle's components (typed getters, never the
## state or config dump, this runs every frame) and only decides what each event should do;
## TrainSoundSystem plays it. Combined (chunked) sounds get the chunk selector as `point`, single samples the
## playback `pitch`, as RunningSoundModel does.

## START plays a stopped loop from its opening bookend; EXCLUSIVE_ONE_SHOT is not started again
## while it still plays (sound_flags::exclusive), a ONE_SHOT always is.
enum Action {STOP, START, LOOP, ONE_SHOT, EXCLUSIVE_ONE_SHOT}

## Labels played once per event rather than looped (DynObj.cpp:4547-4615, 4718-4723, 4749-4759)
const ONE_SHOT_LABELS:Array[String] = [
    "brakeacc", "brakecylinderinc", "brakecylinderdec", "epbrakeinc", "epbrakedec",
    "springbrake", "springbrakeoff",
]
## OpenAL's default AL_MAX_GAIN: whatever sound_source::gain() is set to (clamped 0..2,
## sound.cpp:853), a source is never heard above its recorded level
const MAX_GAIN:float = 1.0
## The pressures are cached from the first update on (m_lastlocalbrakepressure { -1.f },
## Train.h; m_lastbrakepressure, m_lastepbrakepressure { -1.f }, DynObj.h:535-539)
const UNSET_PRESSURE:float = -1.0
## A fading sound stops below this gain (Train.cpp:8500, DynObj.cpp:4657, 4745)
const STOP_GAIN:float = 0.05
## MoverRailVehicleBrake._apply_configuration(): an auxiliary pressure below this is not given, and
## MaxBrakePress[0] is the cylinders' (Mover.cpp, LoadFIZ_Brake)
const AUX_PRESSURE_MIN:float = 0.01

## Train.cpp:8480-8517 - local brake hiss: 10x the filtered rate of the local cylinder's pressure
## change, played past its threshold and faded out at 0.1/s
const LOCAL_RATE_SCALE:float = 10.0
const LOCAL_RATE_FILTER:float = 0.1
const LOCAL_HISS_THRESHOLD:float = 0.05
const LOCAL_HISS_GAIN:float = 0.05
## the release hiss only while the local brake holds the cylinders (LocBrakePress > BrakePress - 0.05)
const LOCAL_RELEASE_MARGIN:float = 0.05
const LOCAL_FADE_PER_SECOND:float = 0.1

## Train.cpp:8519-8586 - FV4a/FVel6 handle hiss: filtered handle flows, half the volume
const HANDLE_VOLUME_SCALE:float = 0.5
const HANDLE_BRAKING_FILTER:float = 0.05
const HANDLE_RELEASE_FILTER:float = 0.25
const HANDLE_BRAKING_GAIN:float = 0.25
const HANDLE_HISS_THRESHOLD:float = 0.05
## Train.cpp:8586-8618 - any other handle: the main valve flow averaged over 4+1 updates
const VALVE_FLOW_HISTORY:float = 4.0
const VALVE_BRAKING_GAIN:float = 2.0
const VALVE_BRAKING_THRESHOLD:float = 0.05
const VALVE_RELEASE_THRESHOLD:float = 0.01

## DynObj.cpp:4697-4716 - the shoes' sound and the squeal's force ratio, above this force and speed
const SHOE_MIN_FORCE:float = 10.0
const SHOE_MIN_SPEED:float = 0.05
const SHOE_LOW_SPEED_FACTOR:float = 0.4
## DynObj.cpp:4725-4746 - the squeal plays above this speed and starts above this gain, and below
## the speed it fades out at 2.5x its gain per second
const SQUEAL_MIN_SPEED:float = 2.5
const SQUEAL_START_GAIN:float = 0.075
const SQUEAL_FADE_RATE:float = 2.5
## DynObj.cpp:6205 - each vehicle's squeal is up to 5% off its declared amplitude offset
const SQUEAL_OFFSET_MIN:float = 0.95
const SQUEAL_OFFSET_MAX:float = 1.05
## the combined squeal picks its chunk from the speed, Vel * 0.01 (DynObj.cpp:4733)
const SPEED_CHUNK_SCALE:float = 0.01

## DynObj.cpp:4547-4575 - a cylinder click per 1/15 of its full pressure
const CYLINDER_STEPS:float = 15.0
## DynObj.cpp:4578-4617 - an EP click per 1/50, increase and decrease at most every 0.05/0.3 s
const EP_STEPS:float = 50.0
const EP_INCREASE_INTERVAL:float = 0.05
const EP_DECREASE_INTERVAL:float = 0.3
const EP_TIMER_DISABLED:float = -1.0
## the combined clicks pick their chunk from the step, step * 0.01
const STEP_CHUNK_SCALE:float = 0.01

## DynObj.cpp:4620-4637 - emergency valve hiss, started and stopped with a hysteresis
const EMERGENCY_START_FLOW:float = 0.025
const EMERGENCY_STOP_FLOW:float = 0.015
const EMERGENCY_FLOW_FILTER:float = 0.1
const EMERGENCY_PIPE_FACTOR:float = 0.1
const EMERGENCY_PIPE_MAX:float = 0.5

## DynObj.cpp:4639-4660 - cylinder release hiss: the filtered rate of pressure drop, faded at 0.5/s
const RELEASE_RATE_FILTER:float = 0.05
const RELEASE_HISS_THRESHOLD:float = 0.05
const RELEASE_MIN_PRESSURE_RATIO:float = 0.05
const RELEASE_BASE_VOLUME:float = 0.25
const RELEASE_PRESSURE_VOLUME:float = 0.75
const RELEASE_FADE_PER_SECOND:float = 0.5

## DynObj.cpp:4662-4673 - the releaser, louder with the cylinder pressure ("arbitrary multiplier")
const RELEASER_PRESSURE_GAIN:float = 1.25

## DynObj.cpp:4676-4689 - slipping wheels squeal above this force and speed
const SLIP_MIN_FORCE:float = 100.0
const SLIP_MIN_SPEED:float = 1.0


## What the formulas read of the vehicle, refreshed by update() from the attached components
class Readings extends RefCounted:
    var speed:float = 0.0
    var slipping:bool = false
    var spring_brake_active:bool = false
    ## BrakePress, LocBrakePress, PipePress
    var cylinder_pressure:float = 0.0
    var local_pressure:float = 0.0
    var pipe_pressure:float = 0.0
    var unit_force:float = 0.0
    var force_ratio:float = 0.0
    var emergency_valve_flow:float = 0.0
    var main_valve_flow:float = 0.0
    var releaser_active:bool = false
    ## LocHandle->GetCP(), LocalBrakePosAEIM, Hamulec->GetEDBCP()
    var control_pressure:float = 0.0
    var local_aeim_position:float = 0.0
    var edb_cylinder_pressure:float = 0.0
    ## Handle->GetSound(s_fv4a_b/u/e/x/t)
    var handle_braking_flow:float = 0.0
    var handle_release_flow:float = 0.0
    var handle_emergency_flow:float = 0.0
    var handle_control_chamber_flow:float = 0.0
    var handle_timing_reservoir_flow:float = 0.0
    ## configuration: Vmax, MaxBrakePress[3], MaxBrakePress[0], an FV4a/FVel6 handle, an EP brake
    var max_speed:float = 0.0
    var max_cylinder_pressure:float = 0.0
    var max_control_pressure:float = 0.0
    var handle_sounds:bool = false
    var electro_pneumatic:bool = false


class BrakeSound extends RefCounted:
    var event_name:StringName = &""
    var source:MmdSoundSourceDefinition
    ## a combined one-shot has one event per chunk, in RunningSoundModel.sorted_chunks() order
    var chunk_events:Array[StringName] = []
    var playing:bool = false
    ## sound_source::gain() and pitch() - set last, a fade-out lowers the gain from there
    var gain:float = 0.0
    var pitch:float = 1.0


## every sound of the bank, for TrainSoundSystem's placement of them
var sounds:Array[BrakeSound] = []
var readings := Readings.new()

var _local_release:BrakeSound
var _local_engage:BrakeSound
var _valve_braking:BrakeSound
var _valve_release:BrakeSound
var _valve_emergency:BrakeSound
var _valve_control_chamber:BrakeSound
var _valve_timing_reservoir:BrakeSound
var _shoe:BrakeSound
var _squeal:BrakeSound
var _release_hiss:BrakeSound
var _emergency:BrakeSound
var _releaser:BrakeSound
var _slip:BrakeSound
var _accelerator:BrakeSound
var _cylinder_increase:BrakeSound
var _cylinder_decrease:BrakeSound
var _ep_increase:BrakeSound
var _ep_decrease:BrakeSound
var _spring_activate:BrakeSound
var _spring_release:BrakeSound

var _vehicle_rid:RID = RID()
var _controller:VehicleController
var _brake:RailVehicleBrake
var _wheels:RailVehicleWheels
var _spring_brake:RailVehicleSpringBrake
var _squeal_amplitude_offset:float = 0.0

## TTrain's m_lastlocalbrakepressure, m_localbrakepressurechange, fPPress, fNPress
var _last_local_pressure:float = UNSET_PRESSURE
var _local_pressure_change:float = 0.0
var _braking_flow:float = 0.0
var _release_flow:float = 0.0
## TDynamicObject's m_lastbrakepressure, m_brakepressurechange, m_lastepbrakepressure and its
## timers, m_emergencybrakeflow, m_springbrakesounds.state
var _last_cylinder_pressure:float = UNSET_PRESSURE
var _cylinder_pressure_change:float = 0.0
var _last_ep_pressure:float = UNSET_PRESSURE
var _ep_increase_timer:float = 0.0
var _ep_decrease_timer:float = 0.0
var _emergency_flow:float = 0.0
var _spring_active:bool = false
var _last_accelerator_count:int = 0


## A sound of the bank, its event already built
func add_sound(source:MmdSoundSourceDefinition, event_name:StringName, chunk_events:Array[StringName]) -> void:
    var sound := BrakeSound.new()
    sound.event_name = event_name
    sound.source = source
    sound.chunk_events = chunk_events
    sounds.append(sound)
    match source.label:
        "localbrakesound": _local_release = sound
        "localbrakesound2": _local_engage = sound
        "airsound": _valve_braking = sound
        "airsound2": _valve_release = sound
        "airsound3": _valve_emergency = sound
        "airsound4": _valve_control_chamber = sound
        "airsound5": _valve_timing_reservoir = sound
        "brakesound": _shoe = sound
        "brake":
            _squeal = sound
            _squeal_amplitude_offset = source.amplitude_offset * randf_range(SQUEAL_OFFSET_MIN, SQUEAL_OFFSET_MAX)
        "unbrake": _release_hiss = sound
        "emergencybrake": _emergency = sound
        "releaser": _releaser = sound
        "slipperysound": _slip = sound
        "brakeacc": _accelerator = sound
        "brakecylinderinc": _cylinder_increase = sound
        "brakecylinderdec": _cylinder_decrease = sound
        "epbrakeinc": _ep_increase = sound
        "epbrakedec": _ep_decrease = sound
        "springbrake": _spring_activate = sound
        "springbrakeoff": _spring_release = sound


## The vehicle whose components the formulas read, taken once - TrainSoundSystem calls it again
## when the vehicle gets another controller (its components are then other objects).
func attach_vehicle(vehicle_rid:RID) -> void:
    _vehicle_rid = vehicle_rid
    _controller = VehicleServer.vehicle_get_controller(vehicle_rid)
    _brake = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    _wheels = VehicleServer.vehicle_component_get(
            vehicle_rid, VehicleComponentType.COMPONENT_WHEELS) as RailVehicleWheels
    _spring_brake = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_SPRING_BRAKE) as RailVehicleSpringBrake


## The bank went out of earshot: its sounds are stopped, and the pressure rates start over when it
## is heard again instead of taking the whole time in between for one step
func silence() -> void:
    for sound:BrakeSound in sounds:
        sound.playing = false
    _last_local_pressure = UNSET_PRESSURE
    _last_cylinder_pressure = UNSET_PRESSURE
    _last_ep_pressure = UNSET_PRESSURE


## Returns event name -> {"action": Action, "parameters": Dictionary, "source": definition}, only
## for the events that play or have to stop. `accelerator_count` counts the brake's
## accelerator_activated events. Without an attached vehicle `readings` are taken as they are.
func update(accelerator_count:int, delta:float) -> Dictionary:
    if _brake:
        _read_vehicle()
    var results:Dictionary = {}
    var brake_pressure:float = readings.cylinder_pressure

    # Train.cpp:8480-8517 - local brake, release and engage
    var local_pressure:float = readings.local_pressure
    if not _last_local_pressure == UNSET_PRESSURE and delta > 0.0:
        _local_pressure_change = lerpf(
                _local_pressure_change, LOCAL_RATE_SCALE * (local_pressure - _last_local_pressure) / delta,
                LOCAL_RATE_FILTER)
    _last_local_pressure = local_pressure
    if _local_release:
        if (_local_pressure_change < -LOCAL_HISS_THRESHOLD
                and local_pressure > brake_pressure - LOCAL_RELEASE_MARGIN):
            _loop(_local_release, _local_release.source.amplitude_offset
                    + _local_release.source.amplitude_factor * -_local_pressure_change * LOCAL_HISS_GAIN,
                    _local_release.pitch, results)
        else:
            _fade(_local_release, LOCAL_FADE_PER_SECOND * delta, results)
    if _local_engage:
        if _local_pressure_change > LOCAL_HISS_THRESHOLD:
            _loop(_local_engage, _local_engage.source.amplitude_offset
                    + _local_engage.source.amplitude_factor * _local_pressure_change * LOCAL_HISS_GAIN,
                    _local_engage.pitch, results)
        else:
            _fade(_local_engage, LOCAL_FADE_PER_SECOND * delta, results)

    # Train.cpp:8519-8618 - the driver's brake valve
    if readings.handle_sounds:
        if _valve_braking:
            _braking_flow = lerpf(
                    _braking_flow, readings.handle_braking_flow, HANDLE_BRAKING_FILTER)
            var volume:float = (
                    _valve_braking.source.amplitude_factor * _braking_flow * HANDLE_BRAKING_GAIN
                    + _valve_braking.source.amplitude_offset if _braking_flow > 0.0 else 0.0)
            _handle_hiss(_valve_braking, volume, results)
        if _valve_release:
            _release_flow = lerpf(
                    _release_flow, readings.handle_release_flow, HANDLE_RELEASE_FILTER)
            var volume:float = (
                    _valve_release.source.amplitude_factor * _release_flow
                    + _valve_release.source.amplitude_offset if _release_flow > 0.0 else 0.0)
            _handle_hiss(_valve_release, volume, results)
        if _valve_emergency:
            _handle_hiss(_valve_emergency, readings.handle_emergency_flow
                    * _valve_emergency.source.amplitude_factor + _valve_emergency.source.amplitude_offset, results)
        if _valve_control_chamber:
            _handle_hiss(_valve_control_chamber, readings.handle_control_chamber_flow
                    * _valve_control_chamber.source.amplitude_factor
                    + _valve_control_chamber.source.amplitude_offset, results)
        if _valve_timing_reservoir:
            _handle_hiss(_valve_timing_reservoir, readings.handle_timing_reservoir_flow
                    * _valve_timing_reservoir.source.amplitude_factor
                    + _valve_timing_reservoir.source.amplitude_offset, results)
    else:
        var main_valve_flow:float = readings.main_valve_flow
        if _valve_braking:
            _braking_flow = (VALVE_FLOW_HISTORY * _braking_flow + maxf(0.0, main_valve_flow)) / (VALVE_FLOW_HISTORY + 1.0)
            var volume:float = (
                    VALVE_BRAKING_GAIN * _valve_braking.source.amplitude_factor * _braking_flow
                    + _valve_braking.source.amplitude_offset if _braking_flow > 0.0 else 0.0)
            if volume > VALVE_BRAKING_THRESHOLD:
                _loop(_valve_braking, volume, _valve_braking.pitch, results)
            else:
                _stop(_valve_braking, results)
        if _valve_release:
            _release_flow = (VALVE_FLOW_HISTORY * _release_flow + minf(0.0, main_valve_flow)) / (VALVE_FLOW_HISTORY + 1.0)
            var volume:float = (
                    -_valve_release.source.amplitude_factor * _release_flow
                    + _valve_release.source.amplitude_offset if _release_flow < 0.0 else 0.0)
            if volume > VALVE_RELEASE_THRESHOLD:
                _loop(_valve_release, volume, _valve_release.pitch, results)
            else:
                _stop(_valve_release, results)

    # DynObj.cpp:4547-4575 - brake cylinder piston
    var maximum_cylinder_pressure:float = maxf(1.0, readings.max_cylinder_pressure)
    var cylinder_ratio:float = maxf(0.0, brake_pressure) / maximum_cylinder_pressure
    if not _last_cylinder_pressure == UNSET_PRESSURE:
        var step:int = int(CYLINDER_STEPS * cylinder_ratio)
        var step_change:int = step - int(CYLINDER_STEPS * maxf(0.0, _last_cylinder_pressure) / maximum_cylinder_pressure)
        if step_change > 0 and _cylinder_increase:
            _one_shot(_cylinder_increase, step * STEP_CHUNK_SCALE, Action.ONE_SHOT, results)
        elif step_change < 0 and _cylinder_decrease:
            _one_shot(_cylinder_decrease, step * STEP_CHUNK_SCALE, Action.ONE_SHOT, results)

    # DynObj.cpp:4578-4617 - EP brake
    if readings.electro_pneumatic and (_ep_increase or _ep_decrease):
        var control_pressure:float = readings.control_pressure
        var local_position:float = readings.local_aeim_position
        var maximum_control_pressure:float = maxf(1.0, readings.max_control_pressure)
        var ep_ratio:float = minf(
                maxf(0.0, control_pressure) / maximum_control_pressure,
                maxf(local_position, readings.edb_cylinder_pressure / readings.max_cylinder_pressure)
                if _ep_decrease_timer > EP_TIMER_DISABLED else 1.0)
        if not _last_ep_pressure == UNSET_PRESSURE:
            _ep_increase_timer += delta
            _ep_decrease_timer += delta
            var step:int = int(EP_STEPS * ep_ratio)
            var step_change:int = step - int(EP_STEPS * maxf(0.0, _last_ep_pressure) / maximum_control_pressure)
            if step_change > 0 and _ep_increase_timer > EP_INCREASE_INTERVAL:
                if _ep_increase:
                    _one_shot(_ep_increase, step * STEP_CHUNK_SCALE, Action.ONE_SHOT, results)
                _ep_increase_timer = 0.0
            elif step_change < 0 and _ep_decrease_timer > EP_DECREASE_INTERVAL:
                if _ep_decrease:
                    _one_shot(_ep_decrease, -step_change * STEP_CHUNK_SCALE, Action.ONE_SHOT, results)
                _ep_decrease_timer = 0.0
        if _ep_increase_timer == 0.0 or _ep_decrease_timer == 0.0:
            _last_ep_pressure = minf(control_pressure, local_position * maximum_control_pressure)

    # DynObj.cpp:4620-4637 - emergency brake
    var emergency_valve_flow:float = readings.emergency_valve_flow
    if emergency_valve_flow > EMERGENCY_START_FLOW:
        _emergency_flow = (
                emergency_valve_flow if _emergency_flow == 0.0
                else lerpf(_emergency_flow, emergency_valve_flow, EMERGENCY_FLOW_FILTER))
        if _emergency:
            var flow_pressure:float = (
                    clampf(_emergency_flow, 0.0, 1.0)
                    + clampf(EMERGENCY_PIPE_FACTOR * readings.pipe_pressure, 0.0, EMERGENCY_PIPE_MAX))
            _loop(_emergency, _emergency.source.amplitude_offset
                    + clampf(flow_pressure, 0.0, 1.0) * _emergency.source.amplitude_factor,
                    _emergency.source.frequency_offset + _emergency.source.frequency_factor, results)
    elif emergency_valve_flow < EMERGENCY_STOP_FLOW:
        _emergency_flow = 0.0
        if _emergency:
            _stop(_emergency, results)

    # DynObj.cpp:4639-4660 - air release
    if not _last_cylinder_pressure == UNSET_PRESSURE and delta > 0.0:
        _cylinder_pressure_change = lerpf(
                _cylinder_pressure_change, (_last_cylinder_pressure - brake_pressure) / delta, RELEASE_RATE_FILTER)
    _last_cylinder_pressure = brake_pressure
    if _release_hiss:
        if _cylinder_pressure_change > RELEASE_HISS_THRESHOLD and cylinder_ratio > RELEASE_MIN_PRESSURE_RATIO:
            _loop(_release_hiss, _release_hiss.source.amplitude_factor * _cylinder_pressure_change
                    * (RELEASE_BASE_VOLUME + RELEASE_PRESSURE_VOLUME * cylinder_ratio), _release_hiss.pitch, results)
        else:
            _fade(_release_hiss, RELEASE_FADE_PER_SECOND * delta, results)

    # DynObj.cpp:4662-4673 - releaser
    if _releaser:
        if readings.releaser_active:
            _loop(_releaser, clampf(brake_pressure * RELEASER_PRESSURE_GAIN, 0.0, 1.0), _releaser.pitch, results)
        else:
            _stop(_releaser, results)

    # DynObj.cpp:4676-4689 - slipping wheels; the amplitude factor is divided by 1 + Vmax on load
    # (DynObj.cpp:6776)
    var speed:float = readings.speed
    var unit_force:float = readings.unit_force
    if _slip:
        if readings.slipping:
            if unit_force > SLIP_MIN_FORCE and speed > SLIP_MIN_SPEED:
                _loop(_slip, _slip.source.amplitude_offset
                        + _slip.source.amplitude_factor / (1.0 + readings.max_speed) * speed / readings.max_speed, _slip.pitch, results)
        else:
            _stop(_slip, results)

    # DynObj.cpp:4697-4716 (and its cab copy, Train.cpp:8621-8641) - brakes; the frequency factor
    # is divided by 1 + Vmax on load (DynObj.cpp:6192, Train.cpp:9091)
    var force_ratio:float = 0.0
    if unit_force > SHOE_MIN_FORCE and speed > SHOE_MIN_SPEED:
        force_ratio = readings.force_ratio
        if _shoe:
            _loop(_shoe, _shoe.source.amplitude_offset
                    + sqrt(force_ratio * lerpf(SHOE_LOW_SPEED_FACTOR, 1.0, speed / (1.0 + readings.max_speed)))
                    * _shoe.source.amplitude_factor,
                    _shoe.source.frequency_offset + speed * _shoe.source.frequency_factor / (1.0 + readings.max_speed),
                    results)
    elif _shoe:
        _stop(_shoe, results)

    # DynObj.cpp:4718-4723 - accelerator
    if _accelerator and not accelerator_count == _last_accelerator_count:
        _one_shot(_accelerator, _accelerator.pitch, Action.EXCLUSIVE_ONE_SHOT, results)
    _last_accelerator_count = accelerator_count

    # DynObj.cpp:4725-4746 - squeal of hard pressed brakes
    if _squeal:
        var volume:float = 0.0
        if speed > SQUEAL_MIN_SPEED:
            volume = _squeal_amplitude_offset + lerpf(-1.0, 1.0, force_ratio) * _squeal.source.amplitude_factor
            if volume > SQUEAL_START_GAIN:
                _loop(_squeal, volume, speed * SPEED_CHUNK_SCALE if _squeal.source.chunks
                        else _squeal.source.frequency_offset + _squeal.source.frequency_factor, results)
        else:
            volume = maxf(0.0, _squeal.gain - _squeal.gain * SQUEAL_FADE_RATE * delta)
            _set_gain(_squeal, volume, results)
        if volume < STOP_GAIN:
            _stop(_squeal, results)

    # DynObj.cpp:4749-4759 - spring brake
    var spring_active:bool = readings.spring_brake_active
    if not _spring_active == spring_active:
        _spring_active = spring_active
        var started:BrakeSound = _spring_activate if spring_active else _spring_release
        var stopped:BrakeSound = _spring_release if spring_active else _spring_activate
        if stopped:
            _stop(stopped, results)
        if started:
            _one_shot(started, started.pitch, Action.EXCLUSIVE_ONE_SHOT, results)
    return results


func _read_vehicle() -> void:
    readings.speed = VehicleServer.vehicle_get_speed(_vehicle_rid)
    readings.slipping = _wheels.get_slipping() if _wheels else false
    readings.spring_brake_active = _spring_brake.get_active() if _spring_brake else false
    readings.cylinder_pressure = _brake.get_air_pressure()
    readings.local_pressure = _brake.get_loco_pressure()
    readings.pipe_pressure = _brake.get_pipe_pressure()
    readings.unit_force = _brake.get_unit_force()
    readings.force_ratio = _brake.get_force_ratio()
    readings.emergency_valve_flow = _brake.get_emergency_valve_flow()
    readings.main_valve_flow = _brake.get_main_valve_flow()
    readings.releaser_active = _brake.get_releaser_active()
    readings.control_pressure = _brake.get_control_pressure()
    readings.local_aeim_position = _brake.get_local_aeim_position()
    readings.edb_cylinder_pressure = _brake.get_edb_cylinder_pressure()
    readings.handle_braking_flow = _brake.get_handle_braking_flow()
    readings.handle_release_flow = _brake.get_handle_release_flow()
    readings.handle_emergency_flow = _brake.get_handle_emergency_flow()
    readings.handle_control_chamber_flow = _brake.get_handle_control_chamber_flow()
    readings.handle_timing_reservoir_flow = _brake.get_handle_timing_reservoir_flow()
    readings.max_speed = _controller.max_velocity if _controller else 0.0
    readings.max_cylinder_pressure = _brake.max_cylinder_pressure
    readings.max_control_pressure = (
            _brake.max_cylinder_pressure if _brake.max_aux_pressure < AUX_PRESSURE_MIN else _brake.max_aux_pressure)
    readings.handle_sounds = (
            _brake.cntrl_brake_handle_type == RailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A
            or _brake.cntrl_brake_handle_type == RailVehicleBrake.BRAKE_HANDLE_TYPE_FVEL6)
    readings.electro_pneumatic = _brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC


## Train.cpp:8530-8584 - an FV4a/FVel6 hiss plays at half its volume, past its threshold
func _handle_hiss(sound:BrakeSound, volume:float, results:Dictionary) -> void:
    if volume * HANDLE_VOLUME_SCALE > HANDLE_HISS_THRESHOLD:
        _loop(sound, volume * HANDLE_VOLUME_SCALE, sound.pitch, results)
        return
    _stop(sound, results)


## sound_source::gain(), pitch() and play(exclusive | looping)
func _loop(sound:BrakeSound, gain:float, pitch:float, results:Dictionary) -> void:
    sound.pitch = pitch
    var action:Action = Action.LOOP if sound.playing else Action.START
    sound.playing = true
    _set_gain(sound, gain, results)
    results[sound.event_name]["action"] = action


## sound_source::gain() alone, heard only while the sound plays
func _set_gain(sound:BrakeSound, gain:float, results:Dictionary) -> void:
    sound.gain = clampf(gain, 0.0, MAX_GAIN)
    if not sound.playing:
        return
    var parameters:Dictionary = {&"gain": sound.gain}
    if sound.source.chunks:
        parameters[&"point"] = RunningSoundModel.combined_point(sound.pitch)
    else:
        parameters[&"pitch"] = sound.pitch
    results[sound.event_name] = {"action": Action.LOOP, "parameters": parameters, "source": sound.source}


## "don't stop the sound too abruptly" - the gain falls by `decrement`, the sound stops when it is
## quiet enough (Train.cpp:8496-8501, DynObj.cpp:4654-4659)
func _fade(sound:BrakeSound, decrement:float, results:Dictionary) -> void:
    _set_gain(sound, maxf(0.0, sound.gain - decrement), results)
    if sound.gain < STOP_GAIN:
        _stop(sound, results)


## sound_source::stop(): the loop is cut and its closing bookend plays. A one-shot is stopped
## whether or not it still plays.
func _stop(sound:BrakeSound, results:Dictionary) -> void:
    if not sound.playing and not sound.source.label in ONE_SHOT_LABELS:
        return
    sound.playing = false
    results[sound.event_name] = {"action": Action.STOP, "parameters": {}, "source": sound.source}


## sound_source::pitch() and play() of a single sample; a combined one plays the chunks at the
## point `pitch` selects (sound.cpp:427-486)
func _one_shot(sound:BrakeSound, pitch:float, action:Action, results:Dictionary) -> void:
    sound.pitch = pitch
    if not sound.source.chunks:
        results[sound.event_name] = {
            "action": action, "parameters": {&"gain": MAX_GAIN, &"pitch": pitch}, "source": sound.source,
        }
        return
    for chunk:Dictionary in RunningSoundModel.chunk_levels(sound.source, RunningSoundModel.combined_point(pitch)):
        results[sound.chunk_events[chunk["index"]]] = {
            "action": action,
            "parameters": {&"gain": clampf(chunk["gain"], 0.0, MAX_GAIN), &"pitch": chunk["pitch"]},
            "source": sound.source,
        }
