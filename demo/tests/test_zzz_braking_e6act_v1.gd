extends MaszynaBrakingTest

## e6act-001 (startup_e6act_v1.scn) started as test_zzz_startup_e6act_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_e6act_v1.scn", "e6act-001", Kind.ELECTRIC_LOCOMOTIVE, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_e6act_v1.scn", "e6act-001", Kind.ELECTRIC_LOCOMOTIVE, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_e6act_v1.scn", "e6act-001", Kind.ELECTRIC_LOCOMOTIVE, Brake.LOCAL)
