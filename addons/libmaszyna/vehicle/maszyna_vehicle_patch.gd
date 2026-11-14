@tool
extends Resource
class_name MaszynaVehiclePatch

## The game's patch of MaSzyna's vehicles: what their files do not define but the game needs - the
## wiper arms of a model whose MMD names none, a cab's controls its MMD lacks, the wiper switch of a
## FIZ of no WiperList. A MaszynaVehicleModelPatch for each file patched, read by the reader of the
## file only where the file has no definition of its own (MASZYNA_ORIGINAL_QUIRKS.md); the data
## stays as it is. The game hands it to MaszynaVehicleSystem.patch_set().

## Every file patched
@export var models:Array[MaszynaVehicleModelPatch] = []
