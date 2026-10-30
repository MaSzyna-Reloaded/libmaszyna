extends MaszynaBrakingTest

## sr61v1 (startup_sr61_v2.scn) started as test_zzz_startup_sr61_v2.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_sr61_v2.scn", "sr61v1", Kind.DIESEL_MECHANICAL, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_sr61_v2.scn", "sr61v1", Kind.DIESEL_MECHANICAL, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_sr61_v2.scn", "sr61v1", Kind.DIESEL_MECHANICAL, Brake.LOCAL)
