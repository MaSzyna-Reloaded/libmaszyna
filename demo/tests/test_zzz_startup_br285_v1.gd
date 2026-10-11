extends MaszynaStartupTest

## br285 (pkp/br285_v1, fixtures/scenery/startup_br285_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_br285_v1.scn", "br285", Kind.DIESEL_ELECTRIC)
