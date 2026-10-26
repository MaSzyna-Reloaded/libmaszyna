extends MaszynaGutTest

## libmaszyna's part of the game (MaszynaPlugin's runtime_script) is held by the core's
## ReloadedPlugins, which enters the tree before the main scene: the catalogue's provider of MaSzyna's
## vehicles is its child, registered as "maszyna" - there for a scene's vehicles as they enter the tree
## (libreloaded#3: an autoload registered in _ready() came after them).

const RUNTIME_SCRIPT:GDScript = preload("res://addons/libmaszyna/libmaszyna_runtime.gd")


func _runtime_plugin() -> ReloadedRuntimePlugin:
    for child:Node in ReloadedPlugins.get_children():
        if child.get_script() == RUNTIME_SCRIPT:
            return child as ReloadedRuntimePlugin
    return null


func test_the_plugin_names_its_runtime_part() -> void:
    var scripts:PackedStringArray = []
    for plugin:ReloadedPlugin in ReloadedPluginServer.plugin_get_list():
        scripts.append(plugin.runtime_script)
    assert_has(scripts, RUNTIME_SCRIPT.resource_path)


func test_the_vehicle_provider_is_the_runtime_plugin_s() -> void:
    var runtime:ReloadedRuntimePlugin = _runtime_plugin()
    assert_not_null(runtime, "ReloadedPlugins holds libmaszyna's runtime plugin")
    if not runtime:
        return
    var provider:Object = instance_from_id(VehicleCatalogServer.provider_get(MaszynaVehicleData.VEHICLE_PROVIDER))
    assert_true(provider is MaszynaVehicleCatalogProvider, "the provider of MaSzyna's vehicles is registered")
    assert_eq((provider as Node).get_parent(), runtime, "made by the runtime plugin, not an autoload")
