extends MaszynaGutTest

## MaszynaFileIndexer keeps what a scenery's loading found of the data and what its readers gave up
## on, from MaszynaSceneryProvider.compile()'s files_clear() on

const FIXTURES:String = "res://tests/fixtures/data_path"


func before_each() -> void:
    MaszynaFileIndexer.files_clear()


func test_a_file_a_reader_gave_up_on_is_kept_with_what_it_is() -> void:
    MaszynaFileIndexer.missing_file_add("glinojeck/glinojeck.ctr", MaszynaFileIndexer.MASZYNA_FILE_INCLUDE)
    MaszynaFileIndexer.missing_file_add("pkp/brak/brak.fiz", MaszynaFileIndexer.MASZYNA_FILE_FIZ)
    var missing:Dictionary = MaszynaFileIndexer.missing_files_get()
    assert_eq(missing.size(), 2)
    assert_eq(missing["glinojeck/glinojeck.ctr"], MaszynaFileIndexer.MASZYNA_FILE_INCLUDE)
    assert_eq(missing["pkp/brak/brak.fiz"], MaszynaFileIndexer.MASZYNA_FILE_FIZ)


## A candidate that is not there is no missing file: only its reader knows when none was
func test_a_lookup_that_finds_nothing_is_no_missing_file() -> void:
    MaszynaDataPath.resolve(FIXTURES, "Missing/Asset.txt")
    assert_eq(MaszynaFileIndexer.missing_files_get().size(), 0)


func test_a_file_found_is_counted_once() -> void:
    MaszynaDataPath.resolve(FIXTURES, "Mixed/Asset.txt")
    MaszynaDataPath.resolve(FIXTURES, "Mixed/Asset.txt")
    MaszynaDataPath.resolve(FIXTURES, "fallback/asset.txt")
    assert_eq(MaszynaFileIndexer.found_files_get_count(), 2)


func test_files_clear_forgets_both() -> void:
    MaszynaDataPath.resolve(FIXTURES, "Mixed/Asset.txt")
    MaszynaFileIndexer.missing_file_add("brak.inc", MaszynaFileIndexer.MASZYNA_FILE_INCLUDE)
    MaszynaFileIndexer.files_clear()
    assert_eq(MaszynaFileIndexer.found_files_get_count(), 0)
    assert_eq(MaszynaFileIndexer.missing_files_get().size(), 0)
