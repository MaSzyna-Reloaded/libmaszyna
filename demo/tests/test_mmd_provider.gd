extends MaszynaGutTest

## The vehicles' MMD files are read through the provider MaszynaVehicleMmdManager holds
## (set_mmd_provider()): an MMD a provider has and the disk has not is read as the file of the same
## words would be - its cabs, its models, whether it names its vehicle - and the files it includes
## through the provider too, the disk's where the provider hands them on; the disk's provider
## (MaszynaFileMmdProvider) again for none.

const FIXTURE_PATH:String = "res://tests/fixtures/test_cabin.mmd"
## Beside the fixture, so the files it includes are found as they are for it
const PROVIDED_PATH:String = "res://tests/fixtures/provided_cabin.mmd"
## The cab the fixture defines whole
const CAB:int = 1


## An MMD provider of texts it is handed, by their absolute paths, and of the disk's files for the
## rest
class TextProvider extends MaszynaMmdProvider:
    var texts:Dictionary[String, String] = {}
    var disk:MaszynaFileMmdProvider = MaszynaFileMmdProvider.new()

    func _file_exists(abs_path:String) -> bool:
        return texts.has(abs_path) or disk.file_exists(abs_path)

    func _read_file(abs_path:String) -> PackedByteArray:
        return texts[abs_path].to_utf8_buffer() if texts.has(abs_path) else disk.read_file(abs_path)

    func _get_modified_time(abs_path:String) -> int:
        return 0 if texts.has(abs_path) else disk.get_modified_time(abs_path)


func after_each() -> void:
    MaszynaVehicleMmdManager.set_mmd_provider(null)


func _provided_path() -> String:
    return ProjectSettings.globalize_path(PROVIDED_PATH)


## The fixture's words, at a path no file is at, through a provider of its own
func _provide_fixture() -> void:
    var provider:TextProvider = TextProvider.new()
    provider.texts[_provided_path()] = FileAccess.get_file_as_string(FIXTURE_PATH)
    MaszynaVehicleMmdManager.set_mmd_provider(provider)


func test_the_disks_provider_is_held_until_another_is_handed_in() -> void:
    assert_is(MaszynaVehicleMmdManager.get_mmd_provider(), MaszynaFileMmdProvider)

    _provide_fixture()
    assert_is(MaszynaVehicleMmdManager.get_mmd_provider(), TextProvider)

    MaszynaVehicleMmdManager.set_mmd_provider(null)
    assert_is(MaszynaVehicleMmdManager.get_mmd_provider(), MaszynaFileMmdProvider)


func test_an_mmd_of_a_provider_is_read_as_its_file_would_be() -> void:
    var from_file:MmdCabinDefinition = MmdCabinInstancer.parse(
            ProjectSettings.globalize_path(FIXTURE_PATH), {}, CAB, {})
    _provide_fixture()
    var provided:MmdCabinDefinition = MmdCabinInstancer.parse(_provided_path(), {}, CAB, {})

    assert_eq(provided.bounds_min, from_file.bounds_min)
    assert_eq(provided.bounds_max, from_file.bounds_max)
    assert_eq(provided.driver_pos, from_file.driver_pos)
    assert_eq(provided.model_relpath, from_file.model_relpath)
    assert_eq(provided.instruments.size(), from_file.instruments.size())
    assert_eq(MmdCabinInstancer.parse_body_model(_provided_path(), {}), "test_body")


func test_a_file_is_there_as_its_provider_has_it() -> void:
    assert_false(MaszynaVehicleMmdManager.mmd_file_exists(_provided_path()), "on no disk")

    _provide_fixture()
    assert_true(MaszynaVehicleMmdManager.mmd_file_exists(_provided_path()))
    assert_false(MaszynaVehicleMmdManager.names_vehicle(_provided_path()))
