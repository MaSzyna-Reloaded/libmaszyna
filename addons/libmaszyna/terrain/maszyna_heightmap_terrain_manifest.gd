extends RefCounted
class_name MaszynaHeightmapTerrainManifest

## The description of a heightmap terrain, terrain/<name>/terrain.yaml (heightmap::manifest,
## load_manifest(), scene/heightmapterrain.cpp:547): the point spacing of its chunks and the palette
## of the materials painted over them, the first covering a chunk of no paint. The file is the YAML
## the original writes (save_manifest()); what the game's terrains hold of it is read - its keys of
## one value and its palette of material names.

const FILE_NAME:String = "terrain.yaml"
## heightmap::spacings, heightmap::default_spacing [m]
const SPACINGS:Array[float] = [0.5, 1.0, 2.0, 5.0]
const DEFAULT_SPACING:float = 2.0
## The palette of a manifest that names none (heightmap::manifest)
const DEFAULT_LAYER:String = "grass"
const LAYERS_KEY:String = "layers"
const SEQUENCE_ITEM:String = "- "
const COMMENT:String = "#"

var spacing:float = DEFAULT_SPACING
## The materials of the palette, in its order
var layers:Array[String] = []


## The manifest of the terrain in p_directory; null when there is none
static func read(directory:String) -> MaszynaHeightmapTerrainManifest:
    var file:FileAccess = FileAccess.open(directory.path_join(FILE_NAME), FileAccess.READ)
    if not file:
        return null
    var manifest := MaszynaHeightmapTerrainManifest.new()
    var in_layers:bool = false
    for raw_line:String in file.get_as_text().split("\n"):
        var line:String = raw_line.strip_edges(false, true)
        if not line.strip_edges() or line.strip_edges().begins_with(COMMENT):
            continue
        var indented:bool = line.begins_with(" ")
        line = line.strip_edges()
        if indented and in_layers and line.begins_with(SEQUENCE_ITEM):
            manifest.layers.append(_scalar(line.trim_prefix(SEQUENCE_ITEM)))
            continue
        if indented:
            continue
        var colon:int = line.find(":")
        if colon < 0:
            continue
        var key:String = line.left(colon).strip_edges()
        in_layers = key == LAYERS_KEY
        if key == "spacing":
            manifest.spacing = _valid_spacing(float(_scalar(line.substr(colon + 1))))
    if not manifest.layers:
        manifest.layers.append(DEFAULT_LAYER)
    return manifest


## A plain or quoted scalar, as yaml-cpp gives it
static func _scalar(text:String) -> String:
    var value:String = text.strip_edges()
    if value.length() >= 2 and value[0] in ["\"", "'"] and value[-1] == value[0]:
        return value.substr(1, value.length() - 2)
    return value


## heightmap::valid_spacing(): the allowed spacing nearest to p_spacing
static func _valid_spacing(value:float) -> float:
    var best:float = DEFAULT_SPACING
    var distance:float = INF
    for allowed:float in SPACINGS:
        if absf(allowed - value) < distance:
            distance = absf(allowed - value)
            best = allowed
    return best
