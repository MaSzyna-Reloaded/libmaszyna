extends RefCounted
class_name MaszynaMeshPlacement

## A mesh a SceneryStreamingProvider places (SceneryStreamingProvider.CONTENT_MESHES) - a piece of
## a world's sector geometry (MaszynaMeshesNode) supplied by MaszynaWorldSectorProvider. Drawn by
## SceneryMeshRenderingServer as long as its cell is supplied: the mesh goes with it.

var mesh:ArrayMesh
## Where the mesh is placed in the world: its sector's corner
var transform:Transform3D = Transform3D.IDENTITY
## How far it is drawn from [m]; 0: the draw distance (MaszynaMeshesNode.range_end)
var range_end:float = 0.0


func _init(placed_mesh:ArrayMesh = null, placed_transform:Transform3D = Transform3D.IDENTITY, placed_range_end:float = 0.0) -> void:
    mesh = placed_mesh
    transform = placed_transform
    range_end = placed_range_end


## The placement drawn in `scenario` - the consumer of CONTENT_MESHES (SceneryInstancer): a surface
## named after the game's material (MaterialManager) gets that material, one with its own (an
## orthophoto, a facade) keeps it. Its RID, for SceneryMeshRenderingServer.mesh_free()
static func create_mesh(placement:MaszynaMeshPlacement, scenario:RID) -> RID:
    for surface:int in placement.mesh.get_surface_count():
        if not placement.mesh.surface_get_material(surface):
            placement.mesh.surface_set_material(surface,
                    MaterialManager.get_material("", placement.mesh.surface_get_name(surface)))
    return SceneryMeshRenderingServer.mesh_create(placement.mesh, placement.transform, placement.range_end, scenario)
