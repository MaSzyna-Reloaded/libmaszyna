extends MaszynaStartupTest

## 301dd (pkp/st45_v2, fixtures/scenery/startup_st45_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_st45_v2.scn", "301dd", Kind.DIESEL_ELECTRIC)
