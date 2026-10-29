extends MaszynaGutTest

## The keys PythonScreenState hands a script without a vehicle - every one the original hands over,
## those it gives only for what the train has included (Train.cpp:696-940, mtable.cpp:641-671)


func test_the_keys_of_what_the_train_may_have_are_present() -> void:
    var screen: Dictionary = PythonScreenState.compose(RID(), {})

    # Train.cpp:860-865 - the inverters of a powered car
    assert_eq(screen["eimp_c8_invno"], 0)
    assert_false(screen["eimp_c8_inv2_act"])
    assert_false(screen["eimp_c1_inv1_allow"])
    # Train.cpp:847-849 - a compressor of the train
    assert_false(screen["compressors_20_work"])
    assert_eq(screen["compressors_1_car_no"], 0)
    # mtable.cpp:660-668 - a station of the timetable, as TMTableLine starts (mtable.h:28-38)
    assert_eq(screen["train_station1_name"], "nowhere")
    assert_eq(screen["train_station100_ah"], -1)
    assert_eq(screen["train_station1_tracks"], 1)
