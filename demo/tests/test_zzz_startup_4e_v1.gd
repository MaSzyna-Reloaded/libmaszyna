extends MaszynaStartupTest

## 4e-ep-tv (pkp/4e_v1, fixtures/scenery/startup_4e_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_4e_v1.scn", "4e-ep-tv", Kind.ELECTRIC_LOCOMOTIVE)
