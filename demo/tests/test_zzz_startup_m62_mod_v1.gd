extends MaszynaStartupTest

## st44-u (pkp/m62_mod_v1, fixtures/scenery/startup_m62_mod_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_m62_mod_v1.scn", "st44-u", Kind.DIESEL_ELECTRIC)
