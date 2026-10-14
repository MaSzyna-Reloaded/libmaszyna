extends MaszynaGutTest

## Regression: the game dir was "." in an exported build. A bare relative path handed to
## FileAccess resolves against res:// - in an export the embedded pack, which holds no scenery -
## so every path built from it was unreachable, and the scenery cache's own validity check
## (SceneryInstancer._is_cache_valid()) failed on the first dependency of every entry. The result
## was a full re-parse of every scenery and subscene on each launch, while the editor, where the
## dir is a real path, cached correctly (see FINDINGS.md, 2026-09-23).
func test_game_dir_is_a_path_fileaccess_can_reach() -> void:
    # the trap itself: a relative path is res://, not the process's working directory
    assert_true(FileAccess.file_exists("project.godot"), "a bare relative path reads from res://")
    assert_true(
        UserSettings.get_maszyna_game_dir().is_absolute_path(),
        "the game dir must be absolute, or everything joined to it resolves inside res://",
    )


## A directory with the original's data (scenery/, dynamic/, textures/) and one without textures/
const GAME_DIR_FIXTURE: String = "res://tests/fixtures/game_dir"
const INCOMPLETE_GAME_DIR_FIXTURE: String = "res://tests/fixtures"


func test_game_dir_is_recognised_by_its_data_folders() -> void:
    assert_true(UserSettings.is_maszyna_game_dir(ProjectSettings.globalize_path(GAME_DIR_FIXTURE)))
    assert_false(
        UserSettings.is_maszyna_game_dir(ProjectSettings.globalize_path(INCOMPLETE_GAME_DIR_FIXTURE)),
        "a directory without textures/ is not a game directory",
    )
    assert_false(UserSettings.is_maszyna_game_dir(""))


## The game's own directory (the editor's binary here) holds no game data, so the saved one is used
func test_saved_game_dir_is_used_and_validated() -> void:
    var previous_game_dir: String = UserSettings.get_maszyna_game_dir()
    var game_dir: String = ProjectSettings.globalize_path(GAME_DIR_FIXTURE)
    watch_signals(UserSettings)
    UserSettings.save_maszyna_game_dir(game_dir)
    assert_eq(UserSettings.get_maszyna_game_dir(), game_dir.simplify_path())
    assert_true(UserSettings.is_maszyna_game_dir_valid())
    assert_signal_emitted(UserSettings, "game_dir_changed")
    UserSettings.save_maszyna_game_dir(ProjectSettings.globalize_path(INCOMPLETE_GAME_DIR_FIXTURE))
    assert_false(UserSettings.is_maszyna_game_dir_valid())
    UserSettings.save_maszyna_game_dir(previous_game_dir)


## "Discard changes" reads the file again - a directory set and not saved goes, and says so
func test_unsaved_game_dir_is_dropped_by_load_config() -> void:
    var previous_game_dir: String = UserSettings.get_maszyna_game_dir()
    UserSettings.set_setting("maszyna", "game_dir", ProjectSettings.globalize_path(GAME_DIR_FIXTURE))
    watch_signals(UserSettings)
    UserSettings.load_config()
    assert_eq(UserSettings.get_maszyna_game_dir(), previous_game_dir)
    assert_signal_emitted(UserSettings, "game_dir_changed")
