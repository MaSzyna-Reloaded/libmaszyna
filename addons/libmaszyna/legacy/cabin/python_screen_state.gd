extends RefCounted
class_name PythonScreenState

## The dictionary a Python cab screen is drawn from - the original's TTrain::GetTrainState()
## (Train.cpp:696-940), under its own key names and value types.
##
## A script reads its keys with state['key'] and a missing one raises KeyError, which stops the
## whole screen from drawing. So every key the original hands over is always present: the ones the
## vehicle's state has an equivalent for come from it, the rest carry the neutral value of their
## type (see TODO.md for what is not ported yet).

## Train.cpp:924 fEIMParams[9][10] - row 0 is the train, rows 1-8 the powered cars
const EIM_CAR_COUNT:int = 8
## Train.cpp:921 fPress[20][7], and the 20 cars of the door/name/slip tables
const CAR_COUNT:int = 20
## Train.cpp:637 ggUniversals
const UNIVERSAL_COUNT:int = 30
## Train.cpp:809-813
const EIM_TRAIN_FIELDS:Array[String] = ["fd", "fdt", "fdb", "pd", "pdt", "pdb", "itothv", "1", "2", "3"]
const EIM_CAR_FIELDS:Array[String] = ["fr", "frt", "frb", "pr", "prt", "prb", "im", "vm", "ihv", "uhv"]
const DIESEL_FIELDS:Array[String] = [
    "enrot", "nrot", "fill_des", "fill_real", "clutch_des", "clutch_real", "water_temp", "oil_press",
    "engine_temp", "retarder_fill"]
const PRESSURE_FIELDS:Array[String] = ["bc", "bp", "sp", "cp", "rp", "mass", "spring"]

## Original key -> key of the occupied vehicle's state holding the same Mover field
## (Train.cpp:711-805; every pair checked against the getter behind the state key)
const STATE_KEYS:Dictionary[String, String] = {
    "cabactive": "cabin",                           # CabActive
    "battery": "power24_available",                 # Power24vIsAvailable
    "converter": "power110_available",              # Power110vIsAvailable
    "direction": "direction",                       # DirActive
    "speedctrl": "speed_control/selected_velocity", # SpeedCtrlValue
    "speedctrlpower": "speed_control/desired_power", # SpeedCtrlUnit.DesiredPower
    "speedctrlactive": "speed_control/active",      # SpeedCtrlUnit.IsActive
    "speedctrlstandby": "speed_control/standby",    # SpeedCtrlUnit.Standby
    "new_speed": "speed_control/set_velocity",      # NewSpeed
    "brake_delay_flag": "brake_delay_setting",      # BrakeDelayFlag
    "brake_op_mode_flag": "brake_operation_mode",   # BrakeOpModeFlag
    "emergency_brake": "alarm_chain_pulled",        # AlarmChainFlag
    "ca": "vigilance_blinking",                     # SecuritySystem.is_vigilance_blinking()
    "shp": "cabsignal_blinking",                    # SecuritySystem.is_cabsignal_blinking()
    "radio": "radio_enabled",                       # Radio
    "radio_channel": "radio_channel",
    "radio_volume": "radio_volume",                 # TTrain::m_radiovolume
    "distance_counter": "distance_counter",         # TTrain::m_distancecounter
    "pipelock": "main_pipe_locked",                 # LockPipe
    "door_lock": "doors_lock_enabled",              # Doors.lock_enabled
    "door_step": "doors_step_enabled",              # Doors.step_enabled
    "door_permit_left": "doors_left_open_permit",   # Doors.instances[left].open_permit
    "door_permit_right": "doors_right_open_permit", # Doors.instances[right].open_permit
    "slipping_wheels": "slipping_wheels",           # SlippingWheels
    "sanding": "sand_active",                       # SandDose
    "odometer": "total_distance",                   # DistCounter
    "epfuse": "dcemued/ep_fuse",                    # EpFuse
}
## Original key -> key of the controlled vehicle's state (mvControlled - the powered car of a
## multiple unit, TDynamicObject::FindPowered(), DynObj.cpp:7772)
const CONTROLLED_STATE_KEYS:Dictionary[String, String] = {
    "linebreaker": "main_switch_enabled",           # Mains
    "converter_overload": "converter_overload",     # ConvOvldFlag
    "compress": "compressor_enabled",               # CompressorFlag
    "mainctrl_pos": "controller_main_position",     # MainCtrlPos
    "main_ctrl_actual_pos": "controller_main_actual_position", # MainCtrlActualPos
    "scndctrl_pos": "controller_second_position",   # ScndCtrlPos
    "scnd_ctrl_actual_pos": "controller_second_actual_position", # ScndCtrlActualPos
    "brakectrl_pos": "brake_controller_position",   # fBrakeCtrlPos
    "localbrake_pos": "brake_local_position_normalized", # LocalBrakePosA
    "fuse": "fuse_active",                          # FuseFlag
}
## TMoverParameters::light bits (MOVER.h:188) of the lamps the state carries
const LIGHT_BITS:Dictionary[String, int] = {
    "headlight_left_enabled": 1 << 0,
    "redmarker_left_enabled": 1 << 1,
    "headlight_upper_enabled": 1 << 2,
    "headlight_right_enabled": 1 << 4,
    "redmarker_right_enabled": 1 << 5,
}
## Train.cpp:765 dir_brake / :772 indir_brake - a control pressure that counts as braking
const BRAKE_PRESSURE_THRESHOLD:float = 0.2
## Train.cpp:762 dir_brake - the share of electrodynamic braking (fEIMParams[0][5]) that counts
const ED_BRAKE_SHARE_THRESHOLD:float = 0.01
## Train.cpp:8617 - the engines whose own voltage is the high voltage fHVoltage shows
const ENGINE_VOLTAGE_TYPES:Array[RailVehicleEngine.EngineType] = [
    RailVehicleEngine.DIESEL_ELECTRIC, RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR]
## Train.cpp:8699 - a compressor that turns
const COMPRESSOR_SPEED_THRESHOLD:float = 0.00001
## A pantograph carrier publishes its collector (EnginePowerSource.SourceType == CurrentCollector)
const COLLECTOR_KEY:String = "current_collector/pantograph_first_active"

static var _neutral_state:Dictionary = {}


## `parameters` are the screen's own `parameters:` from the MMD (Train.cpp:706)
static func compose(vehicle:RID, parameters:Dictionary) -> Dictionary:
    var result:Dictionary = _neutral_state.duplicate()
    result.merge(parameters, true)
    if not vehicle.is_valid():
        return result
    var state:Dictionary = CabinSystem.vehicle_state(vehicle)
    var config:Dictionary = CabinSystem.vehicle_config(vehicle)

    # mvControlled (FindPowered(), DynObj.cpp:7772) and mvPantographUnit (FindPantographCarrier(),
    # DynObj.cpp:7798) - the controlled vehicle's own collector when nothing carries one
    var controlled_vehicle:RID = RailVehicleServer.vehicle_find_powered(vehicle)
    var controlled:Dictionary = VehicleServer.vehicle_dump_state(controlled_vehicle)
    var controlled_config:Dictionary = VehicleServer.vehicle_dump_config(controlled_vehicle)
    var carrier:RID = RailVehicleServer.vehicle_find_pantograph_carrier(vehicle)
    var pantograph_unit:Dictionary = VehicleServer.vehicle_dump_state(carrier) if carrier.is_valid() else controlled

    result["name"] = VehicleServer.vehicle_get_name(vehicle)   # DynamicObject->asName
    for key:String in STATE_KEYS:
        if STATE_KEYS[key] in state:
            result[key] = state[STATE_KEYS[key]]
    for key:String in CONTROLLED_STATE_KEYS:
        if CONTROLLED_STATE_KEYS[key] in controlled:
            result[key] = controlled[CONTROLLED_STATE_KEYS[key]]
    result["master"] = state.get("cabin_controleable", false)
    var driver_cabin:RID = RailVehicleServer.vehicle_get_driver_cabin(vehicle)
    var cabin_kind:RailVehicleCabinKind.Kind = RailVehicleServer.cabin_get_kind(driver_cabin)
    # Train.cpp:8684 - CabOccupied: 1 the front cab, -1 the rear one, 0 the machine room or nobody driving
    result["cab"] = (1 if cabin_kind == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT
            else -1 if cabin_kind == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR else 0)
    # Train.cpp:783-790 - the cab's generic toggles, with universal3 standing for the instrument light
    if driver_cabin.is_valid():
        var cab_state:CabinState = CabinSystem.get_cabin_state(driver_cabin)
        for index:int in UNIVERSAL_COUNT:
            result["universal%d" % index] = bool(cab_state.get_value(StringName("universal%d" % index), false))
    result["universal3"] = CabinSystem.cabin_get_instrument_light_enabled(driver_cabin)   # InstrumentLightActive
    result["mainctrl_pos_count"] = config.get("main_controller_position_max", 0)   # MainCtrlPosNo
    result["velocity"] = absf(state.get("speed", 0.0))   # abs(Vel), km/h
    result["manual_brake"] = state.get("brake_manual_position", 0) > 0
    result["tractionforce"] = absf(state.get("tractive_force", 0.0))   # abs(mvOccupied->Ft)
    result["voltage"] = absf(controlled.get("engine_voltage", 0.0))   # abs(EngineVoltage)
    result["im"] = absf(controlled.get("motor_current", 0.0))   # abs(Im)
    # Train.cpp:717-718 and fHVoltage (Train.cpp:8617-8626)
    var main_countdown:float = controlled.get("main_switch_time", 0.0)   # MainsInitTimeCountdown
    result["main_init"] = main_countdown < controlled_config.get("main_init_time", 0.0) and main_countdown > 0.0
    var high_voltage:float = controlled.get("engine_voltage", 0.0)
    if not controlled.get("engine_type", RailVehicleEngine.NONE) in ENGINE_VOLTAGE_TYPES:
        high_voltage = maxf(controlled.get("current_collector/voltage", 0.0),
                controlled.get("current_collector/trainset_high_voltage", 0.0))
    result["main_ready"] = not controlled.get("main_switch_enabled", false) and high_voltage > 0.0 \
            and main_countdown <= 0.0
    # the train row (Train.cpp:8778-8787): the occupied vehicle's power, the controlled one's force
    var power_share:float = state.get("eimic_real", 0.0)
    result["eimp_t_pd"] = power_share
    result["eimp_t_pdt"] = maxf(power_share, 0.0)
    result["eimp_t_pdb"] = -minf(power_share, 0.0)
    result["eimp_t_fdt"] = result["eimp_t_pdt"] * controlled.get("force_full", 0.0)
    result["eimp_t_fdb"] = result["eimp_t_pdb"] * controlled.get("force_full", 0.0)
    result["eimp_t_fd"] = result["eimp_t_fdt"] - result["eimp_t_fdb"]
    result["dir_brake"] = controlled.get("brake_control_pressure", 0.0) > BRAKE_PRESSURE_THRESHOLD \
            or result["eimp_t_pdb"] > ED_BRAKE_SHARE_THRESHOLD
    # GetEDBCP() is 0 for every brake but TLSt and TEStED, which is the original's typeid test
    result["indir_brake"] = state.get("brake_edb_cylinder_pressure", 0.0) > BRAKE_PRESSURE_THRESHOLD
    result["pantpress"] = absf(pantograph_unit.get("current_collector/pantograph_tank_pressure", 0.0))
    result["pant_compressor"] = pantograph_unit.get("current_collector/pantograph_compressor_enabled", false)
    result["traction_voltage"] = absf(pantograph_unit.get("current_collector/voltage", 0.0))
    # mvOccupied->EnergyMeter (Train.cpp:810-811)
    result["power_drawn"] = state.get("power_drawn", 0.0)
    result["power_returned"] = state.get("power_returned", 0.0)
    result["lights_front"] = _light_bits(state, "front")
    result["lights_rear"] = _light_bits(state, "rear")

    # TTrain::Update(), Train.cpp:8644-8768 - the cars under control, from the end the occupied
    # cab faces (GetFirstDynamic(CabOccupied < 0 ? rear : front, control))
    var cab_end:RailVehicleController.CouplerEnd = (RailVehicleController.COUPLER_END_REAR
            if cabin_kind == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR else RailVehicleController.COUPLER_END_FRONT)
    # Train.cpp:727-745 - the lamps at the outer ends of the train, its front the way the cab faces
    var trainset:Array = RailVehicleServer.vehicle_get_coupled(vehicle, cab_end, RailVehicleController.COUPLING_FLAG_COUPLER)
    result["lights_train_front"] = _outer_light_bits(trainset.front())
    result["lights_train_rear"] = _outer_light_bits(trainset.back())
    var cars:Array = RailVehicleServer.vehicle_get_coupled(vehicle, cab_end, RailVehicleController.COUPLING_FLAG_CONTROL)
    var powered:int = 0
    var induction_cars:int = 0
    var unit_number:int = 1
    var compressors:int = 0
    for index:int in mini(cars.size(), CAR_COUNT):
        var car:RID = cars[index]
        var car_state:Dictionary = VehicleServer.vehicle_dump_state(car)
        var car_number:int = index + 1
        result["eimp_pn%d_bc" % car_number] = car_state.get("brake_air_pressure", 0.0)   # BrakePress
        result["eimp_pn%d_bp" % car_number] = car_state.get("pipe_pressure", 0.0)   # PipePress
        result["eimp_pn%d_sp" % car_number] = car_state.get("feed_pipe_pressure", 0.0)   # ScndPipePress
        result["eimp_pn%d_spring" % car_number] = car_state.get("spring_brake/cylinder_pressure", 0.0)   # SpringBrake.SBP
        result["eimp_pn%d_cp" % car_number] = car_state.get("brake_control_pipe_pressure", 0.0)   # CntrlPipePress
        result["eimp_pn%d_rp" % car_number] = car_state.get("brake_reservoir_pressure", 0.0)   # GetBRP()
        # (TotalMass - Mred) * 0.001
        result["eimp_pn%d_mass" % car_number] = (car_state.get("mass_total", 0.0) - car_state.get("mass_reduced", 0.0)) / LibMaszynaUnits.KILOGRAMS_PER_TONNE  # Train.cpp:8679 - in tonnes
        result["brakes_%d_spring_active" % car_number] = car_state.get("spring_brake/braking", false)   # IsActive
        result["brakes_%d_spring_shutoff" % car_number] = car_state.get("spring_brake/shut_off", false)   # ShuttOff
        var doors_left:bool = car_state.get("doors_left_position", 0.0) > 0.0
        var doors_right:bool = car_state.get("doors_right_position", 0.0) > 0.0
        result["doors_%d" % car_number] = doors_left or doors_right
        result["doors_l_%d" % car_number] = doors_left
        result["doors_r_%d" % car_number] = doors_right
        result["doorstep_l_%d" % car_number] = car_state.get("doors_left_step_position", 0.0) > 0.0
        result["doorstep_r_%d" % car_number] = car_state.get("doors_right_step_position", 0.0) > 0.0
        result["car_name%d" % car_number] = VehicleServer.vehicle_get_name(car)
        # the unit and the last letter of the vehicle's type (Train.cpp:895, 8687-8688)
        result["code_%d" % car_number] = "%d%s" % [unit_number, RailVehicleServer.vehicle_get_type_name(car).right(1)]
        result["slip_%d" % car_number] = car_state.get("slipping_wheels", false)
        if unit_number <= EIM_CAR_COUNT:
            if COLLECTOR_KEY in car_state:
                result["eimp_u%d_pf" % unit_number] = result["eimp_u%d_pf" % unit_number] or car_state.get("current_collector/pantograph_first_active", false)
                result["eimp_u%d_pr" % unit_number] = result["eimp_u%d_pr" % unit_number] or car_state.get("current_collector/pantograph_second_active", false)
            # CompressorStart is never automatic here - the wrapper does not set it
            result["eimp_u%d_comp_a" % unit_number] = result["eimp_u%d_comp_a" % unit_number] or car_state.get("compressor_allowed", false)
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(car, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        if brake and brake.compressor_speed > COMPRESSOR_SPEED_THRESHOLD:
            if unit_number <= EIM_CAR_COUNT:
                result["eimp_u%d_comp_w" % unit_number] = result["eimp_u%d_comp_w" % unit_number] or car_state.get("compressor_enabled", false)
            compressors += 1
            result["compressors_%d_allow" % compressors] = car_state.get("compressor_allowed", false)
            result["compressors_%d_work" % compressors] = car_state.get("compressor_enabled", false)
            result["compressors_%d_car_no" % compressors] = index
        var engine_type:int = car_state.get("engine_type", RailVehicleEngine.NONE)
        # eimc[eimc_p_Pmax] > 1 - an induction motor car; the diesels by their engine type
        if powered < EIM_CAR_COUNT and engine_type in [RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR, RailVehicleEngine.DIESEL, RailVehicleEngine.DIESEL_ELECTRIC]:
            var powered_number:int = powered + 1
            if not engine_type == RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
                result["diesel_param_%d_enrot" % powered_number] = car_state.get("engine_rpm_count", 0.0) * LibMaszynaUnits.SECONDS_PER_MINUTE   # enrot * 60
                result["diesel_param_%d_nrot" % powered_number] = car_state.get("wheel_rotation_speed_rps", 0.0)   # nrot
                result["diesel_param_%d_fill_real" % powered_number] = car_state.get("diesel_fill", 0.0)   # dizel_fill
                result["diesel_param_%d_oil_press" % powered_number] = car_state.get("oil_pump_pressure", 0.0)   # OilPump.pressure
                result["diesel_param_%d_fill_des" % powered_number] = car_state.get("diesel_fill_desired", 0.0)   # RList[MainCtrlPos].R
                result["diesel_param_%d_clutch_des" % powered_number] = car_state.get("diesel_clutch_desired", 0.0)   # RList[MainCtrlPos].Mn
                result["diesel_param_%d_clutch_real" % powered_number] = car_state.get("diesel_clutch_engagement", 0.0)   # dizel_engage
                result["diesel_param_%d_water_temp" % powered_number] = car_state.get("diesel_water_temperature", 0.0)   # dizel_heat.Twy
                result["diesel_param_%d_engine_temp" % powered_number] = car_state.get("diesel_engine_temperature", 0.0)   # dizel_heat.Ts
                result["diesel_param_%d_retarder_fill" % powered_number] = car_state.get("diesel_retarder_fill", 0.0)   # hydro_R_Fill
            else:
                # Train.cpp:8712-8723 - an induction motor car's forces, currents and voltages
                var force:float = car_state.get("force_max", 0.0)   # eimv[eimv_Fmax]
                var force_share:float = force / maxf(car_state.get("force_full", 0.0), 1.0)
                var total_current:float = car_state.get("total_current", 0.0)   # Itot
                result["eimp_c%d_fr" % powered_number] = force
                result["eimp_c%d_frt" % powered_number] = maxf(force, 0.0)
                result["eimp_c%d_frb" % powered_number] = -minf(force, 0.0)
                result["eimp_c%d_pr" % powered_number] = force_share
                result["eimp_c%d_prt" % powered_number] = maxf(force_share, 0.0)
                result["eimp_c%d_prb" % powered_number] = -minf(force_share, 0.0)
                result["eimp_c%d_im" % powered_number] = car_state.get("field_current", 0.0)   # eimv[eimv_If]
                result["eimp_c%d_vm" % powered_number] = car_state.get("motor_voltage", 0.0)   # eimv[eimv_U]
                result["eimp_c%d_ihv" % powered_number] = total_current
                result["eimp_c%d_uhv" % powered_number] = car_state.get("engine_voltage", 0.0)   # EngineVoltage
                result["eimp_t_itothv"] += total_current
            result["eimp_c%d_ms" % powered_number] = car_state.get("main_switch_enabled", false)   # Mains
            result["eimp_c%d_cv" % powered_number] = car_state.get("battery_voltage", 0.0)   # BatteryVoltage
            result["eimp_c%d_fuse" % powered_number] = car_state.get("fuse_active", false)   # FuseFlag
            result["eimp_c%d_batt" % powered_number] = car_state.get("battery_enabled", false)   # Battery
            result["eimp_c%d_conv" % powered_number] = car_state.get("converter_enabled", false)   # ConverterFlag
            result["eimp_c%d_heat" % powered_number] = car_state.get("heating_enabled", false)   # Heating
            powered = powered_number
        # Train.cpp:856-870 - the inverters of each induction motor car (eimc[eimc_p_Pmax] > 1),
        # numbered apart from the diesels
        if induction_cars < EIM_CAR_COUNT and engine_type == RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
            induction_cars += 1
            var inverters:Array = car_state.get("inverters", [])
            result["eimp_c%d_invno" % induction_cars] = inverters.size()   # InvertersNo
            for inverter_index:int in inverters.size():
                var inverter:RailVehicleInverter = inverters[inverter_index]
                var prefix:String = "eimp_c%d_inv%d_" % [induction_cars, inverter_index + 1]
                result[prefix + "act"] = inverter.active   # IsActive
                result[prefix + "error"] = inverter.error   # Error
                result[prefix + "allow"] = inverter.allow   # Activate
        # a control coupling that is not a permanent one ends a unit (Train.cpp:8757)
        if index + 1 < cars.size() and not cars[index + 1] in RailVehicleServer.vehicle_get_coupled(
                car, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_PERMANENT):
            unit_number += 1
    result["car_no"] = mini(cars.size(), CAR_COUNT)
    result["power_no"] = powered
    result["unit_no"] = unit_number
    result["compressors_no"] = compressors

    # world state data (Train.cpp:928-934)
    var seconds:int = int(SimulationServer.time_of_day * LibMaszynaUnits.SECONDS_PER_HOUR)
    result["hours"] = seconds / LibMaszynaUnits.SECONDS_PER_HOUR
    result["minutes"] = seconds / LibMaszynaUnits.SECONDS_PER_MINUTE % LibMaszynaUnits.MINUTES_PER_HOUR
    result["seconds"] = seconds % LibMaszynaUnits.SECONDS_PER_MINUTE
    result["air_temperature"] = SimulationServer.air_temperature
    result["light_level"] = SimulationServer.light_level
    return result


## The TMoverParameters::light bits of the lamps lit at one end ("front"/"rear") of a vehicle
static func _light_bits(state:Dictionary, end:String) -> int:
    var bits:int = 0
    for lamp:String in LIGHT_BITS:
        if state.get("lights/%s_%s" % [end, lamp], false):
            bits |= LIGHT_BITS[lamp]
    return bits


## The lamps at the end of an outermost vehicle of the train that has nothing coupled to it
## (iLights[] of the train's end vehicle, by its own direction, Train.cpp:735-740)
static func _outer_light_bits(end_vehicle:RID) -> int:
    var beyond_front:Array = RailVehicleServer.vehicle_get_coupled(
            end_vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)
    var outer:String = "front" if beyond_front.front() == end_vehicle else "rear"
    return _light_bits(VehicleServer.vehicle_dump_state(end_vehicle), outer)


static func _static_init() -> void:
    var state:Dictionary = {
        "name": "", "cab": 0, "cabactive": 0, "master": false,
        "battery": false, "linebreaker": false, "main_init": false, "main_ready": false,
        "converter": false, "converter_overload": false, "compress": false, "pant_compressor": false,
        "lights_front": 0, "lights_rear": 0, "off_from_dimmer": false, "lights_compartments": false,
        "lights_train_front": 0, "lights_train_rear": 0,
        "direction": 0, "mainctrl_pos": 0, "mainctrl_pos_count": 0, "main_ctrl_actual_pos": 0,
        "scndctrl_pos": 0, "scnd_ctrl_actual_pos": 0, "brakectrl_pos": 0.0, "localbrake_pos": 0.0,
        "new_speed": 0.0, "speedctrl": 0.0, "speedctrlpower": 0.0, "speedctrlactive": false,
        "speedctrlstandby": false,
        "manual_brake": false, "dir_brake": false, "indir_brake": false, "emergency_brake": false,
        "brake_delay_flag": 0, "brake_op_mode_flag": 0, "pipelock": false,
        "ca": false, "shp": false, "distance_counter": 0.0, "pantpress": 0.0,
        "radio": false, "radio_channel": 0, "radio_volume": 0.0,
        "door_lock": false, "door_step": false, "door_permit_left": false, "door_permit_right": false,
        "velocity": 0.0, "tractionforce": 0.0, "slipping_wheels": false, "sanding": false,
        "odometer": 0.0,
        "traction_voltage": 0.0, "voltage": 0.0, "im": 0.0, "fuse": false, "epfuse": false,
        "power_drawn": 0.0, "power_returned": 0.0,
        "compressors_no": 0, "car_no": 0, "power_no": 0, "unit_no": 0,
        "velocity_desired": 0.0, "velroad": 0.0, "vellimitlast": 0.0, "velsignallast": 0.0,
        "velsignalnext": 0.0, "velnext": 0.0, "actualproximitydist": 0.0,
        # TTrainParameters::serialize() (mtable.cpp:641) of a driver without a timetable -
        # TTrainParameters("none"), Driver.cpp:1907
        "trainnumber": "none", "traincategory": "", "trainname": "", "train_brakingmassratio": 0.0,
        "train_enginetype": "", "train_engineload": 0.0, "train_stationfrom": "",
        "train_stationto": "", "train_stationindex": 0, "train_stationcount": 0,
        "train_stationstart": 0, "train_atpassengerstop": false, "train_length": 0.0,
        "scenario": "", "hours": 0, "minutes": 0, "seconds": 0, "air_temperature": 0.0,
        "light_level": 0.0,
        # update_screens() adds the touches of the screen (Train.cpp:10302)
        "touches": [],
    }
    for index:int in UNIVERSAL_COUNT:
        state["universal%d" % index] = false
    for field:String in EIM_TRAIN_FIELDS:
        state["eimp_t_" + field] = 0.0
    for car:int in range(1, EIM_CAR_COUNT + 1):
        for field:String in EIM_CAR_FIELDS:
            state["eimp_c%d_%s" % [car, field]] = 0.0
        for field:String in DIESEL_FIELDS:
            state["diesel_param_%d_%s" % [car, field]] = 0.0
        state["eimp_c%d_cv" % car] = 0.0
        for field:String in ["ms", "fuse", "batt", "conv", "heat"]:
            state["eimp_c%d_%s" % [car, field]] = false
        for field:String in ["pf", "pr", "comp_a", "comp_w"]:
            state["eimp_u%d_%s" % [car, field]] = false
    for car:int in range(1, CAR_COUNT + 1):
        for field:String in PRESSURE_FIELDS:
            state["eimp_pn%d_%s" % [car, field]] = 0.0
        state["brakes_%d_spring_active" % car] = false
        state["brakes_%d_spring_shutoff" % car] = false
        for field:String in ["doors_%d", "doors_l_%d", "doors_r_%d", "doorstep_l_%d", "doorstep_r_%d", "slip_%d"]:
            state[field % car] = false
        state["doors_no_%d" % car] = 0
        state["code_%d" % car] = ""
        state["car_name%d" % car] = ""
    _neutral_state = state
