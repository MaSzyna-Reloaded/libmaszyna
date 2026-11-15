extends MaszynaGutTest

const DDSTextureLoader = preload("res://addons/libmaszyna/materials/dds_texture_loader.gd")

## DDSTextureLoader.load_image() on the data set's own share of the problem: a fifth of its .dds
## files are DX10-extended BC7 (libmaszyna#436) - the classic header's fourCC is "DX10" and a
## second, 20-byte header names the real format. Built here as minimal synthetic files instead of
## a fixture: the DDS header is a handful of fixed-width fields, easier read as the bytes a test
## writes than as an opaque binary checked in.

const MAGIC: PackedByteArray = [68, 68, 83, 32]  # "DDS "
const DX10_FOURCC: PackedByteArray = [68, 88, 49, 48]  # "DX10"
const DXGI_FORMAT_BC7_UNORM: int = 98
const DXGI_FORMAT_BC6H_UF16: int = 95  # not one this loader maps - proves the fallback path
const BC7_BLOCK_SIZE: int = 16


## A classic 124-byte header plus, when fourcc is "DX10", its 20-byte extension - mip_count levels
## of width x height, fourcc's block format; pad fills every mip level's data with one byte value,
## one value an octave apart per level so a returned level is told apart from the others
func _build_dx10_dds(width: int, height: int, mip_count: int, dxgi_format: int, fill_bytes: Array[int]) -> PackedByteArray:
    var bytes := StreamPeerBuffer.new()
    bytes.put_data(MAGIC)
    bytes.put_32(124)  # header size
    bytes.put_32(0)  # flags
    bytes.put_32(height)
    bytes.put_32(width)
    bytes.put_32(0)  # pitch_or_linear_size
    bytes.put_32(0)  # depth
    bytes.put_32(mip_count)
    for _i in range(44):  # reserved1[11]
        bytes.put_8(0)
    bytes.put_32(32)  # pixelformat.size
    bytes.put_32(0x4)  # pixelformat.flags: DDPF_FOURCC
    bytes.put_data(DX10_FOURCC)
    # the rest of the pixel format (rgbBitCount, 4 bitmasks) and the header's 4 caps fields plus
    # reserved2: the DX10 header starts only once the classic 124-byte header ends, not right
    # after fourCC
    for _i in range(40):
        bytes.put_8(0)
    bytes.put_32(dxgi_format)
    for _i in range(16):  # resource_dimension, misc_flag, array_size, misc_flags2
        bytes.put_8(0)

    var level_width: int = width
    var level_height: int = height
    for level: int in range(mip_count):
        var blocks: int = ((level_width + 3) / 4) * ((level_height + 3) / 4)
        for _i in range(blocks * BC7_BLOCK_SIZE):
            bytes.put_8(fill_bytes[level])
        level_width = maxi(level_width / 2, 1)
        level_height = maxi(level_height / 2, 1)

    return bytes.data_array


func _write_temp(bytes: PackedByteArray) -> String:
    var path: String = "user://test_dds_texture_loader.dds"
    var file: FileAccess = FileAccess.open(path, FileAccess.WRITE)
    file.store_buffer(bytes)
    file.close()
    return path


func test_reads_a_bc7_image_at_full_size() -> void:
    var path: String = _write_temp(_build_dx10_dds(8, 8, 1, DXGI_FORMAT_BC7_UNORM, [0xAA]))
    var image: Image = DDSTextureLoader.load_image(path, 8)

    assert_not_null(image)
    assert_eq(image.get_format(), Image.FORMAT_BPTC_RGBA)
    assert_eq(image.get_width(), 8)
    assert_eq(image.get_height(), 8)


## max_size smaller than the top level skips it - and the DX10 header's own 20 bytes must be
## skipped along with it, or the skipped level's data is read as the kept one's
func test_clamps_to_a_smaller_mip_level_past_the_dx10_header() -> void:
    var path: String = _write_temp(_build_dx10_dds(8, 8, 2, DXGI_FORMAT_BC7_UNORM, [0xAA, 0xBB]))
    var image: Image = DDSTextureLoader.load_image(path, 4)

    assert_not_null(image)
    assert_eq(image.get_width(), 4)
    assert_eq(image.get_height(), 4)
    var data: PackedByteArray = image.get_data()
    assert_eq(data.size(), BC7_BLOCK_SIZE, "one 4x4 block")
    assert_eq(data[0], 0xBB, "the smaller level's data, not the skipped level's")


func test_an_unmapped_dx10_format_falls_back_to_null() -> void:
    var path: String = _write_temp(_build_dx10_dds(8, 8, 1, DXGI_FORMAT_BC6H_UF16, [0xAA]))
    var image: Image = DDSTextureLoader.load_image(path, 8)

    assert_null(image, "callers fall back to load(path) for a format this loader does not map")
