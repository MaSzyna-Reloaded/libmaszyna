extends MaszynaStartupTest

## 2ls150 (pkp/sm03_v1, fixtures/scenery/startup_sm03_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sm03_v1.scn", "2ls150", Kind.DIESEL_MECHANICAL)
