extends MaszynaStartupTest

## 104e-039 (pkp/ep09_v1, fixtures/scenery/startup_ep09_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_ep09_v1.scn", "104e-039", Kind.ELECTRIC_LOCOMOTIVE)
