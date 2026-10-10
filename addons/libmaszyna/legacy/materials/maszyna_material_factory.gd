@tool
extends Node

## Texture slots of a shader, in the order of the "#texture (name, index)" declarations of its
## original mat_*.frag: a numbered "textureN:" key of a .mat binds slot N-1 (material.cpp:76-81),
## so which texture it is depends on the shader. A texture outside of the slots is not read.
class TextureMap:
    var albedo: String = "diffuse"
    var normalmap: String = "normalmap"
    var detail_normalmap: String = "detailnormalmap"
    var dudvmap: String = "dudvmap"
    var specgloss: String = "specgloss"
    var reflmap: String = "reflmap"
    var raindropsatlas: String = "raindropsatlas"
    var wipermask: String = "wipermask"
    var slots: Array[String]

    func _init(slots: Array[String]):
        self.slots = slots

class MaszynaShaderMeta:
    var base_material: Material
    var factory: Callable
    var texture_map: TextureMap

    func _init(factory: Callable, base_material: Material, texture_map: TextureMap) -> void:
        self.base_material = base_material
        self.factory = factory
        self.texture_map = texture_map

# Named texture keys of a material without "shader:" (texture_bindings, material.cpp:60-65)
const LEGACY_TEXTURE_BINDINGS: Dictionary[int, String] = {
    0: "diffuse",
    1: "normalmap",
}

var DEFAULT_SHADER:MaszynaShaderMeta = MaszynaShaderMeta.new(
    _apply_default_material,
    preload("res://addons/libmaszyna/legacy/materials/types/default.tres"),
    TextureMap.new(["diffuse", "normalmap"]),
)

# mat_colored.frag - untextured, the color comes from "param_color:" or the submodel
var COLORED_SHADER:MaszynaShaderMeta = MaszynaShaderMeta.new(
    _apply_colored,
    preload("res://addons/libmaszyna/legacy/materials/types/default.tres"),
    TextureMap.new([]),
)

# mat_reflmap.frag - the alpha of the reflmap texture scales the reflection
var REFLMAP_SHADER:MaszynaShaderMeta = MaszynaShaderMeta.new(
    _apply_reflmap,
    preload("./types/reflmap.tres"),
    TextureMap.new(["diffuse", "reflmap"]),
)

var MATERIAL_SHADER_FACTORIES: Dictionary[String, MaszynaShaderMeta] = {
    # mat_default.frag - the plain textured material
    "default": DEFAULT_SHADER,
    # mat_default_0/1/2.frag only include mat_colored, mat_default and mat_reflmap
    "default_0": COLORED_SHADER,
    "default_1": DEFAULT_SHADER,
    "default_2": REFLMAP_SHADER,
    "colored": COLORED_SHADER,
    "reflmap": REFLMAP_SHADER,
    "reflmap_specgloss": MaszynaShaderMeta.new(
        _apply_reflmap,
        preload("./types/reflmap_specgloss.tres"),
        TextureMap.new(["diffuse", "reflmap", "specgloss"]),
    ),
    # mat_default_detail.frag: the detail normal map alone, there is no base normal map
    "default_detail": MaszynaShaderMeta.new(
        _apply_detail_normalmap,
        preload("./types/detail_normalmap.tres"),
        TextureMap.new(["diffuse", "detailnormalmap"]),
    ),
    "detail_normalmap": MaszynaShaderMeta.new(
        _apply_detail_normalmap,
        preload("./types/detail_normalmap.tres"),
        TextureMap.new(["diffuse", "normalmap", "detailnormalmap"]),
    ),
    "shadowlessnormalmap": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/shadowlessnormalmap.tres"),
        TextureMap.new(["diffuse", "normalmap"]),
    ),
    "sunlessnormalmap": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/sunlessnormalmap.tres"),
        TextureMap.new(["diffuse", "normalmap"]),
    ),
    "normalmap": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/normalmap.tres"),
        TextureMap.new(["diffuse", "normalmap"]),
    ),
    "normalmap_specgloss": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/normalmap_specgloss.tres"),
        TextureMap.new(["diffuse", "normalmap", "specgloss"]),
    ),
    # The *_specgloss types below take specular/gloss/metal from the specgloss texture as in the
    # original: their plain shader built with MASZYNA_SPECGLOSS (types/specgloss.gdshaderinc).
    "default_specgloss": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/normalmap_specgloss.tres"),
        TextureMap.new(["diffuse", "specgloss"]),
    ),
    "detail_normalmap_specgloss": MaszynaShaderMeta.new(
        _apply_detail_normalmap,
        preload("./types/detail_normalmap.tres"),
        TextureMap.new(["diffuse", "normalmap", "detailnormalmap", "specgloss"]),
    ),
    "shadowlessnormalmap_specgloss": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/shadowlessnormalmap.tres"),
        TextureMap.new(["diffuse", "normalmap", "specgloss"]),
    ),
    "sunlessnormalmap_specgloss": MaszynaShaderMeta.new(
        _apply_default_material,
        preload("./types/sunlessnormalmap.tres"),
        TextureMap.new(["diffuse", "normalmap", "specgloss"]),
    ),
    "parallax": MaszynaShaderMeta.new(
        _apply_parallax,
        preload("./types/parallax.tres"),
        TextureMap.new(["diffuse", "normalmap"]),
    ),
    "detail_parallax": MaszynaShaderMeta.new(
        _apply_parallax,
        preload("./types/parallax.tres"),
        TextureMap.new(["diffuse", "normalmap", "detailnormalmap"]),
    ),
    "parallax_specgloss": MaszynaShaderMeta.new(
        _apply_parallax,
        preload("./types/parallax_specgloss.tres"),
        TextureMap.new(["diffuse", "normalmap", "specgloss"]),
    ),
    "detail_parallax_specgloss": MaszynaShaderMeta.new(
        _apply_parallax,
        preload("./types/parallax_specgloss.tres"),
        TextureMap.new(["diffuse", "normalmap", "specgloss", "detailnormalmap"]),
    ),
    "water": MaszynaShaderMeta.new(
        _apply_water,
        preload("./types/water.tres"),
        TextureMap.new(["normalmap", "dudvmap", "diffuse"]),
    ),
    "water_specgloss": MaszynaShaderMeta.new(
        _apply_water,
        preload("./types/water_specgloss.tres"),
        TextureMap.new(["normalmap", "dudvmap", "diffuse", "specgloss"]),
    ),
    "rain_windscreen": MaszynaShaderMeta.new(
        _apply_rain_windscreen,
        preload("./types/rain_windscreen.tres"),
        TextureMap.new(["diffuse", "raindropsatlas", "wipermask"]),
    ),
}

## Shader variants by "source shader instance id:alpha blend:cull disabled:specgloss"
var _shader_variants: Dictionary[String, Shader] = {}
## Turns on the specgloss texture (types/specgloss.gdshaderinc) in a type shader that offers it
const SPECGLOSS_DEFINE: String = "MASZYNA_SPECGLOSS"


func create(
    mmat: MaszynaMaterial,
    model_path: String = "",
    season: MaszynaEnvironment.Season = MaszynaEnvironment.Season.SEASON_SUMMER,
    weather: MaszynaEnvironment.Weather = MaszynaEnvironment.Weather.WEATHER_CLEAR,
    options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new(),
) -> Material:
    var variant: MaszynaMaterial.MaszynaMaterialVariant = mmat.get_variant(season, weather)
    var shader_meta:MaszynaShaderMeta = _get_shader_meta(variant)
    var material: Material = shader_meta.base_material.duplicate(true)
    _apply(
        material,
        mmat,
        variant,
        shader_meta,
        shader_meta.texture_map,
        model_path,
        options,
    )
    return material


func apply(
    material: Material,
    mmat: MaszynaMaterial,
    model_path: String,
    season: MaszynaEnvironment.Season,
    weather: MaszynaEnvironment.Weather,
    options: MaterialManager.MaterialOptions = MaterialManager.MaterialOptions.new(),
) -> void:
    var variant: MaszynaMaterial.MaszynaMaterialVariant = mmat.get_variant(season, weather)
    var shader_meta:MaszynaShaderMeta = _get_shader_meta(variant)
    _apply(
        material,
        mmat,
        variant,
        shader_meta,
        shader_meta.texture_map,
        model_path,
        options,
    )


func _get_shader_meta(variant: MaszynaMaterial.MaszynaMaterialVariant) -> MaszynaShaderMeta:
    if not variant.shader:
        # No "shader:" - picked by the bound textures (material.cpp:117-134); the second texture of
        # such a material is a reflection map, not a normal map.
        if not _texture_path(variant, REFLMAP_SHADER.texture_map, "diffuse"):
            return COLORED_SHADER
        if not _texture_path(variant, REFLMAP_SHADER.texture_map, "reflmap"):
            return DEFAULT_SHADER
        return REFLMAP_SHADER
    if not variant.shader in MATERIAL_SHADER_FACTORIES:
        push_warning("Shader is not supported: " + variant.shader)
    return MATERIAL_SHADER_FACTORIES.get(variant.shader, DEFAULT_SHADER)


## Path of the texture bound to a slot of the shader: by its name ("texture_<slot>:"), else by its
## number ("texture<index + 1>:"), as in opengl_material::finalize() (material.cpp:67-108).
func _texture_path(
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    texture_map: TextureMap,
    slot: String,
) -> String:
    var index: int = texture_map.slots.find(slot)
    if index < 0:
        return ""
    var path: String = variant.get_texture_path(slot)
    if not path:
        path = variant.get_texture_path("tex%d" % (index + 1))
    if not path and not variant.shader:
        path = variant.get_texture_path(LEGACY_TEXTURE_BINDINGS.get(index, ""))
    return path


func _apply(
    material: Material,
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    shader_meta: MaszynaShaderMeta,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:

    if not material is ShaderMaterial or not shader_meta.base_material is ShaderMaterial:
        return
    var target_shader_material: ShaderMaterial = material as ShaderMaterial
    var source_shader_material: ShaderMaterial = shader_meta.base_material as ShaderMaterial
    # a type shader that offers the specgloss texture as a variant (parallax_specgloss and
    # water_specgloss read it unconditionally) gets it only when the material binds one
    var specgloss: bool = (
        source_shader_material.shader.code.contains(SPECGLOSS_DEFINE)
        and not _texture_path(variant, texture_map, texture_map.specgloss) == ""
    )
    for property: Dictionary in source_shader_material.get_property_list():
        var property_name: String = property.get("name", "")
        if property_name == "shader":
            target_shader_material.shader = (
                _get_shader_variant(source_shader_material.shader, options, specgloss)
                if options.force_transparent or options.cull_disabled or specgloss
                else source_shader_material.shader
            )
        elif property_name == "render_priority" or property_name.begins_with("shader_parameter/"):
            target_shader_material.set(property_name, source_shader_material.get(property_name))

    shader_meta.factory.call(mmat, variant, material, texture_map, model_path, options)
    # The original's opaque pass keeps the texels at the material's "opacity:" and above
    # (opengl33renderer.cpp:2247-2258): a material that gives one is cut out there - a vehicle's
    # skin with its glass painted in, which a translucent submodel then draws
    var has_opacity: bool = mmat.opacity > 0.0 and mmat.opacity < 1.0
    var transparency: MaterialManager.Transparency = MaterialManager.Transparency.Disabled
    if options.force_transparent:
        transparency = MaterialManager.Transparency.Alpha
    elif (mmat.transparent or has_opacity) and not options.force_opaque:
        transparency = MaterialManager.Transparency.AlphaScissor
    target_shader_material.set_shader_parameter("transparency", transparency)
    target_shader_material.set_shader_parameter(
        "alpha_scissor_threshold", mmat.opacity if has_opacity else options.alpha_scissor_threshold)
    target_shader_material.set_shader_parameter("emission_enabled", options.selfillum_enabled)
    target_shader_material.set_shader_parameter("emission_color", options.selfillum_color if options.selfillum_color else Color(1.0, 1.0, 1.0, 1.0))
    target_shader_material.set_shader_parameter("emission_energy", options.selfillum_energy)
    if options.force_transparent:
        # mat_default has no reflection unless its .mat gives one; the shaders with a normal map
        # or a reflmap reflect in full ("#param (reflection, 1, 2, 1, zero)" against "one")
        var masked: bool = (
            not _texture_path(variant, texture_map, texture_map.normalmap) == ""
            or not _texture_path(variant, texture_map, texture_map.reflmap) == "")
        var default_reflection: float = 1.0 if masked else 0.0
        target_shader_material.set_shader_parameter(
            "blended_reflection", variant.get_parameter("reflection", default_reflection))


func _get_shader_variant(source_shader: Shader, options: MaterialManager.MaterialOptions, specgloss: bool) -> Shader:
    var cache_key: String = "%d:%s:%s:%s" % [
        source_shader.get_instance_id(), options.force_transparent, options.cull_disabled, specgloss
    ]
    var cached_shader: Shader = _shader_variants.get(cache_key)
    if cached_shader:
        return cached_shader

    var code: String = source_shader.code
    if options.force_transparent:
        code = code.replace("shader_type spatial;", "shader_type spatial;\n#define MASZYNA_ALPHA_BLEND")
        # a blended surface mirrors the sky (types/blended_reflection.gdshaderinc), which
        # specular_disabled would take away with the lights' highlights
        code = code.replace("specular_disabled", "specular_schlick_ggx")
    if options.cull_disabled:
        code = code.replace("cull_back", "cull_disabled")
    if specgloss:
        # the specular term the texture scales has to exist (default, detail_normalmap disable it)
        code = code.replace("shader_type spatial;", "shader_type spatial;\n#define %s" % SPECGLOSS_DEFINE)
        code = code.replace("specular_disabled", "specular_schlick_ggx")
    var variant_shader: Shader = Shader.new()
    variant_shader.code = code
    _shader_variants[cache_key] = variant_shader
    return variant_shader


func _apply_default_material(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    var diffuse_texture: String = _texture_path(variant, texture_map, texture_map.albedo)
    var normalmap_texture: String = _texture_path(variant, texture_map, texture_map.normalmap)

    # albedo defaults to this submodel's own parsed E3D diffuse color (e.g. a lamp lens'
    # green/yellow tint over a neutral texture) - a .mat variant's own "diffuse:" override, when
    # present, takes precedence, matching the untextured branch below. Previously this was only
    # ever set for untextured submodels, leaving every textured one (including colored indicator
    # lamps like EP07's wylszybki_on/off, confirmed real: diffuse (0, 0.749, 0) over texture
    # "kran_zasadniczy") stuck at the shader's default white regardless of the model's own color.
    material.set_shader_parameter("albedo", options.diffuse_color)
    if diffuse_texture:
        var albedo_texture: Texture = MaterialManager.load_texture(model_path, diffuse_texture, false, options.max_texture_size)
        if albedo_texture is Texture2D and _texture_has_alpha(albedo_texture):
            mmat.transparent = true
        material.set_shader_parameter("texture_albedo", albedo_texture)
        if variant.has_parameter("diffuse"):
            material.set_shader_parameter("albedo", Color(
                variant.get_parameter("diffuse"),
                variant.get_parameter("diffuse"),
                variant.get_parameter("diffuse"),
                1.0
            ))

    if normalmap_texture:
        # normal_scale stays at the 1.0 of the material types: the original applies the normal map
        # as it is (mat_normalmap.frag:46-48), and a factor here also amplifies the DXT
        # quantisation bias of the lower mips into a fixed tilt of the whole distant surface
        material.set_shader_parameter("texture_normal", MaterialManager.load_texture(model_path, normalmap_texture, true, options.max_texture_size))

    if variant.has_parameter("specular"):
        material.set_shader_parameter("specular", variant.get_parameter("specular"))

    if variant.has_parameter("glossiness"):
        material.set_shader_parameter("roughness", _roughness_from_glossiness(variant.get_parameter("glossiness")))

    if variant.has_parameter("reflection"):
        material.set_shader_parameter("metallic", variant.get_parameter("reflection"))

    # the specgloss variant (see _apply()) reads the raw params the way parallax_specgloss does
    var specgloss_texture: String = _texture_path(variant, texture_map, texture_map.specgloss)
    if specgloss_texture:
        material.set_shader_parameter(
            "specgloss_texture", MaterialManager.load_texture(model_path, specgloss_texture, true, options.max_texture_size))
        material.set_shader_parameter("specular_strength", variant.get_parameter("specular", 0.5))
        material.set_shader_parameter("reflection_strength", variant.get_parameter("reflection", 0.0))
        material.set_shader_parameter("glossiness", variant.get_parameter("glossiness", 10.0))
    material.set_shader_parameter("emission_enabled", options.selfillum_enabled)
    material.set_shader_parameter("emission_color", options.selfillum_color if options.selfillum_color else Color(1.0, 1.0, 1.0, 1.0))
    material.set_shader_parameter("emission_energy", options.selfillum_energy)

## mat_colored.frag: no texture, the color is "param_color:" (the submodel's own diffuse without it)
func _apply_colored(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    _apply_default_material(mmat, variant, material, texture_map, model_path, options)
    if variant.has_parameter_vec4("color"):
        var color: Vector4 = variant.get_parameter_vec4("color")
        material.set_shader_parameter("albedo", Color(color.x, color.y, color.z, color.w))


## mat_reflmap.frag:47 - reflectivity = param_reflection * reflmap.a, param_reflection defaults to
## one (the 1.0 "metallic" of reflmap.tres); the alpha channel is picked by the material type.
func _apply_reflmap(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    _apply_default_material(mmat, variant, material, texture_map, model_path, options)
    var reflmap_texture: String = _texture_path(variant, texture_map, texture_map.reflmap)
    if reflmap_texture:
        material.set_shader_parameter(
            "texture_metallic", MaterialManager.load_texture(model_path, reflmap_texture, true, options.max_texture_size))


## mat_detail_normalmap.frag: the default material plus a tiled detail normal map
## (texture_detailnormalmap, param_detail_scale, param_detail_height_scale - both default to 1.0).
func _apply_detail_normalmap(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    _apply_default_material(mmat, variant, material, texture_map, model_path, options)
    var detail_normalmap_texture: String = _texture_path(variant, texture_map, texture_map.detail_normalmap)
    if detail_normalmap_texture:
        material.set_shader_parameter(
            "texture_detail_normal", MaterialManager.load_texture(model_path, detail_normalmap_texture, true, options.max_texture_size))
    material.set_shader_parameter("detail_scale", variant.get_parameter("detail_scale", 1.0))
    material.set_shader_parameter("detail_height_scale", variant.get_parameter("detail_height_scale", 1.0))


func _apply_parallax(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    var diffuse_texture_path: String = _texture_path(variant, texture_map, texture_map.albedo)
    var normalmap_texture_path: String = _texture_path(variant, texture_map, texture_map.normalmap)
    var detail_normalmap_texture_path: String = _texture_path(variant, texture_map, texture_map.detail_normalmap)

    var albedo_texture:Texture = MaterialManager.UNKNOWN_TEXTURE
    if diffuse_texture_path:
        albedo_texture = MaterialManager.load_texture(model_path, diffuse_texture_path, false, options.max_texture_size)
        if albedo_texture is Texture2D and _texture_has_alpha(albedo_texture):
            mmat.transparent = true

    var normal_texture:Texture = MaterialManager.UNKNOWN_TEXTURE
    if normalmap_texture_path:
        normal_texture = MaterialManager.load_texture(model_path, normalmap_texture_path, true, options.max_texture_size)

    var detail_normal_texture:Texture = MaterialManager.UNKNOWN_TEXTURE
    var use_detail_normal:bool = false
    if detail_normalmap_texture_path:
        detail_normal_texture = MaterialManager.load_texture(model_path, detail_normalmap_texture_path, true, options.max_texture_size)
        use_detail_normal = true

    var albedo_multiplier:Color = Color(1.0, 1.0, 1.0, 1.0)
    if not diffuse_texture_path:
        albedo_multiplier = options.diffuse_color

    if variant.has_parameter("diffuse"):
        albedo_multiplier = Color(
            variant.get_parameter("diffuse"),
            variant.get_parameter("diffuse"),
            variant.get_parameter("diffuse"),
            1.0
        )

    var alpha_scissor_threshold:float = 0.5
    var use_alpha_scissor := false

    if mmat.transparent:
        alpha_scissor_threshold = 0.70
        use_alpha_scissor = true

    material.set_shader_parameter("diffuse_texture", albedo_texture)
    material.set_shader_parameter("normal_texture", normal_texture)
    material.set_shader_parameter("detail_normal_texture", detail_normal_texture)
    material.set_shader_parameter("use_detail_normal", use_detail_normal)
    material.set_shader_parameter("albedo_multiplier", albedo_multiplier)
    material.set_shader_parameter("specular_strength", variant.get_parameter("specular", 0.5))
    material.set_shader_parameter("reflection_strength", variant.get_parameter("reflection", 0.0))
    material.set_shader_parameter("glossiness", variant.get_parameter("glossiness", 10.0))
    material.set_shader_parameter("detail_scale", variant.get_parameter("detail_scale", 1.0))
    material.set_shader_parameter("detail_height_scale", variant.get_parameter("detail_height_scale", 1.0))
    material.set_shader_parameter("height_scale", variant.get_parameter("height_scale", 0.0))
    material.set_shader_parameter("height_offset", variant.get_parameter("height_offset", 0.0))
    material.set_shader_parameter("texture_size", mmat.size)
    material.set_shader_parameter("use_alpha_scissor", use_alpha_scissor)
    material.set_shader_parameter("alpha_scissor_threshold", alpha_scissor_threshold)

    var specgloss_texture_path: String = _texture_path(variant, texture_map, texture_map.specgloss)
    if specgloss_texture_path:
        material.set_shader_parameter(
            "specgloss_texture", MaterialManager.load_texture(model_path, specgloss_texture_path, true, options.max_texture_size))

func _apply_water(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    var diffuse_texture_path: String = _texture_path(variant, texture_map, texture_map.albedo)
    var normalmap_texture_path: String = _texture_path(variant, texture_map, texture_map.normalmap)
    var dudvmap_texture_path: String = _texture_path(variant, texture_map, texture_map.dudvmap)
    var specgloss_texture_path: String = _texture_path(variant, texture_map, texture_map.specgloss)

    var diffuse_texture: Texture = MaterialManager.UNKNOWN_TEXTURE
    if diffuse_texture_path:
        diffuse_texture = MaterialManager.load_texture(model_path, diffuse_texture_path, false, options.max_texture_size)
        if diffuse_texture is Texture2D and _texture_has_alpha(diffuse_texture):
            mmat.transparent = true

    var normal_texture: Texture = MaterialManager.UNKNOWN_TEXTURE
    if normalmap_texture_path:
        normal_texture = MaterialManager.load_texture(model_path, normalmap_texture_path, true, options.max_texture_size)

    var dudv_texture: Texture = MaterialManager.UNKNOWN_TEXTURE
    if dudvmap_texture_path:
        dudv_texture = MaterialManager.load_texture(model_path, dudvmap_texture_path, false, options.max_texture_size)

    var material_color := Vector4(1.0, 1.0, 1.0, 0.0)
    if variant.has_parameter_vec4("color"):
        material_color = variant.get_parameter_vec4("color")

    material.set_shader_parameter("diffuse_texture", diffuse_texture)
    material.set_shader_parameter("normal_texture", normal_texture)
    material.set_shader_parameter("dudv_texture", dudv_texture)
    material.set_shader_parameter("material_color", Color(
        material_color.x,
        material_color.y,
        material_color.z,
        material_color.w
    ))
    material.set_shader_parameter("reflection_strength", variant.get_parameter("reflection", 0.0))
    material.set_shader_parameter("glossiness", variant.get_parameter("glossiness", 10.0))
    material.set_shader_parameter("wave_strength", variant.get_parameter("wave_strength", 0.05))
    material.set_shader_parameter("wave_speed", variant.get_parameter("wave_speed", 0.02))

    if variant.shader == "water_specgloss":
        var specgloss_texture: Texture = MaterialManager.UNKNOWN_TEXTURE
        if specgloss_texture_path:
            specgloss_texture = MaterialManager.load_texture(model_path, specgloss_texture_path, true, options.max_texture_size)
        material.set_shader_parameter("specgloss_texture", specgloss_texture)


## mat_rain_windscreen.frag: cab glass with raindrops wiped by the wipermask. Rain and wiper state
## come in as global shader uniforms.
func _apply_rain_windscreen(
    mmat: MaszynaMaterial,
    variant: MaszynaMaterial.MaszynaMaterialVariant,
    material: ShaderMaterial,
    texture_map: TextureMap,
    model_path: String,
    options: MaterialManager.MaterialOptions,
) -> void:
    var textures: Dictionary[String, String] = {
        "diffuse_texture": texture_map.albedo,
        "raindrops_atlas": texture_map.raindropsatlas,
        "wiper_mask": texture_map.wipermask,
    }
    for parameter: String in textures:
        var texture_path: String = _texture_path(variant, texture_map, textures[parameter])
        if texture_path:
            material.set_shader_parameter(parameter, MaterialManager.load_texture(model_path, texture_path, false, options.max_texture_size))
    material.set_shader_parameter("raindrop_grid_size", variant.get_parameter("raindrop_grid_size", 1.0))
    if variant.has_parameter("specular"):
        material.set_shader_parameter("specular", variant.get_parameter("specular"))
    if variant.has_parameter("glossiness"):
        material.set_shader_parameter("roughness", _roughness_from_glossiness(variant.get_parameter("glossiness")))


func _roughness_from_glossiness(glossiness: float) -> float:
    if glossiness == NAN:
        return 1.0

    var g := abs(glossiness)

    if g < 0.0001:
        return 1.0

    var max_glossiness := 3500.0
    var min_roughness := 0.18
    var max_roughness := 0.75

    var t := log(g + 1.0) / log(max_glossiness + 1.0)

    return lerp(max_roughness, min_roughness, clamp(t, 0.0, 1.0))

func _texture_has_alpha(texture:Texture2D) -> bool:
    var img:Image = texture.get_image()
    if not img:
        return false
    return not img.detect_alpha() == Image.ALPHA_NONE


func _apply_alpha_override_from_texture(
    material:BaseMaterial3D,
) -> MaterialManager.Transparency:
    var texture_alpha:bool = false
    if material.albedo_texture:
        texture_alpha = _texture_has_alpha(material.albedo_texture)

    if not texture_alpha:
        return MaterialManager.Transparency.Disabled

    if MaterialManager.use_alpha_transparency:
        return MaterialManager.Transparency.Alpha
    return MaterialManager.Transparency.AlphaScissor


func _apply_base_material_transparency(material:BaseMaterial3D, transparent:MaterialManager.Transparency) -> void:
    match transparent:
        MaterialManager.Transparency.AlphaScissor:
            material.transparency = StandardMaterial3D.TRANSPARENCY_ALPHA_SCISSOR
        MaterialManager.Transparency.Alpha:
            material.transparency = StandardMaterial3D.TRANSPARENCY_ALPHA_DEPTH_PRE_PASS
