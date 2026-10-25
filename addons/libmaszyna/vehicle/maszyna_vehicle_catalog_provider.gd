@tool
extends VehicleCatalogProvider
class_name MaszynaVehicleCatalogProvider

## MaSzyna's rolling stock for the catalogue (VehicleCatalogServer), registered by libmaszyna's
## runtime plugin (libmaszyna_runtime.gd) as
## MaszynaVehicleData.VEHICLE_PROVIDER: the vehicles of the game's dynamic/ as the launcher of the
## original gathers them (MaszynaVehiclesBank), a livery for each of a vehicle's skins, its side view
## (MaszynaVehicleProfileManager), and a vehicle built by MaszynaVehicleSystem. A vehicle is named by
## its data path and file name (VehicleData.vehicle, "dynamic/pkp/4e_v1/4e-0001").
##
## A vehicle's kind is its index's category code (MaszynaVehiclesBank) - operator, 2026-10-19: a
## carriage, which the launcher does not tell apart, by its series letter; any other code is OTHER.
## People and animals are no rolling stock and are not listed.

## The index's categories that are no rolling stock
const UNLISTED_CATEGORIES:Array[MaszynaVehiclesBank.Category] = [
    MaszynaVehiclesBank.Category.PEOPLE,
    MaszynaVehiclesBank.Category.ANIMALS,
]
const CATEGORIES:Dictionary[MaszynaVehiclesBank.Category, VehicleInfo.VehicleCategory] = {
    MaszynaVehiclesBank.Category.ELECTRIC_LOCOS: VehicleInfo.VehicleCategory.ELECTRIC_LOCOMOTIVE,
    MaszynaVehiclesBank.Category.DIESEL_LOCOS: VehicleInfo.VehicleCategory.DIESEL_LOCOMOTIVE,
    MaszynaVehiclesBank.Category.STEAM_LOCOS: VehicleInfo.VehicleCategory.STEAM_LOCOMOTIVE,
    MaszynaVehiclesBank.Category.EMU: VehicleInfo.VehicleCategory.MULTIPLE_UNIT,
    MaszynaVehiclesBank.Category.RAILCARS: VehicleInfo.VehicleCategory.RAILCAR,
    MaszynaVehiclesBank.Category.TRAMS: VehicleInfo.VehicleCategory.TRAM,
    MaszynaVehiclesBank.Category.DRAISINES: VehicleInfo.VehicleCategory.DRAISINE,
    MaszynaVehiclesBank.Category.UTILITY: VehicleInfo.VehicleCategory.UTILITY_VEHICLE,
    MaszynaVehiclesBank.Category.TRUCKS: VehicleInfo.VehicleCategory.ROAD_TRUCK,
    MaszynaVehiclesBank.Category.BUSES: VehicleInfo.VehicleCategory.ROAD_BUS,
    MaszynaVehiclesBank.Category.CARS: VehicleInfo.VehicleCategory.ROAD_CAR,
}
## The carriages' series letters, and "*" the index gives carriages of mixed classes
const CARRIAGE_CATEGORIES:Dictionary[String, VehicleInfo.VehicleCategory] = {
    "B": VehicleInfo.VehicleCategory.PASSENGER_CAR,
    "W": VehicleInfo.VehicleCategory.PASSENGER_CAR,
    "*": VehicleInfo.VehicleCategory.PASSENGER_CAR,
    "E": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "Z": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "S": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "F": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "U": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "R": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "G": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "T": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "L": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "I": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "H": VehicleInfo.VehicleCategory.FREIGHT_CAR,
    "X": VehicleInfo.VehicleCategory.SPECIAL,
}


## The index's vehicles by name (VehicleInfo.vehicle), read as the provider starts and as the game's
## data is read again - one read takes a tenth of a second of a game directory's 1375 vehicles
var _vehicles:Dictionary[String, MaszynaVehiclesBank.Vehicle] = {}


## The index is read as the provider enters the tree, before its runtime plugin registers it
func _enter_tree() -> void:
    read_vehicles()
    GameDataServer.data_reload_requested.connect(read_vehicles)


func _exit_tree() -> void:
    GameDataServer.data_reload_requested.disconnect(read_vehicles)


## The index read from the game directory again; people and animals are left out
func read_vehicles() -> void:
    _vehicles.clear()
    for vehicle:MaszynaVehiclesBank.Vehicle in MaszynaVehiclesBank.scan(MaszynaGameDirectory.get_game_dir()):
        if not vehicle.category in UNLISTED_CATEGORIES:
            _vehicles[vehicle.data_path.trim_prefix("/").path_join(vehicle.file_name)] = vehicle


func vehicle_list(category:VehicleInfo.VehicleCategory) -> Array[VehicleInfo]:
    var vehicles:Array[VehicleInfo] = []
    for vehicle_name:String in _vehicles:
        var vehicle:MaszynaVehiclesBank.Vehicle = _vehicles[vehicle_name]
        if not category_of(vehicle) == category:
            continue
        var info:VehicleInfo = VehicleInfo.new()
        info.vehicle_provider = MaszynaVehicleData.VEHICLE_PROVIDER
        info.vehicle = vehicle_name
        info.title = vehicle.file_name
        info.category = category
        vehicles.append(info)
    return vehicles


## A livery for each skin of the vehicle's index, named after the skin
func livery_list(vehicle:VehicleInfo) -> Array[VehicleLivery]:
    var liveries:Array[VehicleLivery] = []
    var indexed:MaszynaVehiclesBank.Vehicle = _vehicles.get(vehicle.vehicle)
    if not indexed:
        return liveries
    for skin:String in indexed.skins:
        var livery:VehicleLivery = VehicleLivery.new()
        livery.name = skin
        liveries.append(livery)
    return liveries


## The side view of the vehicle's model in its skin (VehicleData.livery)
func vehicle_get_side_view(vehicle:VehicleData) -> Texture2D:
    return await MaszynaVehicleProfileManager.get_profile(
            vehicle.vehicle.get_base_dir(), vehicle.vehicle.get_file(), vehicle.livery, vehicle.vehicle_id)


## The vehicle's length over the buffers on its side view
func vehicle_get_side_view_coupling_width(vehicle:VehicleData) -> float:
    return MaszynaVehicleProfileManager.get_profile_coupling_width(
            vehicle.vehicle.get_base_dir(), vehicle.vehicle.get_file())


## The vehicle's .fiz built into the core's controller (FizVehicleBuilder.build_description()); null
## for a vehicle of no .fiz - an index may list one (dynamic/road/jelcz's "315", its .fiz is 315dumb),
## which the original cannot drive either
func vehicle_get_description(vehicle:VehicleData) -> VehicleController:
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var data_path:String = vehicle.vehicle.get_base_dir()
    if not FileAccess.file_exists(game_dir.path_join(MaszynaDataPath.resolve(
            game_dir, data_path.path_join(vehicle.vehicle.get_file() + ".fiz")))):
        return null
    return FizVehicleBuilder.build_description(data_path, vehicle.vehicle.get_file())


## The vehicle's .mmd read in its skin (VehicleData.livery): its appearance
## (MaszynaRailVehicle3DInstancer.read_structure())
func vehicle_get_appearance(vehicle:VehicleData) -> VehicleAppearance:
    var structure:MaszynaVehicleStructure = MaszynaRailVehicle3DInstancer.read_structure(
            vehicle.vehicle.get_base_dir(), vehicle.vehicle.get_file(), vehicle.livery, vehicle.vehicle_id)
    return structure.appearance if structure else null


func vehicle_create(vehicle:VehicleData, scene_node_id:int) -> RID:
    return MaszynaVehicleSystem.vehicle_create(vehicle, scene_node_id)


func vehicle_free(vehicle:RID) -> void:
    MaszynaVehicleSystem.vehicle_free(vehicle)


func vehicle_get_data(vehicle:RID) -> VehicleData:
    return MaszynaVehicleSystem.vehicle_get_dynamic(vehicle)


## The catalogue's kind of a vehicle of the index
static func category_of(vehicle:MaszynaVehiclesBank.Vehicle) -> VehicleInfo.VehicleCategory:
    if vehicle.category in CATEGORIES:
        return CATEGORIES[vehicle.category]
    return CARRIAGE_CATEGORIES.get(vehicle.category_code, VehicleInfo.VehicleCategory.OTHER)
