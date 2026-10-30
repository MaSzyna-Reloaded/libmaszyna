extends MaszynaBrakingTest

## 060da-297 (startup_st43_v1.scn) started as test_zzz_startup_st43_v1.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_st43_v1.scn", "060da-297", Kind.DIESEL_ELECTRIC, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_st43_v1.scn", "060da-297", Kind.DIESEL_ELECTRIC, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_st43_v1.scn", "060da-297", Kind.DIESEL_ELECTRIC, Brake.LOCAL)
