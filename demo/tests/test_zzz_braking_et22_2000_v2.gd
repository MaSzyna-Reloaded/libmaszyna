extends MaszynaBrakingTest

## 201em (startup_et22-2000_v2.scn) started as test_zzz_startup_et22_2000_v2.gd does it, driven up to
## speed and stopped by each of its brakes within what its data allows


func test_the_train_brake_stops_it() -> void:
    await run_braking("startup_et22-2000_v2.scn", "201em", Kind.ELECTRIC_LOCOMOTIVE, Brake.TRAIN)


func test_the_emergency_brake_stops_it() -> void:
    await run_braking("startup_et22-2000_v2.scn", "201em", Kind.ELECTRIC_LOCOMOTIVE, Brake.EMERGENCY)


func test_the_local_brake_stops_it() -> void:
    await run_braking("startup_et22-2000_v2.scn", "201em", Kind.ELECTRIC_LOCOMOTIVE, Brake.LOCAL)
