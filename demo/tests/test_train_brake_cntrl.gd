extends MaszynaGutTest

var train: TrainController
var brake: TrainBrake

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    brake = TrainBrake.new()
    train.add_child(brake)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults_match_previous_hardcoded_behavior():
    assert_eq(brake.get("cntrl/brake_system"), TrainBrake.BRAKE_SYSTEM_PNEUMATIC)
    assert_eq(brake.get("cntrl/brake_ctrl_position_count"), 6)
    assert_eq(brake.get("cntrl/brake_delay_1"), 15.0)
    assert_eq(brake.get("cntrl/brake_delay_2"), 3.0)
    assert_eq(brake.get("cntrl/brake_delay_3"), 36.0)
    assert_eq(brake.get("cntrl/brake_delay_4"), 22.0)
    assert_eq(brake.get("cntrl/brake_delays"), TrainBrake.BRAKE_DELAY_GP)
    assert_eq(brake.get("cntrl/brake_op_modes"), TrainBrake.BRAKE_OP_MODE_PNEPMED)
    assert_eq(brake.get("cntrl/brake_handle_type"), TrainBrake.BRAKE_HANDLE_TYPE_FV4A)
    assert_eq(brake.get("cntrl/local_brake_handle_type"), TrainBrake.BRAKE_HANDLE_TYPE_FD1)
    assert_eq(brake.get("cntrl/anti_skid_brake_type"), TrainBrake.ANTI_SKID_BRAKE_MANUAL)
    assert_eq(brake.get("cntrl/local_brake_type"), TrainBrake.LOCAL_BRAKE_TYPE_PNEUMATIC)
    assert_true(brake.get("cntrl/manual_brake_present"))
    assert_true(brake.get("cntrl/spring_brake_cuts_off_drive"))

func test_round_trip_and_update_without_crashing():
    brake.set("cntrl/brake_system", TrainBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC)
    brake.set("cntrl/brake_ctrl_position_count", 8)
    brake.set("cntrl/brake_delays", TrainBrake.BRAKE_DELAY_GPR_MG)
    brake.set("cntrl/brake_op_modes", TrainBrake.BRAKE_OP_MODE_PN)
    brake.set("cntrl/brake_handle_type", TrainBrake.BRAKE_HANDLE_TYPE_KNORR)
    brake.set("cntrl/local_brake_handle_type", TrainBrake.BRAKE_HANDLE_TYPE_WESTINGHOUSE)
    brake.set("cntrl/anti_skid_brake_type", TrainBrake.ANTI_SKID_BRAKE_AUTOMATIC)
    brake.set("cntrl/local_brake_type", TrainBrake.LOCAL_BRAKE_TYPE_HYDRAULIC)
    brake.set("cntrl/manual_brake_present", false)
    brake.set("cntrl/dynamic_brake_type", TrainBrake.DYNAMIC_BRAKE_AUTOMATIC)
    brake.set("cntrl/local_brake_traxx", true)
    brake.set("cntrl/release_parking_by_spring_brake", true)
    brake.set("cntrl/release_parking_by_spring_brake_when_door_open", true)
    brake.set("cntrl/spring_brake_cuts_off_drive", false)
    brake.set("cntrl/spring_brake_drive_emergency_velocity", 5.0)
    await wait_idle_frames(2)

    assert_eq(brake.get("cntrl/brake_system"), TrainBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC)
    assert_eq(brake.get("cntrl/brake_ctrl_position_count"), 8)
    assert_eq(brake.get("cntrl/brake_delays"), TrainBrake.BRAKE_DELAY_GPR_MG)
    assert_eq(brake.get("cntrl/dynamic_brake_type"), TrainBrake.DYNAMIC_BRAKE_AUTOMATIC)
    assert_true(brake.get("cntrl/local_brake_traxx"))
    assert_true(train.state.has("brake_air_pressure"), "TrainBrake should keep functioning after configuring the Cntrl. section")
