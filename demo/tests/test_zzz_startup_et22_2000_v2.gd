extends MaszynaStartupTest

## 201em (pkp/et22-2000_v2, fixtures/scenery/startup_et22-2000_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_et22-2000_v2.scn", "201em", Kind.ELECTRIC_LOCOMOTIVE)
