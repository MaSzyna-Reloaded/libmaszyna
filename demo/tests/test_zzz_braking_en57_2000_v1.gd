extends MaszynaBrakingTest

## en57-2067ra (startup_en57-2000_v1.scn) started as test_zzz_startup_en57_2000_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_en57-2000_v1.scn", "en57-2067ra", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_en57-2000_v1.scn", "en57-2067ra", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_en57-2000_v1.scn", "en57-2067ra", Kind.ELECTRIC_MULTIPLE_UNIT, Brake.LOCAL)
