extends MaszynaStartupTest

## tem2-001a (pkp/tem2_v2, fixtures/scenery/startup_tem2_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_tem2_v2.scn", "tem2-001a", Kind.DIESEL_ELECTRIC)
