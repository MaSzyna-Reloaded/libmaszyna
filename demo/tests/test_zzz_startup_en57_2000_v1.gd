extends MaszynaStartupTest

## EN57-2067ra (PKP/EN57-2000_V1, fixtures/scenery/startup_en57-2000_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_en57-2000_v1.scn", "EN57-2067ra", Kind.ELECTRIC_MULTIPLE_UNIT)
