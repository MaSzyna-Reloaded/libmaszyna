extends MaszynaGutTest

## LockPipeOn/Off and HandleUnlock absent (Mover.cpp:10505-10507)
const MAIN_PIPE_LOCK_NONE:float = -1.0
const MAIN_PIPE_UNLOCK_HANDLE_POSITION_NONE:float = -3.0

var vehicle: VehiclePhysicsNode
var train: VehicleController
var brake: RailVehicleBrake

func before_each():
    vehicle = RailVehiclePhysicsNode.new()
    vehicle.vehicle_id = "TestTrain"
    add_child(vehicle)
    train = VehicleServer.vehicle_get_controller(vehicle.get_vehicle_rid())

    brake = MoverRailVehicleBrake.new()
    train.add_component(brake)
    await wait_idle_frames(2)

func after_each():
    train = null
    brake = null
    remove_child(vehicle)
    vehicle.free()

# LoadFIZ_Cntrl with none of the brake keys (Mover.cpp:10720-10895, MOVER.h:1612-1630, 1696, 2045)
func test_defaults_match_original_mover():
    assert_eq(brake.cntrl_brake_system, RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL)
    assert_eq(brake.cntrl_brake_ctrl_position_count, 0)
    # a delay of 0 is taken from CheckLocomotiveParameters' table (Mover.cpp:12066-12073)
    assert_eq(brake.cntrl_brake_delay_1, 0.0)
    assert_eq(brake.cntrl_brake_delay_2, 0.0)
    assert_eq(brake.cntrl_brake_delay_3, 0.0)
    assert_eq(brake.cntrl_brake_delay_4, 0.0)
    assert_eq(brake.cntrl_brake_delays, RailVehicleBrake.BRAKE_DELAY_NONE)
    # none unless the FIZ says (MOVER.h:1580, Mover.cpp:10746) - with PS the handle works only from an occupied cab
    assert_eq(brake.cntrl_brake_op_modes, RailVehicleBrake.BRAKE_OP_MODE_NONE)
    assert_eq(brake.cntrl_brake_handle_type, RailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE)
    assert_eq(brake.cntrl_local_brake_handle_type, RailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE)
    assert_eq(brake.cntrl_anti_skid_brake_type, RailVehicleBrake.ANTI_SKID_BRAKE_NONE)
    assert_eq(brake.cntrl_local_brake_type, RailVehicleBrake.LOCAL_BRAKE_TYPE_NONE)
    assert_false(brake.cntrl_manual_brake_present)
    assert_true(brake.cntrl_spring_brake_cuts_off_drive)
    assert_eq(brake.brake_method, RailVehicleBrake.BRAKE_METHOD_NONE)
    # LPOn/LPOff/HandlePipeUnlockPos absent (Mover.cpp:10505-10507)
    assert_eq(brake.main_pipe_blocking_pressure, MAIN_PIPE_LOCK_NONE)
    assert_eq(brake.main_pipe_unblocking_pressure, MAIN_PIPE_LOCK_NONE)
    assert_eq(brake.main_pipe_minimum_unblocking_handle_position, MAIN_PIPE_UNLOCK_HANDLE_POSITION_NONE)


func test_round_trip_and_update_without_crashing():
    brake.cntrl_brake_system = RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC
    brake.cntrl_brake_ctrl_position_count = 8
    brake.cntrl_brake_delays = RailVehicleBrake.BRAKE_DELAY_GPR_MG
    brake.cntrl_brake_op_modes = RailVehicleBrake.BRAKE_OP_MODE_PN
    brake.cntrl_brake_handle_type = RailVehicleBrake.BRAKE_HANDLE_TYPE_KNORR
    brake.cntrl_local_brake_handle_type = RailVehicleBrake.BRAKE_HANDLE_TYPE_WESTINGHOUSE
    brake.cntrl_anti_skid_brake_type = RailVehicleBrake.ANTI_SKID_BRAKE_AUTOMATIC
    brake.cntrl_local_brake_type = RailVehicleBrake.LOCAL_BRAKE_TYPE_HYDRAULIC
    brake.cntrl_manual_brake_present = false
    brake.cntrl_dynamic_brake_type = RailVehicleBrake.DYNAMIC_BRAKE_AUTOMATIC
    brake.cntrl_local_brake_traxx = true
    brake.cntrl_release_parking_by_spring_brake = true
    brake.cntrl_release_parking_by_spring_brake_when_door_open = true
    brake.cntrl_spring_brake_cuts_off_drive = false
    brake.cntrl_spring_brake_drive_emergency_velocity = 5.0
    await wait_idle_frames(2)

    assert_eq(brake.cntrl_brake_system, RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC)
    assert_eq(brake.cntrl_brake_ctrl_position_count, 8)
    assert_eq(brake.cntrl_brake_delays, RailVehicleBrake.BRAKE_DELAY_GPR_MG)
    assert_eq(brake.cntrl_dynamic_brake_type, RailVehicleBrake.DYNAMIC_BRAKE_AUTOMATIC)
    assert_true(brake.cntrl_local_brake_traxx)
    assert_true(train.get_state().has("brake_air_pressure"), "RailVehicleBrake should keep functioning after configuring the Cntrl. section")


func test_state_reports_the_handle_flows_its_hiss_is_made_of() -> void:
    var state:Dictionary = VehicleServer.vehicle_dump_state(vehicle.get_vehicle_rid())
    for key:String in [
            "brake_handle_braking_flow", "brake_handle_release_flow", "brake_handle_emergency_flow",
            "brake_handle_control_chamber_flow", "brake_handle_timing_reservoir_flow"]:
        assert_true(state.has(key), key)
    # the sound's own filtering is the sound system's business (BrakeSoundModel)
    assert_false(state.has("brake_loco_pressure_fall_rate"))
    assert_true(brake.has_signal(&"accelerator_activated"))


## Train.cpp:1724, 1750, 1826 - a vehicle whose local brake is the hand wheel (EN57 ra,
## LocalBrake=ManualBrake) has no independent brake: its keys and handle move nothing
func test_a_manual_local_brake_takes_no_independent_brake_command() -> void:
    brake.cntrl_local_brake_type = RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL
    train.apply_configuration()
    var rid:RID = vehicle.get_vehicle_rid()

    VehicleServer.vehicle_send_command(rid, "local_brake_set", 0.5)
    VehicleServer.vehicle_send_command(rid, "local_brake_increase")

    assert_eq(VehicleServer.vehicle_dump_state(rid).get("brake_local_position_normalized", -1.0), 0.0)


func test_a_pneumatic_local_brake_takes_the_independent_brake_command() -> void:
    brake.cntrl_local_brake_type = RailVehicleBrake.LOCAL_BRAKE_TYPE_PNEUMATIC
    train.apply_configuration()
    var rid:RID = vehicle.get_vehicle_rid()

    VehicleServer.vehicle_send_command(rid, "local_brake_set", 0.5)

    assert_eq(VehicleServer.vehicle_dump_state(rid).get("brake_local_position_normalized", -1.0), 0.5)
