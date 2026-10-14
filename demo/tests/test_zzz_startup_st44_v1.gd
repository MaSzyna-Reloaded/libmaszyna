extends MaszynaStartupTest

## st44-n (pkp/st44_v1, fixtures/scenery/startup_st44_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_st44_v1.scn", "st44-n", Kind.DIESEL_ELECTRIC)
