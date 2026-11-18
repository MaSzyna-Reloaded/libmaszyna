@tool
extends CompiledScenery
class_name MaszynaCompiledScenery

## A .scn compiled: the core's CompiledScenery, what MaSzyna's format adds (events, memory cells,
## launchers, isolated sections, terrain) and what its cache is checked by

@export var format_version:int = 0
@export var source_path:String = ""
@export var parameters_hash:String = ""
@export var dependencies:Dictionary = {}
@export var events:Array[MaszynaEventData] = []
@export var memcells:Array[MaszynaMemcellData] = []
@export var launchers:Array[MaszynaEventLauncherData] = []
@export var isolated_sections:Array[MaszynaIsolatedData] = []
## The last `config hiddenevents` read (MaszynaImporterContext.hidden_events)
@export var hidden_events:int = MaszynaImporterContext.DEFAULT_HIDDEN_EVENTS
## The region files (.sbt) whose terrain is supplied as the camera comes near
## (MaszynaSBTTerrainProvider), not read into triangle_chunks
@export var region_files:Array[String] = []
## The binary terrain files (.btf) whose terrain is supplied as the camera comes near
## (MaszynaBTFTerrainProvider), not read into triangle_chunks
@export var terrain_binary_files:Array[String] = []
## The folders of the heightmap terrains named, the last the scenery's
## (MaszynaHeightmapTerrainRenderingServer)
@export var heightmap_terrains:Array[String] = []
## Merged triangle meshes, streamed by MaszynaSceneryChunkRenderingServer (not nodes, so not in [member nodes])
@export var triangle_chunks:Array[MaszynaTrianglesChunkData] = []
