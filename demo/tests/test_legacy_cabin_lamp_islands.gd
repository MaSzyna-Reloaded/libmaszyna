extends MaszynaGutTest

## LegacyCabinLampIslands.submodel_color() - the colour a lit lamp is drawn with in the original:
## its texture times its diffuse (light_common.glsl:164), never the diffuse alone

## ST45 kabina-st45-a.e3d: czuwak_on / shp_on carry this warm diffuse over a blue texture
const ST45_LAMP_DIFFUSE: Color = Color(1.0, 0.804, 0.576)
const BLUE: Color = Color(0.1, 0.2, 0.9)


func _lamp(p_texture: Image) -> MeshInstance3D:
    var material: ShaderMaterial = ShaderMaterial.new()
    material.set_shader_parameter("albedo", ST45_LAMP_DIFFUSE)
    material.set_shader_parameter("texture_albedo", ImageTexture.create_from_image(p_texture))
    var lamp: MeshInstance3D = MeshInstance3D.new()
    lamp.mesh = QuadMesh.new()
    lamp.material_override = material
    return add_child_autofree(lamp)


func test_blue_lamp_under_a_warm_diffuse_shines_blue():
    var texture: Image = Image.create_empty(4, 4, false, Image.FORMAT_RGBA8)
    texture.fill(BLUE)
    var color: Color = LegacyCabinLampIslands.submodel_color(_lamp(texture))
    var expected: Color = BLUE * ST45_LAMP_DIFFUSE
    expected = expected / expected.b
    assert_almost_eq(color.r, expected.r, 0.01)
    assert_almost_eq(color.g, expected.g, 0.01)
    assert_almost_eq(color.b, 1.0, 0.0001, "blue is the brightest channel")


func test_texture_without_colour_channels_shines_with_the_diffuse():
    # SM42's lamps are two-channel BC5: the texture says nothing of the colour
    var texture: Image = Image.create_empty(4, 4, false, Image.FORMAT_RG8)
    texture.fill(BLUE)
    var color: Color = LegacyCabinLampIslands.submodel_color(_lamp(texture))
    assert_almost_eq(color.r, 1.0, 0.0001)
    assert_almost_eq(color.g, ST45_LAMP_DIFFUSE.g, 0.0001)
    assert_almost_eq(color.b, ST45_LAMP_DIFFUSE.b, 0.0001)
