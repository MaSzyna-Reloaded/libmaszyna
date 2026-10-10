extends MaszynaStartupTest

## wmb10 (pkp/wmb10_v1, fixtures/scenery/startup_wmb10_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_wmb10_v1.scn", "wmb10", Kind.DIESEL_MECHANICAL)
