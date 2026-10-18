extends MaszynaGutTest

## streamed_texture_file.gd: an image written in Godot's Streamed Texture Format is read back by
## StreamedTexture2D.load() as the same image - its size, format, mip levels and data.

const StreamedTextureFile = preload("res://addons/libmaszyna/legacy/materials/streamed_texture_file.gd")
const DDSTextureLoader = preload("res://addons/libmaszyna/legacy/materials/dds_texture_loader.gd")
const SAVE_PATH:String = "user://test_streamed_texture_file.stex"
## A 4x4 DXT5 of one level
const DDS_FIXTURE:String = "res://tests/fixtures/material_preload/preloaded.dds"
const DDS_MAX_SIZE:int = 1024
## Large enough for a chain of several levels
const RGBA_SIZE:int = 64


func after_each() -> void:
    DirAccess.remove_absolute(SAVE_PATH)


func _assert_read_back(image:Image) -> void:
    assert_eq(StreamedTextureFile.save(image, SAVE_PATH), OK)
    var texture:StreamedTexture2D = StreamedTexture2D.new()
    assert_eq(texture.load(SAVE_PATH), OK)
    var read:Image = texture.get_image()
    assert_eq(read.get_size(), image.get_size())
    assert_eq(read.get_format(), image.get_format())
    assert_eq(read.get_mipmap_count(), image.get_mipmap_count())
    assert_eq(read.get_data(), image.get_data(), "the data of every level as it was written")


func test_an_image_with_its_mip_levels_reads_back_the_same() -> void:
    var image:Image = Image.create_empty(RGBA_SIZE, RGBA_SIZE, false, Image.FORMAT_RGBA8)
    image.fill(Color.ORANGE)
    image.fill_rect(Rect2i(0, 0, RGBA_SIZE / 2, RGBA_SIZE / 2), Color.NAVY_BLUE)
    image.generate_mipmaps()
    _assert_read_back(image)


func test_a_dds_of_the_original_reads_back_the_same() -> void:
    _assert_read_back(DDSTextureLoader.load_image(ProjectSettings.globalize_path(DDS_FIXTURE), DDS_MAX_SIZE))


func test_no_partial_file_is_left_beside_it() -> void:
    var image:Image = Image.create_empty(RGBA_SIZE, RGBA_SIZE, true, Image.FORMAT_RGBA8)
    assert_eq(StreamedTextureFile.save(image, SAVE_PATH), OK)
    var names:PackedStringArray = DirAccess.get_files_at(SAVE_PATH.get_base_dir())
    for file_name:String in names:
        assert_false(file_name.ends_with(".partial"), "%s left behind" % file_name)
