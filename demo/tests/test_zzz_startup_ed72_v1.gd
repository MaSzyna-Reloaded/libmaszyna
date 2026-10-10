extends MaszynaStartupTest

## ED72-010ra (pkp/ed72_v1, fixtures/scenery/startup_ed72_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_ed72_v1.scn", "ED72-010ra", Kind.ELECTRIC_MULTIPLE_UNIT)
