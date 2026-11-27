@tool
extends RefCounted
class_name MaszynaVehiclesBank

## Every vehicle of the game's dynamic/ directory with its skins, as the launcher of the original
## gathers them: from the textures.txt of each vehicle directory (vehicles_bank::scan_textures(),
## launcher/textures_scanner.cpp:5). A vehicle is a directory and the name of its .fiz/.mmd, both
## lower case as the launcher keeps them (get_vehicle(), :247); a directory without a textures.txt
## has no vehicles here, as it has none in the launcher.

const DYNAMIC_DIR: String = "dynamic"
const INDEX_FILE: String = "textures.txt"
## What a skin's line of the index describes it by, after its "//": a list split by commas
## (vehicles_bank::parse_meta(), launcher/textures_scanner.cpp:146)
const DESCRIPTION_MARK: String = "//"
const DESCRIPTION_SEPARATOR: String = ","
## A line of the index that sets the category of the vehicles listed after it: "!=e,E186" - the
## letter after it is the category's code (CATEGORY_CODES), the rest is no matter here
const CATEGORY_PREFIX: String = "!="
## A line of the index that says the skin lines after it, up to the next such line, go in groups of
## its number, each the vehicles of one formation front to rear, each in its skin: "^3" before an
## EN57's RA, S and RB. The starter's convention: the launcher of the original skips the line
## (launcher/textures_scanner.cpp:18)
const FORMATION_PREFIX: String = "^"

## What kind of vehicle it is, as the original's launcher sorts them - by the index's category
## lines alone, never by the .fiz, the engine or the name
enum Category {
    ELECTRIC_LOCOS,
    DIESEL_LOCOS,
    STEAM_LOCOS,
    RAILCARS,
    EMU,
    UTILITY,
    DRAISINES,
    TRAMS,
    TRUCKS,
    BUSES,
    CARS,
    PEOPLE,
    ANIMALS,
    ## any upper case letter
    CARRIAGES,
    ## any other code, and a vehicle listed before a category line
    UNKNOWN,
}
## The lower case codes; an upper case letter is a carriage
const CATEGORY_CODES: Dictionary[String, Category] = {
    "e": Category.ELECTRIC_LOCOS,
    "s": Category.DIESEL_LOCOS,
    "p": Category.STEAM_LOCOS,
    "a": Category.RAILCARS,
    "z": Category.EMU,
    "r": Category.UTILITY,
    "d": Category.DRAISINES,
    "t": Category.TRAMS,
    "c": Category.TRUCKS,
    "b": Category.BUSES,
    "o": Category.CARS,
    "h": Category.PEOPLE,
    "f": Category.ANIMALS,
}


## The places of a skin's description (vehicles_bank::parse_meta(), launcher/textures_scanner.cpp:
## 146-160): the format's version, the vehicle's name, its operator, where it is kept, the date of
## its revision, who revised it, who made the texture and who took its photos
enum SkinDescription {
    VERSION,
    NAME,
    OPERATOR,
    LOCATION,
    REVISION_DATE,
    REVISION_COMPANY,
    TEXTURE_AUTHOR,
    PHOTO_AUTHOR,
}


## Vehicles that run together, coupled for good, each in its skin, front to rear
class Formation:
    var vehicles: Array[Vehicle] = []
    ## The skin of each of the vehicles
    var skins: Array[String] = []


class Vehicle:
    ## The vehicle's directory as a `dynamic` names it ("/dynamic/pkp/en57_v1")
    var data_path: String
    var file_name: String
    ## In the order of the index
    var skins: Array[String] = []
    ## What the index describes each skin by, its list as written (SkinDescription), by the skin
    var skin_descriptions: Dictionary[String, PackedStringArray] = {}
    var category: Category = Category.UNKNOWN
    ## The code of its category line as the index writes it ("e", "E", "*"); empty before any
    var category_code: String = ""
    ## The formations it runs in, in the order of the index (FORMATION_PREFIX)
    var formations: Array[Formation] = []

    func _init(p_data_path: String, p_file_name: String) -> void:
        data_path = p_data_path
        file_name = p_file_name


## The vehicles under game_dir (an absolute path), by directory and then by name; none without a
## dynamic/ directory
static func scan(game_dir: String) -> Array[Vehicle]:
    var by_path: Dictionary[String, Vehicle] = {}
    var dynamic_dir: String = MaszynaDataPath.resolve(game_dir, DYNAMIC_DIR)
    var directories: Array[String] = [dynamic_dir]
    if not DirAccess.dir_exists_absolute(game_dir.path_join(dynamic_dir)):
        directories.clear()
    while directories:
        var directory: String = directories.pop_back()
        var absolute_dir: String = game_dir.path_join(directory)
        for subdirectory: String in DirAccess.get_directories_at(absolute_dir):
            directories.append(directory.path_join(subdirectory))
        for file: String in DirAccess.get_files_at(absolute_dir):
            if not file.to_lower() == INDEX_FILE:
                continue
            var data_path: String = "/" + DYNAMIC_DIR.path_join(directory.trim_prefix(dynamic_dir).trim_prefix("/"))
            for vehicle: Vehicle in read_index(absolute_dir.path_join(file), data_path):
                by_path[data_path.path_join(vehicle.file_name)] = vehicle
    var keys: Array[String] = []
    keys.assign(by_path.keys())
    keys.sort()
    var vehicles: Array[Vehicle] = []
    for key: String in keys:
        vehicles.append(by_path[key])
    return vehicles


## The vehicles of one index (an absolute path) of the directory a `dynamic` names data_path
## ("/dynamic/pkp/en57_v1"), in the order of the index: each with its skins, its category and the
## formations its skin lines are grouped in
static func read_index(index_path: String, data_path: String) -> Array[Vehicle]:
    var by_name: Dictionary[String, Vehicle] = {}
    # the skins' descriptions are in cp1250 or in UTF-8
    var lines: PackedStringArray = MaszynaLegacyText.decode(FileAccess.get_file_as_bytes(index_path)).split("\n")
    var category_code: String = ""
    var formation_size: int = 0
    var formation: Formation = Formation.new()
    for line: String in lines:
        var rule: String = line.strip_edges()
        if rule.begins_with(CATEGORY_PREFIX):
            category_code = rule.substr(CATEGORY_PREFIX.length(), 1)
            continue
        if rule.begins_with(FORMATION_PREFIX):
            formation_size = rule.substr(FORMATION_PREFIX.length()).to_int()
            formation = Formation.new()
            continue
        var entry: PackedStringArray = MaszynaVehicleSkins.parse_skin_line(line)
        if not entry:
            continue
        var file_name: String = entry[0].to_lower()
        if not by_name.has(file_name):
            by_name[file_name] = Vehicle.new(data_path, file_name)
            by_name[file_name].category = category_of(category_code)
            by_name[file_name].category_code = category_code
        var vehicle: Vehicle = by_name[file_name]
        if not entry[1] in vehicle.skins:
            vehicle.skins.append(entry[1])
            if DESCRIPTION_MARK in line:
                vehicle.skin_descriptions[entry[1]] = line.get_slice(DESCRIPTION_MARK, 1).strip_edges() \
                        .split(DESCRIPTION_SEPARATOR)
        # a group of one is a vehicle running alone
        if formation_size < 2:
            continue
        formation.vehicles.append(vehicle)
        formation.skins.append(entry[1])
        if formation.vehicles.size() == formation_size:
            for member: Vehicle in formation.vehicles:
                if not formation in member.formations:
                    member.formations.append(formation)
            formation = Formation.new()
    var vehicles: Array[Vehicle] = []
    vehicles.assign(by_name.values())
    return vehicles


## The category of a category line's code: a lower case letter of CATEGORY_CODES, any upper case
## letter a carriage, anything else unknown
static func category_of(code: String) -> Category:
    if CATEGORY_CODES.has(code):
        return CATEGORY_CODES[code]
    if code.length() == 1 and code >= "A" and code <= "Z":
        return Category.CARRIAGES
    return Category.UNKNOWN


## The vehicles whose name, directory or a skin contains `query` (any case), in their order; all of
## them for an empty one
static func filter(vehicles: Array[Vehicle], query: String) -> Array[Vehicle]:
    var text: String = query.strip_edges().to_lower()
    var found: Array[Vehicle] = []
    for vehicle: Vehicle in vehicles:
        if (not text or vehicle.file_name.contains(text) or vehicle.data_path.to_lower().contains(text)
                or " ".join(vehicle.skins).to_lower().contains(text)):
            found.append(vehicle)
    return found
