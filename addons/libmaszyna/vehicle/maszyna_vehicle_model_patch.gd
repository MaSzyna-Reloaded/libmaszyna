@tool
extends Resource
class_name MaszynaVehicleModelPatch

## The patch of one file of a vehicle (MaszynaVehiclePatch): the elements its readers add to it
## where it defines none - MaszynaRailVehicle3DInstancer of a vehicle's model, MmdCabinInstancer of a
## cab's model, FizVehicleBuilder of a FIZ.

## The file, by its path in the game's directory: "dynamic/pkp/sm42_v1/6da.e3d" - a model's .e3d
## and .t3d alike - or "dynamic/pkp/6dg_v1/6dg.fiz"
@export var path:String = ""
## What is added to it
@export var elements:Array[MaszynaVehicleModelElementPatch] = []


## Its elements of a kind (a MaszynaVehicleModelElementPatch's class), in their order
func elements_of(kind:Script) -> Array[MaszynaVehicleModelElementPatch]:
    var found:Array[MaszynaVehicleModelElementPatch] = []
    for element:MaszynaVehicleModelElementPatch in elements:
        if is_instance_of(element, kind):
            found.append(element)
    return found


## What tells this patch from another, for a cache to keep with what it read of the file
func digest() -> String:
    # an object's text is every property of it
    return var_to_str([path, elements]).md5_text()


## The submodel of `submodels` (a model's) at a path of its tree, its names in any case:
## "baza/ostoja/wycieraczka01"; null of none
static func find_submodel(submodels:Array, path:String) -> MaszynaE3DSubModel:
    var found:MaszynaE3DSubModel = null
    for name:String in path.split("/"):
        var below:Array = found.submodels if found else submodels
        found = null
        for item:Variant in below:
            var submodel:MaszynaE3DSubModel = item as MaszynaE3DSubModel
            if submodel and submodel.resource_name.to_lower() == name.to_lower():
                found = submodel
                break
        if not found:
            return null
    return found
