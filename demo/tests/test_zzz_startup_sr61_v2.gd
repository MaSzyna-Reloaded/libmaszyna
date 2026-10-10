extends MaszynaStartupTest

## sr61v1 (pkp/sr61_v2, fixtures/scenery/startup_sr61_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sr61_v2.scn", "sr61v1", Kind.DIESEL_MECHANICAL)
