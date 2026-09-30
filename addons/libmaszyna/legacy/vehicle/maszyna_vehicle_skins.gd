@tool
extends RefCounted
class_name MaszynaVehicleSkins

## Skins of a vehicle, as the launcher of the original lists them: from the textures.txt of the
## vehicle directory (launcher/textures_scanner.cpp). A skin line is
## "<texture or material file>=<vehicle>,<group>,<miniature>//<description>", with <vehicle> the
## name of the .fiz/.mmd the skin is made for; lines starting with "!", "*" and "@" describe the
## category, coupling rules and controllability instead.
##
## A skin need not have a .mat at all (dynamic/pkp/e186_v2 has plain textures only), and a .mat
## need not be a skin (its szyby_deszcz.mat is the glass of the cab) - so without a textures.txt
## the .mat files next to the vehicle are all that can be offered.

const INDEX_FILE: String = "textures.txt"
const RULE_PREFIXES: Array[String] = ["!", "*", "@"]


## Skins made for the given vehicle, in the order of the index. vehicle_dir is an absolute path.
static func list_skins(vehicle_dir: String, file_name: String) -> Array[String]:
    var index_path: String = vehicle_dir.path_join(MaszynaDataPath.resolve(vehicle_dir, INDEX_FILE))
    if FileAccess.file_exists(index_path):
        return _list_indexed_skins(index_path, file_name)
    return _list_material_skins(vehicle_dir)


static func _list_indexed_skins(index_path: String, file_name: String) -> Array[String]:
    var skins: Array[String] = []
    # the descriptions are cp1250; file and vehicle names, all that is read here, are ASCII
    var lines: PackedStringArray = FileAccess.get_file_as_bytes(index_path).get_string_from_ascii().split("\n")
    for line: String in lines:
        var entry: PackedStringArray = parse_skin_line(line)
        if not entry or not entry[0].to_lower() == file_name.to_lower():
            continue
        if not entry[1] in skins:
            skins.append(entry[1])
    return skins


## The vehicle and the skin of one line of a textures.txt, as [vehicle, skin]; empty for a line
## that names no skin - a rule, a comment or nothing (vehicles_bank::parse_entry(),
## launcher/textures_scanner.cpp:28)
static func parse_skin_line(line: String) -> PackedStringArray:
    line = line.get_slice("//", 0).strip_edges()
    if not line.contains("=") or line.left(1) in RULE_PREFIXES:
        return PackedStringArray()
    return PackedStringArray([
            line.get_slice("=", 1).get_slice(",", 0).strip_edges(),
            line.get_slice("=", 0).strip_edges().get_basename()])


static func _list_material_skins(vehicle_dir: String) -> Array[String]:
    var skins: Array[String] = []
    var files: PackedStringArray = DirAccess.get_files_at(vehicle_dir)
    files.sort()
    for file: String in files:
        if not file.get_extension().to_lower() == "mat":
            continue
        var skin: String = file.get_basename()
        # multi-slot skins are "<name>,<slot>.mat", only the first slot is a skin of its own
        if skin.contains(",") and not skin.ends_with(",1"):
            continue
        skin = skin.trim_suffix(",1")
        # a skin with slots also has a plain .mat of the same name
        if not skin in skins:
            skins.append(skin)
    return skins
