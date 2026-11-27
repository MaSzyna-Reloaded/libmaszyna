extends MaszynaGutTest

## A data path of the game directory mounted from another directory: the files below it are read
## from there, whatever the letter case the data spells them in, the rest from the game directory

const FIXTURES:String = "res://tests/fixtures/data_path"
const VEHICLE_DATA_PATH:String = "dynamic/pkp/test_v1"
const CARRIER_DATA_PATH:String = "dynamic/pkp"


func after_each() -> void:
    MaszynaGameDirectory.data_path_unmount(VEHICLE_DATA_PATH)
    MaszynaGameDirectory.data_path_unmount(CARRIER_DATA_PATH)


func test_a_file_below_a_mounted_path_is_read_from_its_directory() -> void:
    var directory:String = ProjectSettings.globalize_path(FIXTURES.path_join("Mixed"))
    MaszynaGameDirectory.data_path_mount(VEHICLE_DATA_PATH, directory)
    assert_eq(MaszynaGameDirectory.data_path_resolve("dynamic/PKP/Test_v1/asset.txt"), directory.path_join("Asset.txt"))
    assert_eq(MaszynaGameDirectory.data_path_resolve("/" + VEHICLE_DATA_PATH), directory)


func test_a_path_mounted_nowhere_is_in_the_game_directory() -> void:
    MaszynaGameDirectory.data_path_mount(VEHICLE_DATA_PATH, ProjectSettings.globalize_path(FIXTURES.path_join("Mixed")))
    assert_eq(
            MaszynaGameDirectory.data_path_resolve("dynamic/pkp/test_v10/a.fiz"),
            MaszynaGameDirectory.get_game_dir().path_join("dynamic/pkp/test_v10/a.fiz"))


func test_the_longest_mounted_path_wins() -> void:
    var vehicle_dir:String = ProjectSettings.globalize_path(FIXTURES.path_join("Mixed"))
    var carrier_dir:String = ProjectSettings.globalize_path(FIXTURES.path_join("fallback"))
    MaszynaGameDirectory.data_path_mount(CARRIER_DATA_PATH, carrier_dir)
    MaszynaGameDirectory.data_path_mount(VEHICLE_DATA_PATH, vehicle_dir)
    assert_eq(MaszynaGameDirectory.data_path_resolve(VEHICLE_DATA_PATH.path_join("Asset.txt")), vehicle_dir.path_join("Asset.txt"))
    assert_eq(MaszynaGameDirectory.data_path_resolve("dynamic/pkp/capital.ctr"), carrier_dir.path_join("Capital.ctr"))


func test_an_unmounted_path_is_in_the_game_directory_again() -> void:
    MaszynaGameDirectory.data_path_mount(VEHICLE_DATA_PATH, ProjectSettings.globalize_path(FIXTURES.path_join("Mixed")))
    MaszynaGameDirectory.data_path_unmount(VEHICLE_DATA_PATH)
    assert_eq(
            MaszynaGameDirectory.data_path_resolve(VEHICLE_DATA_PATH.path_join("asset.txt")),
            MaszynaGameDirectory.get_game_dir().path_join(VEHICLE_DATA_PATH.path_join("asset.txt")))


## Mounting and unmounting are told, for whoever reads the path
func test_mounting_and_unmounting_tell_the_data_changed() -> void:
    watch_signals(MaszynaGameDirectory)
    MaszynaGameDirectory.data_path_mount(VEHICLE_DATA_PATH, ProjectSettings.globalize_path(FIXTURES.path_join("Mixed")))
    MaszynaGameDirectory.data_path_unmount(VEHICLE_DATA_PATH)
    MaszynaGameDirectory.data_path_unmount(VEHICLE_DATA_PATH)
    assert_signal_emit_count(MaszynaGameDirectory, "data_path_mounts_changed", 2)


## A data path mounted is no other game: the game's data is not read again (GameDataServer), which
## took its whole catalogue of vehicles anew for each folder mounted
func test_mounting_reads_no_game_data_again() -> void:
    watch_signals(GameDataServer)
    MaszynaGameDirectory.data_path_mount(VEHICLE_DATA_PATH, ProjectSettings.globalize_path(FIXTURES.path_join("Mixed")))
    MaszynaGameDirectory.data_path_unmount(VEHICLE_DATA_PATH)
    assert_signal_not_emitted(GameDataServer, "data_reload_requested")
