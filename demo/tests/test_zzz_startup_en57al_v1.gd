extends MaszynaStartupTest

## EN57AL-1230ra (pkp/en57al_v1, fixtures/scenery/startup_en57al_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_en57al_v1.scn", "EN57AL-1230ra", Kind.ELECTRIC_MULTIPLE_UNIT)
