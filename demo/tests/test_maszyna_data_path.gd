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


func test_any_other_difference_in_case_keeps_the_spelling_from_the_data() -> void:
    if OS.get_name() == "Windows":
        pending("Windows resolves the original spelling case-insensitively")
        return
    assert_eq(MaszynaDataPath.resolve(FIXTURES, "mixed/ASSET.txt"), "mixed/ASSET.txt")
