extends MaszynaStartupTest

## eu05 (pkp/eu05_v2, fixtures/scenery/startup_eu05_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_eu05_v2.scn", "eu05", Kind.ELECTRIC_LOCOMOTIVE, Pantographs.SELECTOR)
