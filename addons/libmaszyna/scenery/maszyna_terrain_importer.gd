@tool
extends RefCounted

## `terrain <file> ... endterrain` (state_serializer::deserialize_terrain(),
## simulationstateserializer.cpp:1296): the scenery's terrain from the files it names, in their
## order. A region file (.sbt) is the scenery's terrain from here on; a terrain file - static
## geometry kept as text (.txtf), or the same geometry in binary (.btf, MaszynaBTFTerrainProvider) -
## is read where the directive stands (include_terrain_file(), :1196).

## The text terrain file and its binary version (scene/sceneterrain.h)
const TEXT_EXTENSION:String = ".txtf"
const BINARY_EXTENSION:String = ".btf"


## Where the binary files made of text terrain files are kept, a file a text
const BINARY_CACHE_DIRECTORY:String = "terrain_binary"

static var _binary_cache:ResourceCache = ResourceCache.create(BINARY_CACHE_DIRECTORY)


## A terrain file is taken as the original takes it (include_terrain_file(), terrain_file::prepare(),
## sceneterrain.cpp:669): in binary, streamed by MaszynaBTFTerrainProvider - the binary file beside
## the text where it was made from it, by the text's length (the original's default check,
## Globals.h:109), or alone; else one made of the text (MaszynaBTFTerrainWriter), kept in the cache
## where the original writes it beside the text. The text is read as any scenery file only where
## no binary file can stand in for it: placed under an origin or a rotation, which the binary file
## does not hold, or holding what a binary file cannot. A name of another extension (.scm) is read
## as named while that file is there, else as the terrain file of the same name.
func import(p:MaszynaParser, context:MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens_until("endterrain")
    tokens.pop_back()
    var scenery_dir:String = MaszynaGameDirectory.get_game_dir().path_join("scenery")
    var relocated:bool = not context.origin == Vector3.ZERO or not context.rotate == Vector3.ZERO
    var objects:Array = []
    for token:Variant in tokens:
        var filename:String = String(token).replace("\\", "/")
        var extension:String = "." + filename.get_extension().to_lower()
        if extension == MaszynaImporterContext.REGION_EXTENSION:
            context.load_binary_terrain(scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, filename)))
            continue
        if not (extension in [TEXT_EXTENSION, BINARY_EXTENSION]):
            var named:String = MaszynaDataPath.resolve(scenery_dir, filename)
            if FileAccess.file_exists(scenery_dir.path_join(named)):
                objects.append_array(MaszynaSceneryProvider.include_importer.include_file(named, {}, context))
                continue
        var stem:String = filename.get_basename()
        var text:String = MaszynaDataPath.resolve(scenery_dir, stem + TEXT_EXTENSION)
        var text_path:String = scenery_dir.path_join(text)
        var binary_path:String = scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, stem + BINARY_EXTENSION))
        var text_file:FileAccess = FileAccess.open(text_path, FileAccess.READ)
        context.register_dependency(text_path)
        if relocated:
            if text_file:
                objects.append_array(MaszynaSceneryProvider.include_importer.include_file(text, {}, context))
            else:
                push_error("Bad scenario: terrain file \"%s\" placed with an offset or rotation can be loaded only from the text, which is missing" % filename)
            continue
        var text_length:int = text_file.get_length() if text_file else -1
        var binary_length:int = MaszynaBTFTerrainProvider.get_source_length(binary_path)
        if text_file and not binary_length == text_length:
            # made of the text, as the original makes it beside the text
            binary_path = _binary_cache.get_file_path(text.get_basename()) + BINARY_EXTENSION
            if not MaszynaBTFTerrainProvider.get_source_length(binary_path) == text_length \
                    and not MaszynaBTFTerrainWriter.write(text_path, binary_path):
                objects.append_array(MaszynaSceneryProvider.include_importer.include_file(text, {}, context))
                continue
        elif binary_length < 0:
            push_error("Bad scenario: terrain file \"%s\" not found" % filename)
            continue
        context.register_dependency(binary_path)
        context.terrain_binary_files.append(binary_path)
    return objects
