extends MaszynaStartupTest

## sa133-021a (pkp/sa134_v1, fixtures/scenery/startup_sa134_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sa134_v1.scn", "sa133-021a", Kind.DIESEL_MULTIPLE_UNIT)
