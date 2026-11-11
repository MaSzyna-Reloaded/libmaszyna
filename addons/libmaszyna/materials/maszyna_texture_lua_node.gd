@tool
extends LuaVehicleTextureNode
class_name MaszynaTextureLuaNode

## A texture drawn in Lua under a name of the data (MaszynaMaterialManager.named_texture_add()): a
## skin, a material or a `make:` request naming it shows what the script draws, as it would a file
## of that name - a skin's slot 4, the destination sign, shows a train's number this way.
##
## MaszynaMaterialManager makes one for a `make:` request whose script has a `.lua` file beside it,
## named after the request: its script gets, after the drawing, the vehicle the request's
## `$timetable=` names - whose trainset's run `reloaded.station.run()` finds - and the request's
## other keys: `local drawing, vehicle, parameters = ...`. One put in a scene by hand draws a
## texture of its own name, its vehicle nil and its parameters as set. Its texture holds the drawing
## upside down (DrawingTexture.flip_y), as the data's textures are - the original turns a script's
## picture over (abstractscreenrenderer.py:69) and its models count their texture coordinates up
## from the bottom.

## A script of the data drawn in Lua - a `make:` texture's, a `pyscreen:` screen's - has a file of
## this extension beside its Python one
const EXTENSION:String = ".lua"

## The name the data gives the texture, as it names a texture's file
@export var texture_name:String = ""
## What the script is given after the vehicle - a `make:` request's keys, in lower case
@export var parameters:Dictionary = {}


func _enter_tree() -> void:
    MaszynaMaterialManager.named_texture_add(texture_name, texture)


func _exit_tree() -> void:
    MaszynaMaterialManager.named_texture_remove(texture_name)


func _get_script_arguments() -> Array:
    return [parameters]
