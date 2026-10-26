extends MaszynaGutTest

## MaszynaMaterialManager.material_preload(): a worker reads a material's files - its .mat and its textures'
## images - and the main thread makes the material of them while the preload is held, reading no file.

## A game directory with a texture and no .mat
const FIXTURES_GAME_DIR:String = "res://tests/fixtures/material_preload"
## A material without a .mat: its texture is preloaded.dds, a 4x4 DXT5
const MATERIAL_NAME:String = "preloaded"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    MaszynaMaterialManager.clear_cache()


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)
    MaszynaMaterialManager.clear_cache()


func test_a_preload_reads_the_material_and_its_textures() -> void:
    var files:MaszynaMaterialManager.MaterialPreload = MaszynaMaterialManager.MaterialPreload.new()
    MaszynaMaterialManager.material_preload("", MATERIAL_NAME, MaszynaMaterialManager.MAX_TEXTURE_SIZE_PROJECT_DEFAULT, files)
    assert_eq(files.materials.size(), 1)
    assert_eq(files.images.size(), 1, "the texture read as an image")


func test_the_main_thread_takes_what_a_held_preload_read() -> void:
    var files:MaszynaMaterialManager.MaterialPreload = MaszynaMaterialManager.MaterialPreload.new()
    MaszynaMaterialManager.material_preload("", MATERIAL_NAME, MaszynaMaterialManager.MAX_TEXTURE_SIZE_PROJECT_DEFAULT, files)
    assert_same(MaszynaMaterialManager.load_material("", MATERIAL_NAME), files.materials[0], "the .mat parsed again")
    var texture:Texture2D = MaszynaMaterialManager.load_texture("", MATERIAL_NAME)
    assert_true(texture is ImageTexture)
    assert_eq(texture.get_size(), Vector2(files.images[0].get_size()), "made of the preloaded image")


func test_a_texture_named_with_its_traits_is_its_file() -> void:
    # models/terenowe/1k96w220m's skin on Baltyk, "peronowe/krawedz_zolty2:#" (Texture.cpp:1310-1318)
    var texture:Texture2D = MaszynaMaterialManager.load_texture("", MATERIAL_NAME + ":#")
    assert_ne(texture, MaszynaMaterialManager.UNKNOWN_TEXTURE, "the traits are no part of the file's name")


func test_a_texture_a_script_draws_is_its_screen_texture() -> void:
    # a direction board on Baltyk (Texture.cpp:1294-1305): the script's screen, one for the name
    var name:String = "make:scripts/tablica_kierunek?kierunek_lewy=Baltyk Glowny&kierunek_prawy=Alakowice"
    var texture:Texture2D = MaszynaMaterialManager.load_texture("", name)
    assert_ne(texture, MaszynaMaterialManager.UNKNOWN_TEXTURE, "not looked for as a file")
    assert_same(MaszynaMaterialManager.load_texture("", name), texture, "drawn once")


func test_a_material_without_a_preload_is_read_on_its_own() -> void:
    var files:MaszynaMaterialManager.MaterialPreload = MaszynaMaterialManager.MaterialPreload.new()
    MaszynaMaterialManager.material_preload("", MATERIAL_NAME, MaszynaMaterialManager.MAX_TEXTURE_SIZE_PROJECT_DEFAULT, files)
    var preloaded_id:int = files.materials[0].get_instance_id()
    files = null
    var read:MaszynaMaterial = MaszynaMaterialManager.load_material("", MATERIAL_NAME)
    assert_not_null(read)
    assert_ne(read.get_instance_id(), preloaded_id, "a preload let go keeps nothing")
