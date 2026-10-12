extends MaszynaStartupTest

## sa132-006a (pkp/21xm_v1, fixtures/scenery/startup_sa132-006a.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sa132-006a.scn", "sa132-006a", Kind.DIESEL_MULTIPLE_UNIT)
