## A texture file a StreamedTexture2D loads: Godot's own Streamed Texture Format (GSTF), as its
## importer writes it (StreamedTexture2D::_save_data(), streamed_texture.cpp). Only the importer
## makes one, and _save_data() is not bound for an extension, so the original's textures - read
## from the game directory at run time - are written here, for StreamedTexture2D.load() to read
## and stream mip by mip.

const MAGIC: String = "GSTF"
## streamed_texture.cpp FORMAT_VERSION
const FORMAT_VERSION: int = 1
## The flags (normal and roughness detection, an editor's business) and the streaming limits
## (0: the project's rendering/textures/streaming/min_lod and max_lod) are left to Godot
const NO_FLAGS: int = 0
const PROJECT_LOD: int = 0
const RESERVED: int = 0


## Writes the image with its mipmaps to the path; a reader never sees half a file - it is written
## beside it, under the writing thread's own name (two workers may convert one texture), and moved
## over it whole
static func save(image: Image, path: String) -> Error:
    var partial_path: String = "%s.%d.partial" % [path, OS.get_thread_caller_id()]
    var file: FileAccess = FileAccess.open(partial_path, FileAccess.WRITE)
    if not file:
        return FileAccess.get_open_error()
    var data: PackedByteArray = image.get_data()
    var mipmaps: int = image.get_mipmap_count()
    file.store_buffer(MAGIC.to_ascii_buffer())
    file.store_32(FORMAT_VERSION)
    file.store_32(image.get_width())
    file.store_32(image.get_height())
    file.store_32(data.size())
    file.store_32(image.get_format())
    file.store_32(mipmaps)
    # each level's offset in the data and its size - the next level's offset, or the end
    for level: int in mipmaps + 1:
        var offset: int = image.get_mipmap_offset(level)
        var end: int = image.get_mipmap_offset(level + 1) if level < mipmaps else data.size()
        file.store_32(offset)
        file.store_32(end - offset)
    file.store_32(NO_FLAGS)
    file.store_32(PROJECT_LOD)
    file.store_32(PROJECT_LOD)
    file.store_32(RESERVED)
    file.store_buffer(data)
    var error: Error = file.get_error()
    file.close()
    if not error == OK:
        DirAccess.remove_absolute(partial_path)
        return error
    return DirAccess.rename_absolute(partial_path, path)
