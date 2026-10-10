extends MaszynaStartupTest

## 6d-su (pkp/su42_v1, fixtures/scenery/startup_su42_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_su42_v1.scn", "6d-su", Kind.DIESEL_ELECTRIC)
