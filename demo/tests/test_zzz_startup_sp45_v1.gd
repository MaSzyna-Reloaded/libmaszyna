extends MaszynaStartupTest

## 301db-152 (pkp/sp45_v1, fixtures/scenery/startup_sp45_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sp45_v1.scn", "301db-152", Kind.DIESEL_ELECTRIC)
