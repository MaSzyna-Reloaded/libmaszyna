extends MaszynaBrakingTest

## sn61-02 (startup_sn61-02.scn) started as test_zzz_startup_sn61_v2.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_sn61-02.scn", "sn61-02", Kind.DIESEL_MECHANICAL, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_sn61-02.scn", "sn61-02", Kind.DIESEL_MECHANICAL, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_sn61-02.scn", "sn61-02", Kind.DIESEL_MECHANICAL, Brake.LOCAL)
