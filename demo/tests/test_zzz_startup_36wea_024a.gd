extends MaszynaStartupTest

## 36WEa-024A (PKP/IMPULS_V1, fixtures/scenery/startup_36wea-024a.scn - l053_poranek.scn's unit, its
## pantographs by individual switches) started from its cab with the keyboard and moved off; its
## driver walking to the rear cab is the game's (the original's driver, test_zzz_ai_driver_36wea_024a.gd)


func test_starts_and_moves_off() -> void:
    await run_startup("startup_36wea-024a.scn", "36WEa-024a", Kind.ELECTRIC_MULTIPLE_UNIT)
