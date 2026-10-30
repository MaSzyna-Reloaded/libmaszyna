extends MaszynaBrakingTest

## br285 (startup_br285_v1.scn) started as test_zzz_startup_br285_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_br285_v1.scn", "br285", Kind.DIESEL_ELECTRIC, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_br285_v1.scn", "br285", Kind.DIESEL_ELECTRIC, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_br285_v1.scn", "br285", Kind.DIESEL_ELECTRIC, Brake.LOCAL)
