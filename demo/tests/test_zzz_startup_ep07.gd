extends MaszynaStartupTest

## EP07-424 (fixtures/dynamic/pkp/303e_v1) started from its cab with the keyboard and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("ep07.scn", "EP07-424", Kind.ELECTRIC_LOCOMOTIVE)
