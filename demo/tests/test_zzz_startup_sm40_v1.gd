extends MaszynaStartupTest

## sm40 (pkp/sm40_v1, fixtures/scenery/startup_sm40_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sm40_v1.scn", "sm40", Kind.DIESEL_ELECTRIC)
