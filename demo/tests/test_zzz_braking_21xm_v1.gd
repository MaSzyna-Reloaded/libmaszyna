extends MaszynaBrakingTest

## sa132-006a (startup_sa132-006a.scn) started as test_zzz_startup_21xm_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_sa132-006a.scn", "sa132-006a", Kind.DIESEL_MULTIPLE_UNIT, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_sa132-006a.scn", "sa132-006a", Kind.DIESEL_MULTIPLE_UNIT, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_sa132-006a.scn", "sa132-006a", Kind.DIESEL_MULTIPLE_UNIT, Brake.LOCAL)
