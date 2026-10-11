extends MaszynaStartupTest

## 201e-rn (pkp/et22_v3, fixtures/scenery/startup_et22_v3.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_et22_v3.scn", "201e-rn", Kind.ELECTRIC_LOCOMOTIVE)
