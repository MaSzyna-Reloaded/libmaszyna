extends MaszynaStartupTest

## e6act-001 (pkp/e6act_v1, fixtures/scenery/startup_e6act_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_e6act_v1.scn", "e6act-001", Kind.ELECTRIC_LOCOMOTIVE)
