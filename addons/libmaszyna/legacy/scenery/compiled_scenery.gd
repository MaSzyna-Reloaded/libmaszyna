@tool
extends Resource
class_name MaszynaCompiledScenery


@export var format_version:int = 0
@export var source_path:String = ""
@export var parameters_hash:String = ""
@export var dependencies:Dictionary = {}
@export var nodes:PackedScene
@export var tracks:Array[MaszynaTrackData] = []
@export var traction:Array[MaszynaTractionData] = []
@export var power_sources:Array[MaszynaPowerSourceData] = []
@export var models:Array[MaszynaModelData] = []
@export var events:Array[MaszynaEventData] = []
@export var memcells:Array[MaszynaMemcellData] = []
@export var launchers:Array[MaszynaEventLauncherData] = []
@export var sounds:Array[MaszynaSoundData] = []
@export var isolated_sections:Array[MaszynaIsolatedData] = []
## The trainsets and their vehicles, built through the servers (not nodes, so not in [member nodes])
@export var trainsets:Array[MaszynaTrainsetData] = []
## The `lua` scripts, relative to the scenery directory
@export var scripts:Array[String] = []
## The region files (.sbt) whose terrain is supplied as the camera comes near
## (MaszynaLegacySBTTerrainProvider), not read into triangle_chunks
@export var region_files:Array[String] = []
## Merged triangle meshes, streamed by MaszynaSceneryChunkRenderingServer (not nodes, so not in [member nodes])
@export var triangle_chunks:Array[MaszynaTrianglesChunkData] = []
