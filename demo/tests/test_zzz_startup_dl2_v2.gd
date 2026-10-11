extends MaszynaStartupTest

## dl2 (pkp/dl2_v2, fixtures/scenery/startup_dl2_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_dl2_v2.scn", "dl2", Kind.DIESEL_MECHANICAL)
