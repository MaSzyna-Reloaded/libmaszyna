extends MaszynaBrakingTest

## ed78-024a (startup_impuls_v1.scn) started as test_zzz_startup_impuls_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_impuls_v1.scn", "ed78-024a", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.TRAIN, Pantographs.SELECTOR)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_impuls_v1.scn", "ed78-024a", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.EMERGENCY, Pantographs.SELECTOR)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_impuls_v1.scn", "ed78-024a", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.LOCAL, Pantographs.SELECTOR)
