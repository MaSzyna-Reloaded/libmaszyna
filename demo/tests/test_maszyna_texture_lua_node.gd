extends MaszynaGutTest

## A texture drawn in Lua under a name of the data (MaszynaTextureLuaNode): a material of that name
## shows it - one made before the node too - and its own texture again once the node is gone

const TEXTURE_NAME:String = "Lua_Sign"
const MODEL_PATH:String = "dynamic/test"


func _node() -> MaszynaTextureLuaNode:
    var node:MaszynaTextureLuaNode = MaszynaTextureLuaNode.new()
    node.texture = DrawingTexture.new()
    node.texture_name = TEXTURE_NAME
    node.source = "local drawing = ...\nreloaded.draw.clear(drawing, {255, 170, 0})"
    return node


func test_a_material_of_the_name_shows_the_drawn_texture() -> void:
    var node:MaszynaTextureLuaNode = _node()
    add_child_autofree(node)

    var material:ShaderMaterial = MaszynaMaterialManager.get_material(MODEL_PATH, TEXTURE_NAME.to_lower())
    assert_eq(MaszynaMaterialManager.material_get_texture(material), node.texture, "the name in any case")


func test_a_material_made_before_the_node_shows_it_and_its_own_again_after() -> void:
    var material:ShaderMaterial = MaszynaMaterialManager.get_material(MODEL_PATH, TEXTURE_NAME)
    var own:Texture2D = MaszynaMaterialManager.material_get_texture(material)
    var node:MaszynaTextureLuaNode = _node()

    add_child(node)
    assert_eq(MaszynaMaterialManager.material_get_texture(material), node.texture)
    remove_child(node)
    node.free()
    assert_eq(MaszynaMaterialManager.material_get_texture(material), own)
