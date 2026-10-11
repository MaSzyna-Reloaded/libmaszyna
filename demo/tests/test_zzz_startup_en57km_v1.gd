extends MaszynaStartupTest

## en57akm-1562ra (pkp/en57km_v1, fixtures/scenery/startup_en57km_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_en57km_v1.scn", "en57akm-1562ra", Kind.ELECTRIC_MULTIPLE_UNIT)
