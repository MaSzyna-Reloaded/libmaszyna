@tool
extends EditorExportPlugin

## The base classes of the cab screen scripts (cabin/python/<runtime>/*.py) go into the export as
## they are: Godot exports no file it does not import, and PythonScreenServer copies them out of the
## pack for the Python host, which reads only files on disk

## The runtimes' directories, relative to this script
const PYTHON_DIR: String = "../cabin/python"


func _get_name() -> String:
    return "MaszynaPythonScreens"


func _export_begin(_features: PackedStringArray, _is_debug: bool, _path: String, _flags: int) -> void:
    var python_dir: String = get_script().resource_path.get_base_dir().path_join(PYTHON_DIR).simplify_path()
    for runtime: String in DirAccess.get_directories_at(python_dir):
        var runtime_dir: String = python_dir.path_join(runtime)
        for file: String in DirAccess.get_files_at(runtime_dir):
            if file.get_extension() == "py":
                var path: String = runtime_dir.path_join(file)
                add_file(path, FileAccess.get_file_as_bytes(path), false)
