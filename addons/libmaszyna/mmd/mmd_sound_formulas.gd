extends RefCounted
class_name MmdSoundFormulas

## The original's sound formulas of one vehicle bank, as the bank's own data: the raw quantities it
## declares (SfxBank.parameters, fed by MaszynaVehicleSoundServer by their names), the gains, pitches
## and chunk points computed from them (SfxBank.derived_parameters, Godot expressions of them, with
## `previous` and `delta` for the fades and slews), and the triggers that start and stop the events.
## The cab's (TTrain::update_sounds(), Train.cpp:8474-8641) and the vehicle's (TDynamicObject's brake
## section, DynObj.cpp:4545-4760, and its running sounds, DynObj.cpp:3477-3545, 4630-4763,
## 7933-8092, 8244-8291). The vehicle's configuration and the MMD source's factors are written into
## the expressions - a bank is built for one vehicle.

## Operator, 2026-10-25: the original eases by a share per rendered frame - a time constant here,
## taken at this frame rate
const REFERENCE_FRAME_RATE:float = 60.0
## A gate opens and closes on an "active" parameter of 1 and 0
const GATE_THRESHOLD:float = 0.5
## OpenAL's default AL_MAX_GAIN: a brake sound is never heard above its recorded level (sound.cpp:853)
const MAX_BRAKE_GAIN:float = 1.0
## sound_source::gain() clamps at 2 (sound.cpp:853)
const MAX_GAIN:float = 2.0
## A fading or quiet sound stops below this gain (Train.cpp:8500, DynObj.cpp:4657, 4745, 8330)
const STOP_GAIN:float = 0.05
## An eased parameter this close to its target is at it
const EASED_TOLERANCE:float = 0.000001
## Labels played once per event rather than looped (DynObj.cpp:4547-4615, 4718-4723, 4749-4759)
const ONE_SHOT_LABELS:Array[String] = [
    "brakeacc", "brakecylinderinc", "brakecylinderdec", "epbrakeinc", "epbrakedec",
    "springbrake", "springbrakeoff",
]
## MaszynaMoverRailVehicleBrake: an auxiliary pressure below this is not given, and MaxBrakePress[0]
## is the cylinders' (Mover.cpp, LoadFIZ_Brake)
const AUX_PRESSURE_MIN:float = 0.01
## DynObj.cpp:6205 - each vehicle's squeal is up to 5% off its declared amplitude offset
const SQUEAL_OFFSET_MIN:float = 0.95
const SQUEAL_OFFSET_MAX:float = 1.05
## The combined motor and engine sounds select their chunks in hundredths of revolutions a minute
## (DynObj.cpp:8448-8451, 8252-8257)
const COMBINED_REVOLUTIONS_NORMALIZER:float = 60.0 * 0.01
## The traction motor's amplitude factor is over its nmax and three times the power (DynObj.cpp:5714)
const MOTOR_AMPLITUDE_POWER_FACTOR:float = 3.0
## A car's engine as the Dumb one sounds: its amplitude factor over three times the power
## (DynObj.cpp:6014-6021), its pitch lowered by the engine power over a thousandfold power
## (DynObj.cpp:8262-8264)
const CAR_ENGINE_AMPLITUDE_POWER_DIVISOR:float = 3.0
const CAR_ENGINE_PITCH_POWER_FACTOR:float = 0.2
const CAR_ENGINE_PITCH_POWER_DIVISOR:float = 1000.0
## The diesel turbo (DynObj.cpp:8266-8290): its lowest goal pitch, its rate of change [1/s], the share
## of it a falling pitch changes by, the engine speed it needs, its pitch mapping
const TURBO_MIN_PITCH:float = 0.025
const TURBO_CHANGE_RATE:float = 0.4
const TURBO_PITCH_FALL_SHARE:float = 0.5
const TURBO_MIN_ENGINE_SPEED:float = 0.1
const TURBO_PITCH_BASE:float = 0.4
const TURBO_PITCH_SCALE:float = 0.4
const GRAVITY:float = 9.81
## Where an outernoise bogie copy starts in its sample, as a fraction of it: an even copy in the
## later range, an odd one in the earlier, so two neighbours never start close (DynObj.cpp:6505-6514)
const BOGIE_NOISE_EVEN_START_MIN:float = 0.5
const BOGIE_NOISE_EVEN_START_MAX:float = 0.8
const BOGIE_NOISE_ODD_START_MIN:float = 0.0
const BOGIE_NOISE_ODD_START_MAX:float = 0.3
## Labels whose events sit at the cab's controls: the driver's brake valve, the local brake
## (Train.cpp:9406-9421)
const BRAKE_VALVE_LABELS:Array[String] = ["airsound", "airsound2", "airsound3", "airsound4", "airsound5"]
const LOCAL_BRAKE_LABELS:Array[String] = ["localbrakesound", "localbrakesound2"]
const PLACEMENTS:Dictionary[StringName, MaszynaVehicleSoundServer.Placement] = {
    &"internal": MaszynaVehicleSoundServer.PLACEMENT_INTERNAL,
    &"engine": MaszynaVehicleSoundServer.PLACEMENT_ENGINE,
    &"external": MaszynaVehicleSoundServer.PLACEMENT_EXTERNAL,
    &"custom": MaszynaVehicleSoundServer.PLACEMENT_CUSTOM,
}

var parameters:Array[SfxParameter] = []
var derived_parameters:Array[SfxDerivedParameter] = []
## MaszynaVehicleSoundServer.bank_register()'s "proofing", "axles" and the cab's anchors
var proofing:Array[Dictionary] = []
var axles:PackedFloat32Array = PackedFloat32Array()
var brake_valve_events:PackedStringArray = PackedStringArray()
var local_brake_events:PackedStringArray = PackedStringArray()
var tachometer_events:PackedStringArray = PackedStringArray()

var _declared:Dictionary[StringName, bool] = {}
var _controller:VehicleController
var _road:RoadVehicleController
var _brake:MaszynaMoverRailVehicleBrake
var _engine:RailVehicleEngine
var _wheels:RailVehicleWheels
## The controller's Vmax and power, none for a bank built before its vehicle has a controller
var _max_velocity:float = 0.0
var _power:float = 0.0


func _init(vehicle_rid:RID) -> void:
    _controller = VehicleServer.vehicle_get_controller(vehicle_rid)
    _road = _controller as RoadVehicleController
    _brake = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_BRAKES) as MaszynaMoverRailVehicleBrake
    _engine = VehicleServer.vehicle_component_get(
            vehicle_rid, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    _wheels = VehicleServer.vehicle_component_get(
            vehicle_rid, VehicleComponentType.COMPONENT_WHEELS) as RailVehicleWheels
    if _controller:
        _max_velocity = _controller.max_velocity
        _power = _controller.power


## The bank's parameters, its own and its derived ones
func apply(bank:SfxBank) -> void:
    bank.parameters = parameters
    bank.derived_parameters = derived_parameters


## A brake sound's event, played by its formula (Train.cpp:8474-8641, DynObj.cpp:4545-4760): a loop
## with its bookends or its chunks crossfaded on its point, or a one-shot of the chunk its value picks
func brake_sound(definition:MmdSoundSourceDefinition, key:StringName, events:Array[SfxEvent]) -> void:
    var one_shot:bool = definition.label in ONE_SHOT_LABELS
    var event:SfxEvent = _event(definition, key, key, one_shot)
    var emitter:Array[SfxEvent] = [event]
    MmdSoundEventBuilder.shape_emitter(emitter, definition, definition.start_offset)
    events.append(event)
    if definition.offset == Vector3.ZERO and definition.label in BRAKE_VALVE_LABELS:
        brake_valve_events.append(key)
    elif definition.offset == Vector3.ZERO and definition.label in LOCAL_BRAKE_LABELS:
        local_brake_events.append(key)
    var ao:String = _n(definition.amplitude_offset)
    var af:String = _n(definition.amplitude_factor)
    var tone:String = _n(definition.frequency_offset + definition.frequency_factor)
    var speed:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_SPEED)
    var vmax:String = _n(1.0 + _max_velocity)
    var cylinder:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE)
    var max_cylinder:String = _n(maxf(1.0, _brake.max_cylinder_pressure) if _brake else 1.0)
    match definition.label:
        # Train.cpp:8480-8517 - the local brake's hiss: ten times the eased rate of its cylinder's
        # pressure, faded out at 0.1/s
        "localbrakesound", "localbrakesound2":
            var local:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_LOCAL_PRESSURE)
            var rate:StringName = _derive(&"local_brake_rate", local, [local], SfxDerivedParameter.DERIVATIVE)
            var change:StringName = _ease(&"local_brake_change", "10.0 * %s" % rate, [rate], 0.1)
            var condition:String = (
                    "%s < -0.05 and %s > %s - 0.05" % [change, local, cylinder] if definition.label == "localbrakesound"
                    else "%s > 0.05" % change)
            var level:String = "abs(%s) * 0.05" % change
            _hiss(event, key, "clamp(%s + %s * %s, 0.0, 1.0)" % [ao, af, level], condition, 0.1, [change, local, cylinder])
            _tone(event, definition, key, "1.0", [])
        # Train.cpp:8519-8618 - the driver's brake valve
        "airsound", "airsound2", "airsound3", "airsound4", "airsound5":
            _brake_valve(definition, event, key, ao, af)
        # DynObj.cpp:4547-4575 - a cylinder click per 1/15 of its full pressure, the combined one at
        # its step's chunk, a single one at its own pitch
        "brakecylinderinc", "brakecylinderdec":
            var step:StringName = _derive(&"brake_cylinder_step",
                    "floor(15.0 * max(0.0, %s) / %s)" % [cylinder, max_cylinder], [cylinder])
            _one_shot(event, definition, key, step,
                    SfxParameterTrigger.UP if definition.label == "brakecylinderinc" else SfxParameterTrigger.DOWN,
                    "%s * 0.01" % step, [step], tone, false)
        # DynObj.cpp:4578-4617 - an EP click, counted by MaszynaVehicleSoundServer; the combined one
        # at the chunk of its step (an increase) or of the change (a decrease)
        "epbrakeinc", "epbrakedec":
            var increase:bool = definition.label == "epbrakeinc"
            var count:StringName = _quantity(
                    MaszynaVehicleSoundServer.QUANTITY_EP_BRAKE_INCREASE_COUNT if increase
                    else MaszynaVehicleSoundServer.QUANTITY_EP_BRAKE_DECREASE_COUNT)
            var pitch:StringName = _quantity(
                    MaszynaVehicleSoundServer.QUANTITY_EP_BRAKE_INCREASE_PITCH if increase
                    else MaszynaVehicleSoundServer.QUANTITY_EP_BRAKE_DECREASE_PITCH)
            _one_shot(event, definition, key, count, SfxParameterTrigger.UP, pitch, [pitch], tone, false)
        # DynObj.cpp:4620-4637 - the emergency valve, started and stopped with a hysteresis
        "emergencybrake":
            var flow:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW)
            var pipe:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_PIPE_PRESSURE)
            var eased:StringName = _derive(&"emergency_brake_flow",
                    _pick("%s > 0.025" % flow,
                        _pick("previous == 0.0", flow, "lerp(previous, %s, %s)" % [flow, _easing(0.1)]),
                        _pick("%s < 0.015" % flow, "0.0", "previous")), [flow])
            var gain:StringName = _derive(StringName("%s_gain" % key),
                    _pick("%s > 0.025" % flow, "clamp(%s + clamp(clamp(%s, 0.0, 1.0) + clamp(0.1 * %s, 0.0, 0.5), 0.0, 1.0) * %s, 0.0, 1.0)"
                    % [ao, eased, pipe, af], "previous"), [eased, pipe, flow])
            var active:StringName = _derive(StringName("%s_active" % key),
                    _pick("%s > 0.025" % flow, "1.0", _pick("%s < 0.015" % flow, "0.0", "previous")), [flow])
            _gate(event, active)
            _modulate(event, gain, SfxParameterModulation.Target.GAIN)
            _tone(event, definition, key, tone, [])
        # DynObj.cpp:4639-4660 - the cylinder's release: the eased rate of its pressure drop, faded
        # out at 0.5/s
        "unbrake":
            var rate:StringName = _derive(&"brake_cylinder_rate", cylinder, [cylinder], SfxDerivedParameter.DERIVATIVE)
            var change:StringName = _ease(&"brake_release_change", "-%s" % rate, [rate], 0.05)
            var ratio:String = "max(0.0, %s) / %s" % [cylinder, max_cylinder]
            _hiss(event, key, "clamp(%s * %s * (0.25 + 0.75 * %s), 0.0, 1.0)" % [af, change, ratio],
                    "%s > 0.05 and %s > 0.05" % [change, ratio], 0.5, [change, cylinder])
            _tone(event, definition, key, "1.0", [])
        # DynObj.cpp:4662-4673 - the releaser, louder with the cylinder pressure
        "releaser":
            var releaser:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_RELEASER_ACTIVE)
            _gate(event, releaser)
            _modulate(event, _derive(StringName("%s_gain" % key), "clamp(%s * 1.25, 0.0, 1.0)" % cylinder, [cylinder]),
                    SfxParameterModulation.Target.GAIN)
            _tone(event, definition, key, "1.0", [])
        # DynObj.cpp:4676-4689 - slipping wheels; the amplitude factor is over 1 + Vmax (DynObj.cpp:6776)
        "slipperysound":
            var slipping:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_WHEELS_SLIPPING)
            var force:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE)
            var condition:String = "%s > 0.5 and %s > 100.0 and %s > 1.0" % [slipping, force, speed]
            var per_speed:float = (definition.amplitude_factor / (1.0 + _max_velocity) / _max_velocity
                    if _max_velocity > 0.0 else 0.0)
            _modulate(event, _derive(StringName("%s_gain" % key),
                    _pick(condition, "clamp(%s + %s * %s, 0.0, 1.0)" % [ao, _n(per_speed), speed], "previous"),
                    [slipping, force, speed]), SfxParameterModulation.Target.GAIN)
            _gate(event, _derive(StringName("%s_active" % key),
                    _pick(condition, "1.0", _pick("%s > 0.5" % slipping, "previous", "0.0")), [slipping, force, speed]))
            _tone(event, definition, key, "1.0", [])
        # DynObj.cpp:4697-4716 (and its cab copy, Train.cpp:8621-8641) - the shoes; the frequency
        # factor is over 1 + Vmax (DynObj.cpp:6192, Train.cpp:9091)
        "brakesound":
            var braking:String = _braking_condition(speed)
            var ratio:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO)
            var force:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE)
            _gate(event, _derive(StringName("%s_active" % key), "float(%s)" % braking, [force, speed]))
            _modulate(event, _derive(StringName("%s_gain" % key),
                    "clamp(%s + sqrt(%s * lerp(0.4, 1.0, %s / %s)) * %s, 0.0, 1.0)" % [ao, ratio, speed, vmax, af],
                    [ratio, speed]), SfxParameterModulation.Target.GAIN)
            _tone(event, definition, key, "%s + %s * %s / %s" % [_n(definition.frequency_offset), speed,
                    _n(definition.frequency_factor), vmax], [speed])
        # DynObj.cpp:4718-4723 - the accelerator, not started again while it plays
        "brakeacc":
            _one_shot(event, definition, key, _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_ACCELERATOR_COUNT),
                    SfxParameterTrigger.UP, "1.0", [], "1.0", true)
        # DynObj.cpp:4725-4746 - the squeal of hard pressed brakes, faded out at 2.5 of its gain a
        # second below 2.5 km/h
        "brake":
            var braking:String = _braking_condition(speed)
            var ratio:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO)
            var force:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE)
            var offset:String = _n(definition.amplitude_offset * randf_range(SQUEAL_OFFSET_MIN, SQUEAL_OFFSET_MAX))
            var target:StringName = _derive(StringName("%s_target" % key),
                    "%s + lerp(-1.0, 1.0, %s) * %s" % [offset, _pick(braking, ratio, "0.0"), af], [ratio, force, speed])
            _squeal(event, definition, key, speed, target)
        # DynObj.cpp:4749-4759 - the spring brake, once as it holds and once as it lets go
        "springbrake", "springbrakeoff":
            var spring:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_SPRING_BRAKE_ACTIVE)
            var trigger := SfxParameterTrigger.new()
            trigger.parameter = spring
            trigger.mode = SfxParameterTrigger.CROSS
            trigger.on_threshold = GATE_THRESHOLD
            trigger.direction = SfxParameterTrigger.UP if definition.label == "springbrake" else SfxParameterTrigger.DOWN
            trigger.exclusive = true
            _add_trigger(event, trigger)


## A running sound's events, one per location: traction motors and bogie noise are repeated at every
## `tractionmotors:`/`bogies:` offset, keeping the x/y of the sound's own offset (DynObj.cpp:5722-5738,
## 6137-6146); a wheel clatter definition is one axle already. MMD offsets are in the original
## vehicle frame (+Z forward), turned by 180 degrees into the vehicle's -Z forward frame.
func running_sound(
        definition:MmdSoundSourceDefinition, key:StringName, locations:Dictionary, events:Array[SfxEvent]) -> void:
    var offsets:Array[Vector3] = [definition.offset]
    var location_key:String = {"tractionmotor": "tractionmotors", "outernoise": "bogies"}.get(definition.label, "")
    if location_key and locations[location_key]:
        offsets.clear()
        for location:float in locations[location_key]:
            offsets.append(Vector3(definition.offset.x, definition.offset.y, -location))
    var one_shot:bool = definition.label == "wheel_clatter"
    for index:int in range(offsets.size()):
        var event_id:StringName = key
        if one_shot:
            event_id = StringName("%s_%d" % [key, axles.size()])
        elif offsets.size() > 1:
            event_id = StringName("%s_%d" % [key, index])
        var event:SfxEvent = _event(definition, event_id, key, one_shot)
        event.spatial_config.position = Vector3(-offsets[index].x, offsets[index].y, -offsets[index].z)
        # every copy its own start, as the original sets it when it reads the MMD: bogies
        # alternate between two ranges (DynObj.cpp:6505-6514), located motors start anywhere
        # (DynObj.cpp:6085), anything else where the MMD's startoffset: says
        var start_fraction:float = definition.start_offset
        if definition.label == "outernoise":
            start_fraction = (randf_range(BOGIE_NOISE_ODD_START_MIN, BOGIE_NOISE_ODD_START_MAX) if index % 2
                    else randf_range(BOGIE_NOISE_EVEN_START_MIN, BOGIE_NOISE_EVEN_START_MAX))
        elif definition.label == "tractionmotor" and location_key and locations[location_key]:
            start_fraction = randf()
        var emitter:Array[SfxEvent] = [event]
        MmdSoundEventBuilder.shape_emitter(emitter, definition, start_fraction)
        events.append(event)
        _running_formula(definition, event, key)


## A sound a quantity of the vehicle drives - a state, an event count (MmdSoundCatalog's
## state_property, a MaszynaVehicleSoundServer quantity by its name) - by its TriggerMode: playing
## while it is within the thresholds, or once whenever it changes
func regular_sound(definition:MmdSoundSourceDefinition, entry:Dictionary, event_name:StringName) -> SfxEvent:
    var mode:MmdSoundCatalog.TriggerMode = entry["trigger_mode"]
    var value:StringName = StringName(entry["state_property"])
    _declare(value)
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, event_name,
            value if mode == MmdSoundCatalog.TriggerMode.CONTINUOUS else &"",
            not mode == MmdSoundCatalog.TriggerMode.CHANGE)
    _proof(event, definition, event_name)
    if event_name == &"tachoclock" and definition.offset == Vector3.ZERO:
        tachometer_events.append(event_name)
    var minimum:String = _n(float(entry.get("trigger_threshold_min", 0.0)))
    var maximum:String = _n(float(entry.get("trigger_threshold_max", 1.0)))
    match mode:
        MmdSoundCatalog.TriggerMode.CHANGE:
            var trigger := SfxParameterTrigger.new()
            trigger.parameter = value
            trigger.mode = SfxParameterTrigger.STEP
            _add_trigger(event, trigger)
        MmdSoundCatalog.TriggerMode.TOGGLE:
            _gate(event, _derive(StringName("%s_active" % event_name),
                    "float(abs(%s) > 0.0 and %s >= %s and %s <= %s)" % [value, value, minimum, value, maximum], [value]))
        MmdSoundCatalog.TriggerMode.CONTINUOUS:
            _gate(event, _derive(StringName("%s_active" % event_name),
                    "float(%s >= %s and %s <= %s)" % [value, minimum, value, maximum], [value]))
    # the engine's gain from its speed and load (TDynamicObject's engine section)
    if event_name == &"engine" and _engine:
        var power:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_POWER)
        var rpm_ratio:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_RPM_RATIO)
        var load:String = "max(%s / %s, 0.0)" % [power, _n(_power)] if _power > 0.0 else "0.0"
        _modulate(event, _derive(&"engine_gain",
                "clamp(%s + %s * (0.25 * %s + 0.75 * clamp(%s, 0.0, 1.0)), 0.0, 2.0)"
                % [_n(definition.amplitude_offset), _n(definition.amplitude_factor), load, rpm_ratio],
                [power, rpm_ratio]), SfxParameterModulation.Target.GAIN)
    return event


## MaszynaVehicleSoundServer.bank_register()'s registration of the bank built with these formulas
func registration(cabin_only:bool, soundproofing:Array[PackedFloat32Array]) -> Dictionary:
    return {
        "cabin_only": cabin_only,
        "soundproofing": soundproofing,
        "proofing": proofing,
        "axles": axles,
        "brake_valve_events": brake_valve_events,
        "local_brake_events": local_brake_events,
        "tachometer_events": tachometer_events,
    }


func _running_formula(definition:MmdSoundSourceDefinition, event:SfxEvent, key:StringName) -> void:
    var ao:String = _n(definition.amplitude_offset)
    var af:String = _n(definition.amplitude_factor)
    var fo:String = _n(definition.frequency_offset)
    var ff:String = _n(definition.frequency_factor)
    var combined:bool = not not definition.chunks
    var speed:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_SPEED)
    var vmax:float = _max_velocity
    match definition.label:
        "tractionmotor":
            _traction_motor(definition, event, key, ao, af, fo, ff, combined)
        # DynObj.cpp:8081-8092, divisors from DynObj.cpp:5806
        "ventilator":
            var series:RailVehicleElectricSeriesEngine = _engine as RailVehicleElectricSeriesEngine
            var max_rpm:float = series.resistor_fan_max_rpm if series else 0.0
            if max_rpm <= 0.0:
                return
            var fan:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_RESISTOR_FAN_ROTATION)
            _gate(event, _derive(StringName("%s_active" % key), "float(%s > 0.1)" % fan, [fan]))
            _modulate(event, _derive(StringName("%s_gain" % key), "clamp(%s + %s * %s, 0.0, 2.0)"
                    % [ao, _n(definition.amplitude_factor / max_rpm), fan], [fan]), SfxParameterModulation.Target.GAIN)
            _tone(event, definition, key, "%s + %s * %s" % [fo, _n(definition.frequency_factor / max_rpm), fan], [fan])
        # DynObj.cpp:4735-4763. AccN = V^2/R + g*dHrail/TrackW (Mover.cpp:1312); the cant always lifts
        # the outer rail, so it is taken against the centripetal part by magnitude
        "curve":
            var velocity:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_VELOCITY)
            var radius:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_CURVE_RADIUS)
            var cant:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_CURVE_CANT)
            var switch:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_TRACK_SWITCH)
            var track_width:String = _n(maxf(_wheels.track_width, 0.001) if _wheels else 1.435)
            var inputs:Array[StringName] = [speed, velocity, radius, cant, switch]
            var in_curve:String = "%s > 5.0 and abs(%s) * abs(%s) > 1.0 and abs(%s) < 15000.0" % [speed, radius, radius, radius]
            var volume:StringName = _derive(StringName("%s_volume" % key),
                    _pick(in_curve, "abs(%s * %s / abs(%s) - %s * abs(%s) / %s) * lerp(0.5, 1.0, clamp(%s / 40.0, 0.0, 1.0)) * %s"
                    % [velocity, velocity, radius, _n(GRAVITY), cant, track_width, speed,
                    _pick("%s > 0.5 and abs(%s) < 1500.0" % [switch, radius], "100.0", "1.0")], "0.0"), inputs)
            _running_level(event, key, "clamp(2.5 * %s, 0.0, 2.0)" % volume, "%s > 0.05" % volume, [volume])
            _tone(event, definition, key, "%s * 0.01" % speed if combined else "%s + %s" % [fo, ff], [speed])
        # DynObj.cpp:4630-4720, divisors from DynObj.cpp:6129; the trainset driven from the
        # listener's cab hears the cab's running noise instead (DynObj.cpp:4632-4640)
        "outernoise":
            var audible:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_OUTER_NOISE_AUDIBLE)
            var ratio:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO)
            var quality:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_TRACK_QUALITY)
            var inputs:Array[StringName] = [speed, audible, ratio, quality]
            # DynObj.cpp:4412-4420 - the ratio is only non-zero for a braking, moving vehicle
            var braking:String = "%s > 0.05" % speed
            if not _road:
                var force:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE)
                braking = _braking_condition(speed)
                inputs.append(force)
            var volume:StringName = _derive(StringName("%s_volume" % key),
                    "(%s + %s * %s) * %s * %s%s"
                    % [ao, _n(definition.amplitude_factor / (1.0 + vmax)), speed,
                    _pick(braking, "1.0 + 0.125 * %s" % ratio, "1.0"), _quality_volume(quality),
                    "" if combined else " * clamp(%s / 40.0, 0.0, 1.0)" % speed], inputs)
            _running_level(event, key, "clamp(%s, 0.0, 2.0)" % volume,
                    "%s > 0.5 and %s > 0.5 and %s > 0.05" % [speed, audible, volume], [speed, audible, volume])
            _tone(event, definition, key, "%s + %s * %s" % [fo, _n(definition.frequency_factor / (1.0 + vmax)
                    * (vmax * 0.01 if combined else 1.0)), speed], [speed])
        # Train.cpp:8272-8282 and update_sounds_runningnoise(), frequency divisor from Train.cpp:8636
        "runningnoise":
            var quality:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_TRACK_QUALITY)
            var inputs:Array[StringName] = [speed, quality]
            var braking:String = "1.0"
            if _brake:
                var ratio:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO)
                var rotation:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_WHEELS_ROTATION)
                braking = _pick("%s > 0.01" % rotation, "1.0 + 0.125 * %s" % ratio, "1.0")
                inputs.append_array([ratio, rotation])
            var volume:StringName = _derive(StringName("%s_volume" % key),
                    "(%s + %s * lerp(%s / %s, 1.0, 0.5)) * %s * %s%s"
                    % [ao, af, speed, _n(1.0 + vmax), braking, _quality_volume(quality),
                    "" if combined else " * clamp(%s / 25.0, 0.0, 1.0)" % speed], inputs)
            _running_level(event, key, "clamp(%s, 0.0, 2.0)" % volume, "%s > 0.5 and %s > 0.05" % [speed, volume],
                    [speed, volume])
            _tone(event, definition, key, "%s + %s * %s" % [fo, _n(definition.frequency_factor / (1.0 + vmax)
                    * (vmax * 0.01 if combined else 1.0)), speed], [speed])
        "turbo":
            _turbo(definition, event, key, ao, af, fo, ff)
        # DynObj.cpp:8244-8291 - a car's engine as the original sounds a Dumb one: idling while
        # started, its pitch from the engine's revolutions, its gain from the engine power (the power
        # times how much the car accelerates) and revolutions, eased by a quarter an update
        "engine":
            var power:float = _power
            if power <= 0.0:
                return
            var running:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_CAR_ENGINE_RUNNING)
            var revolutions:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_CAR_ENGINE_REVOLUTIONS)
            var throttle:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_CAR_THROTTLE)
            var volume:String = "%s + %s * (%s * %s + %s * %s)" % [ao,
                    _n(definition.amplitude_factor / (CAR_ENGINE_AMPLITUDE_POWER_DIVISOR * power)), _n(power), throttle,
                    revolutions, _n(Units.SECONDS_PER_MINUTE)]
            var gain:StringName = _derive(StringName("%s_gain" % key),
                    _pick("%s > 0.5" % running, _eased(volume, 0.25), "0.0"),
                    [running, revolutions, throttle])
            _gate(event, _derive(StringName("%s_active" % key), "float(%s >= %s)" % [gain, _n(STOP_GAIN)], [gain]))
            _modulate(event, _derive(StringName("%s_level" % key), "clamp(%s, 0.0, 2.0)" % gain, [gain]),
                    SfxParameterModulation.Target.GAIN)
            _tone(event, definition, key, "%s + %s * %s - %s * %s" % [fo,
                    _n(definition.frequency_factor * (COMBINED_REVOLUTIONS_NORMALIZER if combined else 1.0)), revolutions,
                    _n(CAR_ENGINE_PITCH_POWER_FACTOR * power / (1.0 + power * CAR_ENGINE_PITCH_POWER_DIVISOR)), throttle],
                    [revolutions, throttle])
        # DynObj.cpp:4693-4739 - a car's brake squeal, as hard as it brakes
        "brake":
            var ratio:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO)
            var target:StringName = _derive(StringName("%s_target" % key),
                    "%s + lerp(-1.0, 1.0, %s) * %s" % [ao, ratio, af], [ratio])
            _squeal(event, definition, key, speed, target)
        # DynObj.cpp:3477-3545 - an axle's click at every rail joint MaszynaVehicleSoundServer counts;
        # the combined clatter picks its chunk by the speed (DynObj.cpp:3525)
        "wheel_clatter":
            var clicks:StringName = StringName("axle_clicks_%d" % axles.size())
            axles.append(definition.offset.z)
            _declare(clicks)
            var quality:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_TRACK_QUALITY)
            var environment:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_TRACK_ENVIRONMENT)
            _modulate(event, _derive(&"wheel_clatter_gain",
                    "clamp(%s * %s, 0.0, 2.0)" % [_quality_volume(quality),
                    _pick("%s == %d" % [environment, MaszynaTrackData.TUNNEL], "1.1",
                    _pick("%s == %d" % [environment, MaszynaTrackData.BRIDGE], "1.2", "1.0"))],
                    [quality, environment]), SfxParameterModulation.Target.GAIN)
            var trigger := SfxParameterTrigger.new()
            trigger.parameter = clicks
            trigger.mode = SfxParameterTrigger.STEP
            _add_trigger(event, trigger)
            if combined:
                _derive(&"wheel_clatter_point", _point("%s * 0.01" % speed), [speed])


## DynObj.cpp:8441-8527 - while the wheels turn: the motors' pitch from their revolutions, their gain
## from the power and revolutions by the engine's kind, scaled by the momentum MaszynaVehicleSoundServer
## integrates and eased by a quarter an update; standing, they keep what they had
func _traction_motor(
        definition:MmdSoundSourceDefinition, event:SfxEvent, key:StringName, ao:String, af:String,
        fo:String, ff:String, combined:bool) -> void:
    if not _engine:
        return
    # nmax is the Mover's model of the engine, which each engine kind has its own adapter of
    var max_rpm:float = 0.0
    if _engine is MaszynaMoverRailVehicleElectricSeriesEngine:
        max_rpm = (_engine as MaszynaMoverRailVehicleElectricSeriesEngine).get_circuit_nmax_rpm()
    elif _engine is MaszynaMoverRailVehicleElectricInductionEngine:
        max_rpm = (_engine as MaszynaMoverRailVehicleElectricInductionEngine).get_circuit_nmax_rpm()
    elif _engine is MaszynaMoverRailVehicleDieselElectricEngine:
        max_rpm = (_engine as MaszynaMoverRailVehicleDieselElectricEngine).get_circuit_nmax_rpm()
    elif _engine is MaszynaMoverRailVehicleDieselEngine:
        max_rpm = (_engine as MaszynaMoverRailVehicleDieselEngine).get_circuit_nmax_rpm()
    var rotation:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_WHEELS_ROTATION)
    var engine_power:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_POWER)
    var momentum:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_MOTOR_MOMENTUM)
    var revolutions:String = "%s * %s" % [rotation, _n(_engine.get_transmission_ratio())]
    var divisor:float = max_rpm + _power * MOTOR_AMPLITUDE_POWER_FACTOR
    var factor:String = _n(definition.amplitude_factor / divisor if divisor > 0.0 else 0.0)
    var inputs:Array[StringName] = [rotation, engine_power, momentum]
    var volume:String = "%s + %s * %s * %s" % [ao, factor, revolutions, _n(Units.SECONDS_PER_MINUTE)]
    match _engine.get_type():
        RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
            volume = "%s + %s * (%s + %s * 2.0)" % [ao, factor, engine_power, revolutions]
        RailVehicleEngine.ELECTRIC_SERIES_MOTOR:
            var rpm:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_REVOLUTIONS)
            var dynamic_brake:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_DYNAMIC_BRAKE_ACTIVE)
            var base:StringName = _derive(StringName("%s_base" % key), "%s + %s * (%s + %s * %s)"
                    % [ao, factor, engine_power, revolutions, _n(Units.SECONDS_PER_MINUTE)], [rotation, engine_power])
            # the volume's variation (DynObj.cpp:8476-8483) and the dynamic brake heard (DynObj.cpp:8485-8489)
            var variation:String = "randf() * 100.0 * %s / %s" % [rpm, _n(1.0 + max_rpm / Units.SECONDS_PER_MINUTE)]
            var varied:StringName = _derive(StringName("%s_variation" % key), variation, [rpm])
            volume = "%s + %s + %s" % [base,
                    _pick("%s < 1.0 and %s < 100.0 and %s < 2.0" % [base, engine_power, varied], "%s / 200.0" % varied, "0.0"),
                    _pick("%s > 0.5 and %s > 0.1" % [dynamic_brake, engine_power], "0.8", "0.0")]
            inputs.append_array([base, varied, dynamic_brake])
    var gain:StringName = _derive(StringName("%s_gain" % key),
            _pick("%s > 0.01" % rotation, _eased("(%s) * max(0.25, %s)" % [volume, momentum], 0.25), "previous"),
            inputs)
    _gate(event, _derive(StringName("%s_active" % key),
            _pick("%s > 0.01" % rotation, "float(%s >= %s)" % [gain, _n(STOP_GAIN)], "previous"), [gain, rotation]))
    _modulate(event, _derive(StringName("%s_level" % key), "clamp(%s, 0.0, 2.0)" % gain, [gain]),
            SfxParameterModulation.Target.GAIN)
    _tone(event, definition, key, "%s + %s * %s" % [fo,
            _n(definition.frequency_factor * _engine.get_transmission_ratio()
            * (COMBINED_REVOLUTIONS_NORMALIZER if combined else 1.0)), rotation], [rotation])


## DynObj.cpp:8266-8290: a diesel's turbocharger, heard from the master controller position
## TurboPos: on while the engine turns; its pitch and gain move toward their goals at a rate
func _turbo(
        definition:MmdSoundSourceDefinition, event:SfxEvent, key:StringName, ao:String, af:String,
        fo:String, ff:String) -> void:
    var diesel:RailVehicleDieselEngine = _engine as RailVehicleDieselEngine
    if not diesel or diesel.turbo_position <= 0:
        return
    var rpm:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_REVOLUTIONS)
    var fill:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_ENGINE_FILL)
    var position:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_MASTER_CONTROLLER_POSITION)
    var pitch_diesel:String = (
            "%s / %s * %s" % [rpm, _n(diesel.mechanical_max_rpm), fill] if diesel.mechanical_max_rpm > 0.0 else "0.0")
    var goal_pitch:String = "max(%s, (%s + %s) * %s)" % [_n(TURBO_MIN_PITCH), pitch_diesel, fo, ff]
    var change:String = "%s * delta" % _n(TURBO_CHANGE_RATE)
    var pitch:StringName = _derive(StringName("%s_turbo_pitch" % key),
            _pick("previous > %s" % goal_pitch, "max(%s, previous - %s * %s)" % [goal_pitch, change, _n(TURBO_PITCH_FALL_SHARE)],
            "min(%s, previous + %s)" % [goal_pitch, change]), [rpm, fill])
    var heard:String = "%s >= %d and %s > %s" % [position, diesel.turbo_position, rpm, _n(TURBO_MIN_ENGINE_SPEED)]
    var goal_volume:String = _pick(heard, "max(0.0, (%s + %s) * %s)" % [pitch, ao, af], "0.0")
    var volume:StringName = _derive(StringName("%s_gain" % key),
            _pick("previous > %s" % goal_volume, "max(%s, previous - %s)" % [goal_volume, change],
            "min(%s, previous + %s)" % [goal_volume, change]), [pitch, position, rpm])
    _gate(event, _derive(StringName("%s_active" % key), "float(%s > %s)" % [volume, _n(STOP_GAIN)], [volume]))
    _modulate(event, volume, SfxParameterModulation.Target.GAIN)
    _tone(event, definition, key, "%s + %s * %s" % [_n(TURBO_PITCH_BASE), pitch, _n(TURBO_PITCH_SCALE)], [pitch])


## Train.cpp:8519-8618 - the driver's brake valve: an FV4a/FVel6 hisses with its handle's eased
## flows at half their volume, any other with the main valve's flow averaged over 4+1 updates
func _brake_valve(definition:MmdSoundSourceDefinition, event:SfxEvent, key:StringName, ao:String, af:String) -> void:
    var fv4a:bool = _brake and (
            _brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A
            or _brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FVEL6)
    var volume:String = ""
    var inputs:Array[StringName] = []
    var threshold:float = 0.05
    if fv4a:
        var flows:Dictionary[String, MaszynaVehicleSoundServer.Quantity] = {
            "airsound": MaszynaVehicleSoundServer.QUANTITY_BRAKE_HANDLE_BRAKING_FLOW,
            "airsound2": MaszynaVehicleSoundServer.QUANTITY_BRAKE_HANDLE_RELEASE_FLOW,
            "airsound3": MaszynaVehicleSoundServer.QUANTITY_BRAKE_HANDLE_EMERGENCY_FLOW,
            "airsound4": MaszynaVehicleSoundServer.QUANTITY_BRAKE_HANDLE_CONTROL_CHAMBER_FLOW,
            "airsound5": MaszynaVehicleSoundServer.QUANTITY_BRAKE_HANDLE_TIMING_RESERVOIR_FLOW,
        }
        var flow:StringName = _quantity(flows[definition.label])
        match definition.label:
            "airsound":
                var eased:StringName = _ease(StringName("%s_flow" % key), flow, [flow], 0.05)
                volume = _pick("%s > 0.0" % eased, "%s * %s * 0.25 + %s" % [af, eased, ao], "0.0")
                inputs = [eased]
            "airsound2":
                var eased:StringName = _ease(StringName("%s_flow" % key), flow, [flow], 0.25)
                volume = _pick("%s > 0.0" % eased, "%s * %s + %s" % [af, eased, ao], "0.0")
                inputs = [eased]
            _:
                volume = "(%s * %s + %s)" % [flow, af, ao]
                inputs = [flow]
        volume = "%s * 0.5" % volume
    else:
        if not definition.label in ["airsound", "airsound2"]:
            return
        var main:StringName = _quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_MAIN_VALVE_FLOW)
        var braking:bool = definition.label == "airsound"
        var eased:StringName = _ease(StringName("%s_flow" % key), ("max(0.0, %s)" if braking else "min(0.0, %s)") % main,
                [main], 0.2)
        volume = (_pick("%s > 0.0" % eased, "2.0 * %s * %s + %s" % [af, eased, ao], "0.0") if braking
                else _pick("%s < 0.0" % eased, "-%s * %s + %s" % [af, eased, ao], "0.0"))
        threshold = 0.05 if braking else 0.01
        inputs = [eased]
    var level:StringName = _derive(StringName("%s_volume" % key), volume, inputs)
    _gate(event, _derive(StringName("%s_active" % key), "float(%s > %s)" % [level, _n(threshold)], [level]))
    _modulate(event, _derive(StringName("%s_gain" % key), "clamp(%s, 0.0, 1.0)" % level, [level]),
            SfxParameterModulation.Target.GAIN)
    _tone(event, definition, key, "1.0", [])


## A hiss played at `gain` while `condition` holds, and else faded out by `fade` a second until it is
## quiet enough to stop ("don't stop the sound too abruptly", Train.cpp:8496-8501, DynObj.cpp:4654-4659)
func _hiss(event:SfxEvent, key:StringName, gain:String, condition:String, fade:float, inputs:Array[StringName]) -> void:
    var level:StringName = _derive(StringName("%s_gain" % key),
            _pick(condition, gain, "max(0.0, previous - %s * delta)" % _n(fade)), inputs)
    var active_inputs:Array[StringName] = inputs.duplicate()
    active_inputs.append(level)
    _gate(event, _derive(StringName("%s_active" % key),
            _pick(condition, "1.0", "float(previous > 0.5 and %s >= %s)" % [level, _n(STOP_GAIN)]),
            active_inputs))
    _modulate(event, level, SfxParameterModulation.Target.GAIN)


## DynObj.cpp:4725-4746: above 2.5 km/h the squeal plays at its target once above 0.075, below it the
## gain falls by 2.5 of itself a second; quieter than 0.05 it stops
func _squeal(event:SfxEvent, definition:MmdSoundSourceDefinition, key:StringName, speed:StringName, target:StringName) -> void:
    var gain:StringName = _derive(StringName("%s_gain" % key),
            _pick("%s > 2.5" % speed, _pick("%s > 0.075" % target, "clamp(%s, 0.0, 1.0)" % target, "previous"),
            "max(0.0, previous - previous * 2.5 * delta)"), [target, speed])
    _gate(event, _derive(StringName("%s_active" % key),
            _pick("%s > 2.5 and %s > 0.075" % [speed, target], "1.0",
            _pick("%s < %s" % [_pick("%s > 2.5" % speed, target, gain), _n(STOP_GAIN)], "0.0", "previous")),
            [speed, target, gain]))
    _modulate(event, gain, SfxParameterModulation.Target.GAIN)
    _tone(event, definition, key, "%s * 0.01" % speed if definition.chunks
            else _n(definition.frequency_offset + definition.frequency_factor), [speed])


## A running sound's gain and its start and stop by `condition`
func _running_level(
        event:SfxEvent, key:StringName, gain:String, condition:String, inputs:Array[StringName]) -> void:
    _gate(event, _derive(StringName("%s_active" % key), "float(%s)" % condition, inputs))
    _modulate(event, _derive(StringName("%s_gain" % key), gain, inputs), SfxParameterModulation.Target.GAIN)


## A one-shot fired at each `step` change of `value` the `direction` way: the combined one at the
## chunk `pitch` selects, latched as it starts, a single one at `tone`
func _one_shot(
        event:SfxEvent, definition:MmdSoundSourceDefinition, key:StringName, value:StringName,
        direction:SfxParameterTrigger.Direction, pitch:String, inputs:Array[StringName], tone:String,
        exclusive:bool) -> void:
    var trigger := SfxParameterTrigger.new()
    trigger.parameter = value
    trigger.mode = SfxParameterTrigger.STEP
    trigger.direction = direction
    trigger.exclusive = exclusive
    _add_trigger(event, trigger)
    _tone(event, definition, key, pitch if definition.chunks else tone, inputs)


## A sound's pitch: a combined sound's chunk point, a single sample's playback pitch
## (sound.cpp:478 compute_combined_point())
func _tone(event:SfxEvent, definition:MmdSoundSourceDefinition, key:StringName, frequency:String, inputs:Array[StringName]) -> void:
    if definition.chunks:
        _derive(StringName("%s_point" % key), _point(frequency), inputs)
        return
    _modulate(event, _derive(StringName("%s_pitch" % key), frequency, inputs), SfxParameterModulation.Target.PITCH)


## The event of a formula-driven sound: a loop, or a one-shot whose combined chunks are picked once as
## it starts; its point and soundproofing by `key`
func _event(definition:MmdSoundSourceDefinition, event_name:StringName, key:StringName, one_shot:bool) -> SfxEvent:
    var event:SfxEvent = MmdSoundEventBuilder.build(
            definition, event_name, StringName("%s_point" % key) if definition.chunks else &"", not one_shot)
    for automation:SfxAutomation in event.automations:
        automation.latch_on_start = one_shot
    _proof(event, definition, key)
    return event


## The event's soundproofing, by where it sits - MaszynaVehicleSoundServer writes it as the listener
## moves; a general sound has none
func _proof(event:SfxEvent, definition:MmdSoundSourceDefinition, key:StringName) -> void:
    if not PLACEMENTS.has(definition.placement):
        return
    var parameter:StringName = StringName("soundproofing_%s" % key)
    if not _declared.has(parameter):
        _declare(parameter, 1.0)
        proofing.append({
            "parameter": parameter, "placement": PLACEMENTS[definition.placement], "profile": definition.soundproofing,
        })
    _modulate(event, parameter, SfxParameterModulation.Target.GAIN)
    _modulate(event, parameter, SfxParameterModulation.Target.UNIT_SIZE)


## A gate opening the event at the parameter's 1 and closing it at 0. Whatever it finds on when it
## is heard - again, or for the first time - resumes past its opening bookend: a sound activated out
## of range drops its opening (sound.cpp:364-371)
func _gate(event:SfxEvent, active:StringName) -> void:
    var trigger := SfxParameterTrigger.new()
    trigger.parameter = active
    trigger.mode = SfxParameterTrigger.GATE
    trigger.on_threshold = GATE_THRESHOLD
    trigger.off_threshold = GATE_THRESHOLD
    for clip:SfxClip in event.clips:
        if clip.bookend and not clip.trigger_mode == SfxClip.TriggerMode.TRIGGER_SUSTAIN:
            trigger.resume_offset = clip.length
    _add_trigger(event, trigger)


## gnd-sfx takes a resource's array only assigned, never changed in place
func _modulate(event:SfxEvent, parameter:StringName, target:SfxParameterModulation.Target) -> void:
    var modulations:Array[SfxParameterModulation] = event.parameter_modulations
    modulations.append(MmdSoundEventBuilder.build_modulation(parameter, target))
    event.parameter_modulations = modulations


func _add_trigger(event:SfxEvent, trigger:SfxParameterTrigger) -> void:
    var triggers:Array[SfxParameterTrigger] = event.triggers
    triggers.append(trigger)
    event.triggers = triggers


## A raw quantity the bank reads, declared once
func _quantity(quantity:MaszynaVehicleSoundServer.Quantity) -> StringName:
    var parameter:StringName = MaszynaVehicleSoundServer.quantity_get_name(quantity)
    _declare(parameter)
    return parameter


func _declare(name:StringName, default_value:float = 0.0) -> void:
    if _declared.has(name):
        return
    _declared[name] = true
    var parameter := SfxParameter.new()
    parameter.name = name
    parameter.min_value = -INF
    parameter.max_value = INF
    parameter.default_value = default_value
    parameters.append(parameter)


## A parameter computed from `inputs`, declared once (two sounds of a bank share the cab's rates)
func _derive(
        name:StringName, expression:String, inputs:Array[StringName],
        filter:SfxDerivedParameter.Filter = SfxDerivedParameter.NONE, time_constant:float = 0.0) -> StringName:
    if _declared.has(name):
        return name
    _declared[name] = true
    var parameter := SfxDerivedParameter.new()
    parameter.name = name
    parameter.expression = expression
    parameter.inputs = PackedStringArray(inputs)
    parameter.filter = filter
    parameter.time_constant = time_constant
    derived_parameters.append(parameter)
    return name


## A parameter easing toward `target` by `share` of the way per frame, as the original's filters do,
## from 0 - and at the target once within EASED_TOLERANCE of it, so it settles
func _ease(name:StringName, target:String, inputs:Array[StringName], share:float) -> StringName:
    return _derive(name, _eased(target, share), inputs)


## `previous` eased toward `target` by `share` of the way per frame, settling at it
func _eased(target:String, share:float) -> String:
    return _pick("abs((%s) - previous) < %s" % [target, _n(EASED_TOLERANCE)], target,
            "lerp(previous, (%s), %s)" % [target, _easing(share)])


## Original engine: the brakes are heard pressing above 10 N and 0.05 km/h (DynObj.cpp:4697)
func _braking_condition(speed:StringName) -> String:
    return "%s > 10.0 and %s > 0.05" % [_quantity(MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE), speed]


## `then_value` where `condition` holds, else `else_value`: Godot's Expression takes `a if c else b`
## only as a whole expression, never inside parentheses or a call
func _pick(condition:String, then_value:String, else_value:String) -> String:
    return "[(%s), (%s)][int(%s)]" % [else_value, then_value, condition]


## The track's quality heard: from 0.8 to 1.2 of the volume over the .scn's quality 0-20
func _quality_volume(quality:StringName) -> String:
    return "lerp(0.8, 1.2, clamp(%s / 20.0, 0.0, 1.0))" % quality


## sound.cpp:478 - values up to 1.0 are a 0-1 fraction of the 0-100 chunk range
func _point(frequency:String) -> String:
    return "%s * 100.0" % _pick("%s <= 1.0" % frequency, "clamp(%s, 0.0, 0.99)" % frequency, "max(0.0, %s)" % frequency)


## The share eased toward a goal per delta, for the original's share per frame
func _easing(share:float) -> String:
    return "(1.0 - exp(-delta / %s))" % _n(time_constant(share))


## The time constant [s] of easing by `share` per frame at REFERENCE_FRAME_RATE
static func time_constant(share:float) -> float:
    return -1.0 / (REFERENCE_FRAME_RATE * log(1.0 - share))


static func _n(value:float) -> String:
    return "(%s)" % var_to_str(value)
