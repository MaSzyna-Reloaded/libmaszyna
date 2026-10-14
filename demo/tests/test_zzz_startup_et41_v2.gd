extends MaszynaStartupTest

## 203e-a (pkp/et41_v2, fixtures/scenery/startup_et41_v2.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_et41_v2.scn", "203e-a", Kind.ELECTRIC_LOCOMOTIVE, Pantographs.SELECTED)
