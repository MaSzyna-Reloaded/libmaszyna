@tool
extends MaszynaVehicleModelElementPatch
class_name MaszynaMaterialPatch

## The material of a vehicle's texture that has no .mat of its own (an element of the
## MaszynaVehicleModelPatch of the .mat the original would read, "dynamic/pkp/sm42_v1/okno_czolo_dol.mat"),
## as a .mat defines one (MaszynaMaterialParser): the SM42's cab glass drawn with the rain on it.

## The .mat's shader: - "rain_windscreen"
@export var shader:String = ""
## The .mat's textures by their slots, without "texture_": one of the data by its path in the game's
## directory ("raindropsatlas": "textures/fx/raindrops-atlas"), one of the patch's own by its
## res:// path (a "wipermask" made from the model's wipers) - the diffuse one, when not given, the
## material's own texture
@export var textures:Dictionary[String, String] = {}
## The .mat's parameters, without "param_" ("raindrop_grid_size": 150)
@export var parameters:Dictionary[String, float] = {}
## Its parameters of four values, without "param_" - the middle of wiper N's pane of a glass whose
## panes share their UV ("wiper_pane_0": its x, y, z in the mesh's own frame and 1, rain_windscreen)
@export var vector_parameters:Dictionary[String, Vector4] = {}
