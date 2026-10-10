extends MaszynaStartupTest

## 301d (pkp/su45_v2, fixtures/scenery/startup_su45_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_su45_v2.scn", "301d", Kind.DIESEL_ELECTRIC)
