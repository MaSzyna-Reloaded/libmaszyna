extends MaszynaGutTest

## A cab screen's picture replaces the texture of its material in every material of the texture's
## name (MaszynaMaterialManager.material_replace_texture()), as the original draws a screen into the
## texture itself.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## A model's directory, as the game hands it, with a leading slash (MaszynaMaterialManager drops the
## first segment of it)
const DATA_PATH:String = "/dynamic/test/animated_v1"
## The screen's material, its texture screen.dds
const SCREEN_MATERIAL:String = "screen"


## Train.cpp:10691: every material of the texture's name shows the screen - a self-illuminated
## variant (timetable_podsw beside timetable), one built after the screen - and shows its own
## texture again when the screen goes
func test_a_screen_shows_on_every_variant_of_its_material() -> void:
    var previous_game_dir:String = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(ProjectSettings.globalize_path(FIXTURES_GAME_DIR))
    var model_path:String = DATA_PATH.trim_prefix("/")
    var lit:MaszynaMaterialManager.MaterialOptions = MaszynaMaterialManager.MaterialOptions.new()
    lit.selfillum_enabled = true
    var plain:ShaderMaterial = MaszynaMaterialManager.get_material(model_path, SCREEN_MATERIAL)
    var illuminated:ShaderMaterial = MaszynaMaterialManager.get_material(model_path, SCREEN_MATERIAL, lit)
    var own:Texture2D = MaszynaMaterialManager.material_get_texture(plain)
    var screen:ImageTexture = ImageTexture.new()

    MaszynaMaterialManager.material_replace_texture(plain, screen)
    var cut:MaszynaMaterialManager.MaterialOptions = MaszynaMaterialManager.MaterialOptions.new()
    cut.cull_disabled = true
    var later:ShaderMaterial = MaszynaMaterialManager.get_material(model_path, SCREEN_MATERIAL, cut)
    assert_ne(plain, illuminated, "two materials of one name")
    assert_eq(MaszynaMaterialManager.material_get_texture(illuminated), screen, "the variant shows the screen")
    assert_eq(MaszynaMaterialManager.material_get_texture(later), screen, "and one built after it")
    MaszynaMaterialManager.material_restore_texture(plain)
    assert_eq(MaszynaMaterialManager.material_get_texture(illuminated), own, "its own texture again")
    MaszynaGameDirectory.save_game_dir(previous_game_dir)
