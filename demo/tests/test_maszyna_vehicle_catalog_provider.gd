extends MaszynaGutTest

## MaszynaVehicleCatalogProvider: the vehicles of MaSzyna's index for the catalogue, of the kind
## their category code says (a carriage by its series letter), a livery for each skin; and the
## MaSzyna data of a vehicle derived from the catalogue's name and livery

const GAME_DIR:String = "res://tests/fixtures/vehicles_bank"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(GAME_DIR)


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _names(vehicles:Array[VehicleInfo]) -> Array[String]:
    var names:Array[String] = []
    for info:VehicleInfo in vehicles:
        if info.vehicle_provider == MaszynaVehicleData.VEHICLE_PROVIDER:
            names.append(info.vehicle)
    return names


func test_the_index_vehicles_are_listed_by_their_kind_under_their_name() -> void:
    assert_eq(_names(VehicleCatalogServer.vehicle_list(VehicleInfo.VehicleCategory.ELECTRIC_LOCOMOTIVE)),
            ["dynamic/pkp/e186_v1/f140ms", "dynamic/pkp/e186_v1/p160dc"] as Array[String])
    # the wagon's code "A" is none of the ones the catalogue maps
    assert_eq(_names(VehicleCatalogServer.vehicle_list(VehicleInfo.VehicleCategory.OTHER)),
            ["dynamic/test/wagon_v1/wagon"] as Array[String])


func test_a_livery_for_each_skin_of_the_index() -> void:
    var p160dc:VehicleInfo = VehicleCatalogServer.vehicle_list(VehicleInfo.VehicleCategory.ELECTRIC_LOCOMOTIVE).filter(
            func(info:VehicleInfo) -> bool: return info.vehicle.ends_with("p160dc"))[0]
    var names:Array[String] = []
    for livery:VehicleLivery in VehicleCatalogServer.livery_list(p160dc):
        names.append(livery.name)

    assert_eq(names, ["146_226-6", "480_002-9"] as Array[String])


func test_a_carriage_is_of_the_kind_of_its_series_letter() -> void:
    var carriage:MaszynaVehiclesBank.Vehicle = MaszynaVehiclesBank.Vehicle.new("/dynamic/test", "wagon")
    var kinds:Dictionary[String, VehicleInfo.VehicleCategory] = {
        "B": VehicleInfo.VehicleCategory.PASSENGER_CAR,
        "*": VehicleInfo.VehicleCategory.PASSENGER_CAR,
        "E": VehicleInfo.VehicleCategory.FREIGHT_CAR,
        "Z": VehicleInfo.VehicleCategory.FREIGHT_CAR,
        "X": VehicleInfo.VehicleCategory.SPECIAL,
        "A": VehicleInfo.VehicleCategory.OTHER,
    }
    for code:String in kinds:
        carriage.category_code = code
        carriage.category = MaszynaVehiclesBank.category_of(code)
        assert_eq(MaszynaVehicleCatalogProvider.category_of(carriage), kinds[code], code)


func test_the_maszyna_data_of_a_vehicle_is_its_name_and_livery() -> void:
    var vehicle:MaszynaVehicleData = MaszynaVehicleData.new()
    vehicle.file_name = "p160dc"
    vehicle.data_path = "/dynamic/pkp/e186_v1"
    vehicle.skin = "480_002-9"

    assert_eq(vehicle.vehicle_provider, MaszynaVehicleData.VEHICLE_PROVIDER)
    assert_eq(vehicle.vehicle, "dynamic/pkp/e186_v1/p160dc")
    assert_eq(vehicle.data_path, "dynamic/pkp/e186_v1")
    assert_eq(vehicle.livery, "480_002-9")
