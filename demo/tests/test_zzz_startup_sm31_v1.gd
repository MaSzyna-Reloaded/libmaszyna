extends MaszynaStartupTest

## 411d-025 (pkp/sm31_v1, fixtures/scenery/startup_sm31_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sm31_v1.scn", "411d-025", Kind.DIESEL_ELECTRIC)
