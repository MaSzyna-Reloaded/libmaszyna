@tool
extends ReloadedRuntimePlugin

## libmaszyna's part of the game (MaszynaPlugin's runtime_script): the readers of the vehicles' FIZ
## and MMD files handed to their managers, and MaSzyna's rolling stock for the catalogue, made and
## registered as the plugin enters the tree - before the main scene, whose vehicles
## ask for it as they enter it (MaszynaRailVehicle3D._enter_tree()) - and unregistered as it leaves

var _vehicle_catalog_provider:MaszynaVehicleCatalogProvider = null


func _enter_tree() -> void:
    # what reads the vehicles' FIZ and MMD files, whose results the managers keep
    MaszynaFizManager.set_description_reader(
            FizVehicleBuilder.create_description, FizVehicleBuilder.FIZ_PARSER_FORMAT_VERSION)
    MaszynaVehicleMmdManager.set_mmd_data_reader(
            MaszynaRailVehicle3DInstancer.read_mmd_data, MaszynaRailVehicle3DInstancer.MMD_DATA_FORMAT_VERSION)
    _vehicle_catalog_provider = MaszynaVehicleCatalogProvider.new()
    add_child(_vehicle_catalog_provider)
    VehicleCatalogServer.provider_register(MaszynaVehicleData.VEHICLE_PROVIDER, _vehicle_catalog_provider.get_instance_id())


func _exit_tree() -> void:
    VehicleCatalogServer.provider_unregister(MaszynaVehicleData.VEHICLE_PROVIDER)
    _vehicle_catalog_provider.queue_free()
    _vehicle_catalog_provider = null
