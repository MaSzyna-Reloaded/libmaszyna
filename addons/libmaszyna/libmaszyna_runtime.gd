@tool
extends ReloadedRuntimePlugin

## libmaszyna's part of the game (MaszynaPlugin's runtime_script): MaSzyna's rolling stock for the
## catalogue, made and registered as the plugin enters the tree - before the main scene, whose vehicles
## ask for it as they enter it (MaszynaRailVehicle3D._enter_tree()) - and unregistered as it leaves

var _vehicle_catalog_provider:MaszynaVehicleCatalogProvider = null


func _enter_tree() -> void:
    _vehicle_catalog_provider = MaszynaVehicleCatalogProvider.new()
    add_child(_vehicle_catalog_provider)
    VehicleCatalogServer.provider_register(MaszynaVehicleData.VEHICLE_PROVIDER, _vehicle_catalog_provider.get_instance_id())


func _exit_tree() -> void:
    VehicleCatalogServer.provider_unregister(MaszynaVehicleData.VEHICLE_PROVIDER)
    _vehicle_catalog_provider.queue_free()
    _vehicle_catalog_provider = null
