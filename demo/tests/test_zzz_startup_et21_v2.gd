extends MaszynaStartupTest

## 3e2 (pkp/et21_v2, fixtures/scenery/startup_et21_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_et21_v2.scn", "3e2", Kind.ELECTRIC_LOCOMOTIVE)
