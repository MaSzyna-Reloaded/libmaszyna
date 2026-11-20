extends Object
class_name MaszynaHeightmapTerrainLooks

## What a heightmap terrain is drawn with (MaszynaHeightmapTerrainLook), made of the materials of its
## palette as the original makes it (opengl33_terrain_materials, opengl33terrain.cpp): each layer's
## diffuse texture, normal map and specular/gloss map - of the season and the weather of now - copied
## into three texture arrays of one size each, and what the terrain's shader needs of each layer.

const TERRAIN_SHADER:Shader = preload("heightmap_terrain.gdshader")
## The sides of the arrays: diffuse, normal maps, specular/gloss maps (opengl33terrain.cpp:26)
const DIFFUSE_SIZE:int = 1024
const NORMAL_SIZE:int = 512
const SPECGLOSS_SIZE:int = 256
## What a layer without a map of a kind takes: grey, flat, without specular reflections
## (opengl33_terrain_materials::copy_layer())
const DEFAULT_DIFFUSE:Color = Color(0.5, 0.5, 0.5, 1.0)
const DEFAULT_NORMAL:Color = Color(0.5, 0.5, 1.0, 1.0)
const DEFAULT_SPECGLOSS:Color = Color(0.0, 1.0, 0.0, 1.0)
## Metres a layer's textures repeat at when its material gives no size (opengl33terrain.cpp:23)
const DEFAULT_REPEAT:float = 8.0
## A material's glossiness when it names none (MaszynaMaterialFactory)
const DEFAULT_GLOSSINESS:float = 10.0


## The look of the terrain the manifest describes; made on the main thread, as the materials are
static func make(manifest:MaszynaHeightmapTerrainManifest) -> MaszynaHeightmapTerrainLook:
    var diffuse:Array[Image] = []
    var normal:Array[Image] = []
    var specgloss:Array[Image] = []
    var palette:PackedFloat32Array = []
    for index:int in manifest.layers.size():
        var material:MaszynaMaterial = MaszynaMaterialManager.load_material("", manifest.layers[index])
        var variant:MaszynaMaterial.MaszynaMaterialVariant = material.get_variant(
            MaszynaMaterialManager.season, MaszynaMaterialManager.weather)
        diffuse.append(_layer_image(variant.get_texture_path("diffuse"), DIFFUSE_SIZE, DEFAULT_DIFFUSE))
        var normal_path:String = variant.get_texture_path("normalmap")
        normal.append(_layer_image(normal_path, NORMAL_SIZE, DEFAULT_NORMAL))
        var specgloss_path:String = variant.get_texture_path("specgloss")
        specgloss.append(_layer_image(specgloss_path, SPECGLOSS_SIZE, DEFAULT_SPECGLOSS))
        var repeat:float = material.size.x if material.size.x > 0.0 else DEFAULT_REPEAT
        palette.append_array([
            float(index), 1.0 / repeat, 1.0 if normal_path else 0.0, 1.0 if specgloss_path else 0.0,
            variant.get_parameter("reflection", 0.0), 1.0, 0.0,
            variant.get_parameter("glossiness", DEFAULT_GLOSSINESS),
        ])
    var look := MaszynaHeightmapTerrainLook.new()
    look.shader = TERRAIN_SHADER
    look.diffuse_layers = _array(diffuse)
    look.normal_layers = _array(normal)
    look.specgloss_layers = _array(specgloss)
    look.palette = palette
    return look


## A layer's map as its array holds it: the texture of the path at the array's side, with its
## mipmaps; a missing one the default of its kind
static func _layer_image(path:String, side:int, default:Color) -> Image:
    var texture:Texture2D = MaszynaMaterialManager.load_texture("", path, false, side) if path else null
    var image:Image = texture.get_image() if texture and not texture == MaszynaMaterialManager.UNKNOWN_TEXTURE else null
    if not image:
        image = Image.create_empty(side, side, false, Image.FORMAT_RGBA8)
        image.fill(default)
    if image.is_compressed():
        image.decompress()
    image.clear_mipmaps()
    image.convert(Image.FORMAT_RGBA8)
    image.resize(side, side, Image.INTERPOLATE_BILINEAR)
    image.generate_mipmaps()
    return image


static func _array(images:Array[Image]) -> Texture2DArray:
    var array := Texture2DArray.new()
    array.create_from_images(images)
    return array
