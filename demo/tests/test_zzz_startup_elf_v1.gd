extends MaszynaStartupTest

## EN96-004-A (PKP/ELF_V1, fixtures/scenery/startup_elf_v1.scn) started from its cab with the keyboard
## and moved off


func test_starts_and_moves_off() -> void:
    await run_startup("startup_elf_v1.scn", "EN96-004-A", Kind.ELECTRIC_MULTIPLE_UNIT)
