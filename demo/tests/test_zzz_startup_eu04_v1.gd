extends MaszynaStartupTest

## eu04-01 (pkp/eu04_v1, fixtures/scenery/startup_eu04_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_eu04_v1.scn", "eu04-01", Kind.ELECTRIC_LOCOMOTIVE)
