@tool
## Minimal DDS (DXT1/DXT3/DXT5, and DX10-extended BC4/BC5/BC7) loader that can discard the top
## mipmap levels of an oversized texture before it ever reaches the GPU - port of the original
## engine's own texture size clamp (Texture.cpp's CurrentMaxTextureSize mip-skip loop, Globals.h's
## iMaxTextureSize/maxtexturesize .ini option), which this wrapper had no equivalent of:
## MaszynaMaterialManager.load_texture() used to hand every .dds straight to Godot's own loader
## at full native resolution with no way to cap VRAM use on a weaker GPU.
##
## A fifth of the game's .dds files are DX10-extended (BC7, the data set's dynamic/pkp normal
## maps among them): the classic header's fourCC is "DX10" and a second, 20-byte header right
## after it names the real format (DXGI_FORMAT) - Godot's own loader is stricter about the
## classic header's flags than the files this loader has to read are, and refuses some of them
## outright (MaSzyna-Reloaded/libmaszyna#436).


## Where the pixel format's fourCC stands: magic, then the header up to ddspf.dwFourCC
const PIXEL_FORMAT_FOURCC_OFFSET:int = 84
## DX10_HEADER_SIZE bytes right after the classic header, before the pixel data, when the classic
## header's fourCC is "DX10": DXGI_FORMAT, D3D10_RESOURCE_DIMENSION, misc flag, array size, misc
## flags 2 - only the first is read
const DX10_HEADER_SIZE:int = 20
## DXGI_FORMAT values this loader maps to a Godot Image.Format (the data set has none but
## BC7_UNORM; BC4/BC5 are read the same way and are common in other DDS files of the format)
const DXGI_FORMAT_BC4_UNORM:int = 80
const DXGI_FORMAT_BC4_SNORM:int = 81
const DXGI_FORMAT_BC5_UNORM:int = 83
const DXGI_FORMAT_BC5_SNORM:int = 84
const DXGI_FORMAT_BC7_UNORM:int = 98
const DXGI_FORMAT_BC7_UNORM_SRGB:int = 99

## Whether the texture has an alpha channel, read from its header alone - the original's
## data_components: GL_RGB for DXT1, GL_RGBA for DXT3 and DXT5 (Texture.cpp:618-621)
static func has_alpha(path:String) -> bool:
    var file:FileAccess = FileAccess.open(path, FileAccess.READ)
    if not file or not file.get_buffer(4).get_string_from_ascii() == "DDS ":
        return false
    file.seek(PIXEL_FORMAT_FOURCC_OFFSET)
    var fourcc:String = file.get_buffer(4).get_string_from_ascii()
    return fourcc == "DXT3" or fourcc == "DXT5"


## The width and height of a .dds, from its header; zero when it is not one
static func get_size(path:String) -> Vector2i:
    var file:FileAccess = FileAccess.open(path, FileAccess.READ)
    if not file or not file.get_buffer(4).get_string_from_ascii() == "DDS ":
        return Vector2i.ZERO
    file.get_32()  # header size
    file.get_32()  # flags
    var height:int = file.get_32()
    var width:int = file.get_32()
    return Vector2i(width, height)


## The texture's Image, read on any thread - the RenderingServer's texture is made of it on the
## main thread (ImageTexture.create_from_image()). Null on anything this loader doesn't handle (bad
## magic, unsupported fourCC, a max_size too small to keep any mip level) - callers should fall
## back to load(path).
static func load_image(path:String, max_size:int) -> Image:
    var file:FileAccess = FileAccess.open(path, FileAccess.READ)
    if not file:
        return null

    if file.get_buffer(4).get_string_from_ascii() != "DDS ":
        return null

    file.get_32()  # header size
    file.get_32()  # flags
    var height:int = file.get_32()
    var width:int = file.get_32()
    file.get_32()  # pitch_or_linear_size
    file.get_32()  # depth
    var mipmap_count:int = maxi(file.get_32(), 1)
    file.seek(file.get_position() + 44)  # reserved1[11]

    file.get_32()  # pixelformat.size
    file.get_32()  # pixelformat.flags
    var fourcc:String = file.get_buffer(4).get_string_from_ascii()

    var format:Image.Format
    var block_size:int
    var data_start:int = 4 + 124  # magic + header
    match fourcc:
        "DXT1":
            format = Image.FORMAT_DXT1
            block_size = 8
        "DXT3":
            format = Image.FORMAT_DXT3
            block_size = 16
        "DXT5":
            format = Image.FORMAT_DXT5
            block_size = 16
        "DX10":
            # the DX10 header always starts right after the classic one (DDS_HEADER is a fixed
            # 124 bytes whichever pixel format it names), not right after fourCC
            file.seek(4 + 124)
            var dxgi_format:int = file.get_32()
            file.seek(file.get_position() + DX10_HEADER_SIZE - 4)  # the rest of the DX10 header
            data_start += DX10_HEADER_SIZE
            match dxgi_format:
                DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM:
                    format = Image.FORMAT_RGTC_R
                    block_size = 8
                DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM:
                    format = Image.FORMAT_RGTC_RG
                    block_size = 16
                DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB:
                    format = Image.FORMAT_BPTC_RGBA
                    block_size = 16
                _:
                    return null
        _:
            return null
    var file_size:int = file.get_length()
    var offset:int = 0
    while (width > max_size or height > max_size) and mipmap_count > 1:
        offset += ((width + 3) / 4) * ((height + 3) / 4) * block_size
        width /= 2
        height /= 2
        mipmap_count -= 1

    # Quirk: some data declares more levels than its size has (dynamic/pkp/e186_v2/szyba_1_refl.dds
    # is 1x1 with 2), which Image.create_from_data() refuses and the original's loader shrugs off.
    # Godot takes the whole chain down to 1x1 or the top level alone, so a shorter chain keeps the top.
    var full_chain_levels:int = 1
    while (width >> (full_chain_levels - 1)) > 1 or (height >> (full_chain_levels - 1)) > 1:
        full_chain_levels += 1
    var levels:int = full_chain_levels if mipmap_count >= full_chain_levels else 1
    var data_size:int = 0
    for level:int in levels:
        data_size += ((maxi(width >> level, 1) + 3) / 4) * ((maxi(height >> level, 1) + 3) / 4) * block_size
    if data_size > file_size - data_start - offset:
        return null

    file.seek(data_start + offset)
    var data:PackedByteArray = file.get_buffer(data_size)

    return Image.create_from_data(width, height, levels > 1, format, data)
