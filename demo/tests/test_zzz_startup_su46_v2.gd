extends MaszynaStartupTest

## 303d (pkp/su46_v2, fixtures/scenery/startup_su46_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_su46_v2.scn", "303d", Kind.DIESEL_ELECTRIC)
