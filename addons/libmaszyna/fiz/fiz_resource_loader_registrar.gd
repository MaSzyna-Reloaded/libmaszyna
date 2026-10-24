extends Node

## Registers FIZResourceLoader with the engine's ResourceLoader. Must be an autoload (not just
## code inside libmaszyna.gd's _enable_plugin()) so `.fiz` stays load()-able in exported game
## builds too, not only while the editor plugin is running - EditorPlugin scripts don't run in
## exported builds, but autoloads registered via add_autoload_singleton() do.

var _loader: FIZResourceLoader


func _enter_tree() -> void:
    _loader = FIZResourceLoader.new()
    ResourceLoader.add_resource_format_loader(_loader)


func _exit_tree() -> void:
    if _loader:
        ResourceLoader.remove_resource_format_loader(_loader)
        _loader = null
