extends MaszynaBrakingTest

## eu05 (startup_eu05_v2.scn) started as test_zzz_startup_eu05_v2.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_eu05_v2.scn", "eu05", Kind.ELECTRIC_LOCOMOTIVE, Brake.TRAIN, Pantographs.SELECTOR)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_eu05_v2.scn", "eu05", Kind.ELECTRIC_LOCOMOTIVE, Brake.EMERGENCY, Pantographs.SELECTOR)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_eu05_v2.scn", "eu05", Kind.ELECTRIC_LOCOMOTIVE, Brake.LOCAL, Pantographs.SELECTOR)
