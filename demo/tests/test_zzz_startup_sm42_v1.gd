extends MaszynaStartupTest

## 6d1 (pkp/sm42_v1, fixtures/scenery/startup_sm42_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sm42_v1.scn", "6d1", Kind.DIESEL_ELECTRIC)
