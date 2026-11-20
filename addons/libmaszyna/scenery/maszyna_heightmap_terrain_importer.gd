@tool
extends RefCounted

## `heightmap_terrain <name> endheightmap_terrain` (state_serializer::deserialize_heightmapterrain(),
## simulationstateserializer.cpp:1324): the scenery's heightmap terrain is terrain/<name>/ of the game
## (MaszynaHeightmapTerrainRenderingServer). The original keeps one - the last named
## (EditorTerrain.open()).

func import(p:MaszynaParser, context:MaszynaImporterContext) -> Array:
    var tokens:Array = p.get_tokens_until("endheightmap_terrain")
    tokens.pop_back()
    if not tokens:
        push_error("Bad scenario: \"heightmap_terrain\" names no terrain")
        return []
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var directory:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, "terrain".path_join(String(tokens[0]))))
    context.register_dependency(directory.path_join(MaszynaHeightmapTerrainManifest.FILE_NAME))
    context.heightmap_terrains.append(directory)
    return []
