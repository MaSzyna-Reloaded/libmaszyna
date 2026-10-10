extends MaszynaStartupTest

## ED78-024A (pkp/impuls_v1, fixtures/scenery/startup_impuls_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_impuls_v1.scn", "ED78-024A", Kind.ELECTRIC_MULTIPLE_UNIT, Pantographs.SELECTOR)
