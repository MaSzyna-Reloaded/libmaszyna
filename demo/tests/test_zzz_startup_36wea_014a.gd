extends MaszynaStartupTest

## 36WEa-014A (pkp/impuls_v1, fixtures/scenery/startup_36wea-014a.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_36wea-014a.scn", "36WEa-014A", Kind.ELECTRIC_MULTIPLE_UNIT, Pantographs.SELECTOR)
