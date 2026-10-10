extends MaszynaStartupTest

## asf (pkp/el16_v2, fixtures/scenery/startup_el16_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_el16_v2.scn", "asf", Kind.ELECTRIC_LOCOMOTIVE)
