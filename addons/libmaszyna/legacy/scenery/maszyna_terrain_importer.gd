@tool
extends RefCounted

## `terrain <file>.sbt endterrain`: the scenery's terrain from here on is that region file's
## (state_serializer::deserialize_terrain(), simulationstateserializer.cpp:786)
func import(p:MaszynaParser, context: MaszynaImporterContext):
    var tokens:Array = p.get_tokens_until("endterrain")
    var filename:String = String(tokens[0]).replace("\\", "/") if tokens.size() > 1 else ""
    if filename.to_lower().ends_with(MaszynaImporterContext.REGION_EXTENSION):
        var scenery_dir:String = UserSettings.get_maszyna_game_dir().path_join("scenery")
        context.load_binary_terrain(scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, filename)))
    return []
