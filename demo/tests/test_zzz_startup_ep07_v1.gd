extends MaszynaStartupTest

## 4e-ep-zez (pkp/ep07_v1, fixtures/scenery/startup_ep07_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_ep07_v1.scn", "4e-ep-zez", Kind.ELECTRIC_LOCOMOTIVE)
