@tool
extends EditorImportPlugin
class_name FIZImportPlugin

## Standard Godot import pipeline for .fiz files placed inside the project (res://): gives the
## Import dock, a .import sidecar, and Reimport - unlike FIZResourceLoader (a bare
## ResourceFormatLoader), which only covers direct load()/FizTrainController.fiz_path access
## to .fiz files living outside the project, in the external MaSzyna data directory. Both
## reuse the same FizVehicleBuilder builder. Registered via add_import_plugin() in
## libmaszyna.gd's _enter_tree()/_exit_tree().


func _get_importer_name() -> String:
    return "libmaszyna.fiz"


func _get_visible_name() -> String:
    return "FIZ Vehicle"


func _get_recognized_extensions() -> PackedStringArray:
    return PackedStringArray(["fiz"])


## A .fiz imports into the vehicle's description (a MoverRailVehicleController), a Resource - `.scn` is the packed-scene extension
## and ResourceSaver refuses anything else under it (ERR_FILE_UNRECOGNIZED), which left every
## .fiz in the project unimported.
func _get_save_extension() -> String:
    return "res"


func _get_resource_type() -> String:
    return "Resource"


func _get_preset_count() -> int:
    return 1


func _get_preset_name(_preset_index: int) -> String:
    return "Default"


func _get_import_options(_path: String, _preset_index: int) -> Array[Dictionary]:
    return []


func _get_option_visibility(_path: String, _option_name: StringName, _options: Dictionary) -> bool:
    return true


func _get_import_order() -> int:
    return 0


func _get_priority() -> float:
    return 1.0


func _import(
        source_file: String, save_path: String, _options: Dictionary,
        _platform_variants: Array[String], _gen_files: Array[String]) -> Error:
    var description: VehicleController = FizVehicleBuilder.build_description_at(source_file)
    if description == null:
        return ERR_CANT_CREATE
    return ResourceSaver.save(description, "%s.%s" % [save_path, _get_save_extension()])
