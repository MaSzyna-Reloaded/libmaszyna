extends MaszynaStartupTest

## SN61-02 (pkp/sn61_v2, sn61_v1.fiz, fixtures/scenery/startup_sn61-02.scn) started from its rear cab
## with the keyboard and moved off - the vehicle of calkowo_sn61_zima.scn's Macierzewo mission


func test_starts_and_moves_off() -> void:
    await run_startup("startup_sn61-02.scn", "SN61-02", Kind.DIESEL_MECHANICAL)
