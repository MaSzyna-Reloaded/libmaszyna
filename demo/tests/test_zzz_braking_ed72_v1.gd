extends MaszynaBrakingTest

## ed72-010ra (startup_ed72_v1.scn) started as test_zzz_startup_ed72_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_ed72_v1.scn", "ed72-010ra", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_ed72_v1.scn", "ed72-010ra", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_ed72_v1.scn", "ed72-010ra", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.LOCAL)
