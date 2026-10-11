extends MaszynaStartupTest

## 102e-zez (pkp/ep08_v1, fixtures/scenery/startup_ep08_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_ep08_v1.scn", "102e-zez", Kind.ELECTRIC_LOCOMOTIVE)
