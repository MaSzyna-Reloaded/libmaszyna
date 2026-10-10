extends RefCounted
class_name MmdSoundCatalog

## MMD `sounds:` label -> gnd-sfx event name + TrainSoundTrigger wiring, evidence-based the same
## way MmdSemanticCatalog is: state_property names are copied from demo/vehicles/sm42/sm_42.tscn's
## already-working hand-authored TrainSoundTrigger wiring (oil_pump_active/engine_rpm) or from the
## C++ VehicleComponent state each other label's own property is confirmed to expose (fuel_pump_active -
## RailVehicleDieselEngine.cpp:180, battery_enabled - VehicleController.cpp:428, compressor_enabled -
## RailVehicleEngine.cpp:197/RailVehicleElectricEngine.cpp:169, horn_low_active/horn_high_active/whistle_active
## - RailVehicleHorns.cpp). Any MMD sound label not listed here is parsed (so the token stream stays
## aligned) but produces no bank event and no trigger - same "nothing built rather than something
## wrong" discipline as MmdSemanticCatalog.
##
## v1 is Tier 1 (exact parity with the proven SM42 reference: oilpump/fuelpump/horn1/horn2/horn3/
## engine) + Tier 2 (same shapes, additional labels with confirmed wrapper state: battery/
## compressor) + Tier 3 (brake-related labels - `entry["controller"] == &"brake"` marks these;
## MmdSoundBankInstancer hands them to BrakeSoundModel instead of building a TrainSoundTrigger
## from `state_property`/`trigger_mode` the way Tier 1/2 entries do) + running sounds (`entry["controller"] == &"running"`, see RunningSoundModel). Every
## other label surveyed in dynamic/pkp/ (door family/announcements/...) is deliberately
## absent - each still needs its own wrapper-state cross-reference before it can be added, same
## discipline as the cabin catalog.

static var _catalog:Dictionary = {}
static var _built:bool = false


static func _ensure_built() -> void:
    if _built:
        return
    _built = true

    _catalog = {
        "oilpump": {
            "event_name": &"oil_pump",
            "state_property": "oil_pump_active",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        "fuelpump": {
            "event_name": &"fuel_pump",
            "state_property": "fuel_pump_active",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        "battery": {
            "event_name": &"battery",
            "state_property": "battery_enabled",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        # Coupler attach/detach one-shots per coupling type (DynObj.cpp:6409-6520, played at
        # DynObj.cpp:4855-4905). The vehicle reports each attach and each detach once
        # (VehicleController.coupler_attached / coupler_detached); the running counts these names
        # address live in TrainSoundSystem, which is what owns sound state.
        # A coupler adapter fitted and taken off (DynObj.cpp:6835-6852, played on sound::attachadapter/
        # removeadapter of TDynamicObject::attach/remove_coupler_adapter())
        "coupleradapterattach": {
            "event_name": &"coupler_adapter_attach",
            "state_property": "coupler_sound/attach_adapter",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "coupleradapterremove": {
            "event_name": &"coupler_adapter_remove",
            "state_property": "coupler_sound/remove_adapter",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "couplerattach": {
            "event_name": &"coupler_attach",
            "state_property": "coupler_sound/attach_coupler",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "brakehoseattach": {
            "event_name": &"brakehose_attach",
            "state_property": "coupler_sound/attach_brakehose",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "mainhoseattach": {
            "event_name": &"mainhose_attach",
            "state_property": "coupler_sound/attach_mainhose",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "controlattach": {
            "event_name": &"control_attach",
            "state_property": "coupler_sound/attach_control",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "gangwayattach": {
            "event_name": &"gangway_attach",
            "state_property": "coupler_sound/attach_gangway",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "heatingattach": {
            "event_name": &"heating_attach",
            "state_property": "coupler_sound/attach_heating",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "couplerdetach": {
            "event_name": &"coupler_detach",
            "state_property": "coupler_sound/detach_coupler",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "brakehosedetach": {
            "event_name": &"brakehose_detach",
            "state_property": "coupler_sound/detach_brakehose",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "mainhosedetach": {
            "event_name": &"mainhose_detach",
            "state_property": "coupler_sound/detach_mainhose",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "controldetach": {
            "event_name": &"control_detach",
            "state_property": "coupler_sound/detach_control",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "gangwaydetach": {
            "event_name": &"gangway_detach",
            "state_property": "coupler_sound/detach_gangway",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "heatingdetach": {
            "event_name": &"heating_detach",
            "state_property": "coupler_sound/detach_heating",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        # the departure signal loops on each door speaker while the vehicle's door warning sounds
        # (DynObj.cpp:4768-4787); range 25 m unless the MMD says (DynObj.cpp:6359) - one event at
        # every `doors:` location, none without them (m_doorspeakers, DynObj.cpp:6359-6364, 6634)
        "departuresignal": {
            "event_name": &"departure_signal",
            "state_property": "doors_departure_signal_sounding",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
            "range": 25.0,
            "at_door_speakers": true,
        },
        "compressor": {
            "event_name": &"compressor",
            "state_property": "compressor_enabled",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        # sConverter runs while ConverterFlag is set (DynObj.cpp:4433-4450)
        "converter": {
            "event_name": &"converter",
            "state_property": "converter_enabled",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        # sSmallCompressor runs while PantCompFlag is set (DynObj.cpp:4506)
        "small-compressor": {
            "event_name": &"small_compressor",
            "state_property": "current_collector/pantograph_compressor_enabled",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        # A pantograph touching the wire and dropping (DynObj.cpp:3881-3934, 4007-4036),
        # reported by RailVehicleEnginePowerSource.pantograph_up / pantograph_down and counted by
        # TrainSoundSystem like the coupler events
        "pantographup": {
            "event_name": &"pantograph_up",
            "state_property": "pantograph_sound/up",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        "pantographdown": {
            "event_name": &"pantograph_down",
            "state_property": "pantograph_sound/down",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CHANGE,
        },
        # horn1/horn2/horn3 map onto RailVehicleHorns' low/high/whistle bits, in that fixed order -
        # confirmed via the original engine's Train.cpp (OnCommand_hornlowactivate/
        # OnCommand_hornhighactivate/OnCommand_whistleactivate) and DynObj.cpp's per-frame
        # WarningSignal bit 1/2/4 -> sHorn1/sHorn2/sHorn3 dispatch, NOT by the sample names
        # vehicles happen to give the files (dynamic/pkp/sm42_v1's horn3 samples are literally
        # named "...-klakson-..." - a klaxon-style third horn tone, not a train whistle, but it's
        # still driven by the whistle_activate command/WarningSignal bit 4 in the engine).
        "horn1": {
            "event_name": &"horn1",
            "state_property": "horn_low_active",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        "horn2": {
            "event_name": &"horn2",
            "state_property": "horn_high_active",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        "horn3": {
            "event_name": &"horn3",
            "state_property": "whistle_active",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        # buzzer:/buzzershp: (internaldata:, not sounds: - see MmdSoundSourceParser.
        # parse_internal_data()) drive a LOOPING sound while the alerter is actively unacknowledged
        # (Train.cpp:10111-10151: dsbBuzzer/dsbBuzzerShp play() while is_beeping()/
        # is_cabsignal_beeping(), stop() otherwise) - a SEPARATE, later-triggered stage from the
        # light's own on/off click (RailVehicleSecuritySystem::is_beeping(), Mover.cpp:186:
        # `alert_timer > SoundSignalDelay` - the buzzer only starts SoundSignalDelay seconds after
        # the light already began blinking, not simultaneously).
        "buzzer": {
            "event_name": &"buzzer",
            "state_property": "beeping",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        "buzzershp": {
            "event_name": &"buzzershp",
            "state_property": "cabsignal_beeping",
            "trigger_mode": TrainSoundTrigger.TriggerMode.TOGGLE,
        },
        # Hasler recorder ticking (Train.cpp:8323-8335, dsbHasler): chunks sound1..soundN are picked
        # by speed in km/h (pitch fTachoVelocity * 0.01, sound.cpp:477 compute_combined_point() * 100),
        # silent below the first chunk (sound.cpp:436). tachometer_clock_speed is 0 while the
        # fTachoCount hysteresis keeps the recorder stopped.
        "tachoclock": {
            "event_name": &"tachoclock",
            "state_property": "tachometer_clock_speed",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CONTINUOUS,
            "sound_parameter": &"speed",
            "trigger_threshold_min": 1.0,
            "trigger_threshold_max": 10000.0,
        },
        "engine": {
            "event_name": &"engine",
            "state_property": "engine_rpm",
            "trigger_mode": TrainSoundTrigger.TriggerMode.CONTINUOUS,
            "sound_parameter": &"rpm",
            # Matches sm_42.tscn's own Engine TrainSoundTrigger thresholds - a chunk-based
            # automation is only meaningful once the engine is actually turning, and RPM has no
            # natural upper bound worth clamping below in practice.
            "trigger_threshold_min": 10.0,
            "trigger_threshold_max": 10000.0,
        },
        # Brake sounds - one event per label, played by BrakeSoundModel with the original's own
        # per-frame logic (MmdSoundBankInstancer builds the events, TrainSoundSystem plays them).
        # The vehicle's (DynObj.cpp:4545-4760): shoes rsBrake, squeal rsPisk, cylinder release
        # rsUnbrake, emergency valve, wheel slip, releaser, accelerator, cylinder and EP clicks,
        # spring brake. The cab's (Train.cpp:8474-8641): local brake rsSBHiss/rsSBHissU, the
        # driver's brake valve rsHiss/rsHissU/rsHissE/rsHissX/rsHissT, and its own copy of the shoes.
        "brakesound": {"event_name": &"brake_shoe", "controller": &"brake"},
        "brake": {"event_name": &"brake_squeal", "controller": &"brake"},
        "unbrake": {"event_name": &"brake_release_hiss", "controller": &"brake"},
        "emergencybrake": {"event_name": &"emergency_brake_hiss", "controller": &"brake"},
        "slipperysound": {"event_name": &"wheel_slip_squeal", "controller": &"brake"},
        "releaser": {"event_name": &"brake_releaser", "controller": &"brake"},
        "brakeacc": {"event_name": &"brake_accelerator", "controller": &"brake"},
        "brakecylinderinc": {"event_name": &"brake_cylinder_increase", "controller": &"brake"},
        "brakecylinderdec": {"event_name": &"brake_cylinder_decrease", "controller": &"brake"},
        "epbrakeinc": {"event_name": &"ep_brake_increase", "controller": &"brake"},
        "epbrakedec": {"event_name": &"ep_brake_decrease", "controller": &"brake"},
        "springbrake": {"event_name": &"spring_brake_activate", "controller": &"brake"},
        "springbrakeoff": {"event_name": &"spring_brake_release", "controller": &"brake"},
        "localbrakesound": {"event_name": &"local_brake_release_hiss", "controller": &"brake"},
        "localbrakesound2": {"event_name": &"local_brake_engage_hiss", "controller": &"brake"},
        "airsound": {"event_name": &"brake_valve_braking_hiss", "controller": &"brake"},
        "airsound2": {"event_name": &"brake_valve_release_hiss", "controller": &"brake"},
        "airsound3": {"event_name": &"brake_valve_emergency_hiss", "controller": &"brake"},
        "airsound4": {"event_name": &"brake_valve_control_chamber_hiss", "controller": &"brake"},
        "airsound5": {"event_name": &"brake_valve_timing_reservoir_hiss", "controller": &"brake"},
        # Running sounds - gain/pitch computed each update by RunningSoundModel from the mover state
        # and the track under the vehicle, the same formulas as the original. Positional labels get
        # one event per location, suffixed with its index.
        # Traction motors, one per `tractionmotors:` location (DynObj.cpp:5710, 7933-8010)
        "tractionmotor": {"event_name": &"traction_motor", "controller": &"running"},
        # A diesel's turbocharger from the master controller position TurboPos: on (DynObj.cpp:6255-6259,
        # 8266-8290)
        "turbo": {"event_name": &"engine_turbo", "controller": &"running"},
        # Resistor ventilator (DynObj.cpp:5801, 8081-8092)
        "ventilator": {"event_name": &"ventilator", "controller": &"running"},
        # Curve squeal (DynObj.cpp:5916, 4735-4763)
        "curve": {"event_name": &"curve", "controller": &"running"},
        # Running noise, one per `bogies:` location (DynObj.cpp:6123, 4630-4720)
        "outernoise": {"event_name": &"outer_noise", "controller": &"running"},
        # Rail joint clatter, one-shot per axle (DynObj.cpp:5643, 3477-3545)
        "wheel_clatter": {"event_name": &"wheel_clatter", "controller": &"running"},
        # Cab running noise (Train.cpp:8272, update_sounds_runningnoise())
        "runningnoise": {"event_name": &"running_noise", "controller": &"running"},
    }


static func has_label(label:String) -> bool:
    _ensure_built()
    return _catalog.has(label)


static func get_entry(label:String) -> Dictionary:
    _ensure_built()
    return _catalog.get(label, {})
