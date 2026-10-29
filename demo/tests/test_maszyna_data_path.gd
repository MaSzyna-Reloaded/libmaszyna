extends MaszynaGutTest

const FIXTURES:String = "res://tests/fixtures/data_path"


func test_existing_path_keeps_the_spelling_from_the_data() -> void:
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "Mixed/Asset.txt"), "Mixed/Asset.txt")


func test_missing_original_path_falls_back_to_lowercase() -> void:
    if OS.get_name() == "Windows":
        pending("Windows resolves the original spelling case-insensitively")
        return
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "FALLBACK/ASSET.TXT"), "fallback/asset.txt")


func test_missing_path_keeps_the_original_spelling_for_diagnostics() -> void:
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "Missing/Asset.txt"), "Missing/Asset.txt")


func test_windows_separators_are_normalized_without_changing_case() -> void:
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "Mixed\\Asset.txt"), "Mixed/Asset.txt")


func test_any_other_difference_in_case_matches_each_part_of_the_path() -> void:
    if OS.get_name() == "Windows":
        pending("Windows resolves the original spelling case-insensitively")
        return
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "mixed/ASSET.txt"), "Mixed/Asset.txt")


## #338: Glinojeck.scn includes glinojeck/glinojeck.ctr, the file is glinojeck/Glinojeck.ctr
func test_a_file_is_found_whatever_its_case_in_a_directory_as_written() -> void:
    if OS.get_name() == "Windows":
        pending("Windows resolves the original spelling case-insensitively")
        return
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "fallback/capital.ctr"), "fallback/Capital.ctr")
