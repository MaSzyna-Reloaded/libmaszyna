extends MaszynaStartupTest

## et40v1a (pkp/et40_v1, fixtures/scenery/startup_et40_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_et40_v1.scn", "et40v1a", Kind.ELECTRIC_LOCOMOTIVE, Pantographs.SELECTOR)
