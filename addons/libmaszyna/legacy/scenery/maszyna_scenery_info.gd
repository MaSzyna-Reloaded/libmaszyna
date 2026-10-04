@tool
extends RefCounted
class_name MaszynaSceneryInfo

## Scenario description from the header comments of a .scn file (read by the original starter):
## its name (read_display_name(), from [code]//$l[/code], [code]//$n[/code] and the file name),
## [code]//$d[/code] description lines, [code]//$i[/code] image in
## scenery/images/, plus the trainsets of its [code]trainset[/code] blocks. Files are in cp1250.
##
## Read line by line: the tokens of a "trainset"/"node ... dynamic" are written on one line in
## every scenery, and the mission description of a trainset is in [code]//$o[/code] comments,
## which a tokenizer would drop.

## One vehicle of a trainset ("node ... dynamic ... enddynamic")
class Vehicle:
    ## The scenery's name of the vehicle - MaszynaDynamicData.name
    var train_id:String = ""
    ## e.g. "dynamic/pkp/su42_v1"
    var data_path:String = ""
    var skin:String = ""
    ## Base name of the vehicle's .mmd/.fiz files
    var file_name:String = ""
    ## "headdriver" for the vehicle the player starts in, "reardriver", "passenger", ...
    var driver_type:String = ""
    ## Stood the other way round - its offset is -1 (MaszynaNodeDynamicImporter)
    var reversed:bool = false

    ## A driver sits in it - only "headdriver" and "reardriver" steer a cab, "passenger" and
    ## "nobody" are no driver (DynObj.cpp:1994-2001)
    func has_driver() -> bool:
        return driver_type == "headdriver" or driver_type == "reardriver"

    ## The same vehicle in the same files and skin - the name of the scenery included
    func is_same(other:Vehicle) -> bool:
        return (train_id == other.train_id and data_path == other.data_path
                and file_name == other.file_name and skin == other.skin and reversed == other.reversed)

    ## The same vehicle, to be changed without touching this one
    func copy() -> Vehicle:
        var vehicle:Vehicle = Vehicle.new()
        vehicle.train_id = train_id
        vehicle.data_path = data_path
        vehicle.skin = skin
        vehicle.file_name = file_name
        vehicle.driver_type = driver_type
        vehicle.reversed = reversed
        return vehicle


## One "trainset ... endtrainset" block
class Trainset:
    var name:String = ""
    var track:String = ""
    ## Mission description of the trainset (//$o lines)
    var description:String = ""
    var vehicles:Array[Vehicle] = []

    ## A driver sits in one of its vehicles, so the player can take the trainset - the original's
    ## launcher refuses one without ("Trainset not occupied", scenery_list.cpp:135-153)
    func is_occupied() -> bool:
        for vehicle:Vehicle in vehicles:
            if vehicle.has_driver():
                return true
        return false

    ## The scenario offers the trainset to the player: one whose mission description begins with
    ## "-" is an AI train or a decoration, which the original's Starter does not list (a
    ## convention of the data, MASZYNA_ORIGINAL_QUIRKS.md)
    func is_offered() -> bool:
        return not description.begins_with("-")

    ## The player can drive it: it is occupied and the scenario offers it
    func is_drivable() -> bool:
        return is_occupied() and is_offered()

    ## Vehicle the player starts in: the one with a headdriver, else any with a driver
    func get_driver_train_id() -> String:
        for vehicle:Vehicle in vehicles:
            if vehicle.driver_type == "headdriver":
                return vehicle.train_id
        for vehicle:Vehicle in vehicles:
            if vehicle.driver_type == "reardriver":
                return vehicle.train_id
        return vehicles[0].train_id if vehicles else ""

## Only the beginning of a scenery is read - the header and the trainsets are there
const MAX_BYTES:int = 262144
## The title alone is in the first lines of the file
const MAX_HEADER_BYTES:int = 8192
## How a header writes the stations of a line ("//$n Macierzewo - Całkowo - Wili - Jarkawki")
const NAME_SEPARATOR:String = "-"
## Between "//$l", "//$n" and the file name in read_display_name()
const PART_SEPARATOR:String = " · "
## Words of a header that name nothing ("//$n Sceneria Linia053_Wrzosy"), as _simplify() gives them
const NAME_FILLER_WORDS:PackedStringArray = ["sceneria"]
## A "//$n" this long is a description, not a name
const NAME_MAX_LENGTH:int = 60
## ...and so is one with any of these ("//$n Bieszczady topuwa, dużo izoluw przy torach wisi...")
const SENTENCE_MARKS:PackedStringArray = [",", "...", "!", "?"]
## The scenery's name, as the scenario selector and the loading screen show it (read_display_name())
var title:String = ""
var description:String = ""
## Absolute path of the scenario image, empty when it does not exist
var image_path:String = ""
var trainsets:Array[Trainset] = []


## Name of scenery/<filename> for a list of sceneries: its "//$l" line made readable (written as
## carelessly as "Całkowo_v2_towarowe"), its "//$n" line unless it is a sentence (the scenario's
## description written there instead of a name) and the file name made readable, each
## without the words an earlier one already has - the file name is what tells apart the scenarios
## of one scenery ("baltyk_skm1.scn" with "//$l Bałtyk" and "//$n Bałtyk" -> "Bałtyk · SKM1").
static func read_display_name(filename:String) -> String:
    var scenery_dir:String = UserSettings.get_maszyna_game_dir().path_join("scenery")
    var file:FileAccess = FileAccess.open(
        scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, filename)), FileAccess.READ
    )
    var bytes:PackedByteArray = (
        file.get_buffer(mini(file.get_length(), MAX_HEADER_BYTES)) if file else PackedByteArray()
    )
    var scenery_name:String = ""
    var scenario_name:String = ""
    var start:int = 0
    while start < bytes.size():
        var end:int = bytes.find(10, start)  # "\n"
        if end < 0:
            end = bytes.size()
        var line:String = Windows1250.decode(bytes.slice(start, end)).strip_edges()
        start = end + 1
        if line.begins_with("//$l"):
            scenery_name = humanize_name(line.substr(4).strip_edges())
        elif line.begins_with("//$n"):
            # a description, so its words keep their case - only "Linia053_Wrzosy" is split
            scenario_name = line.substr(4).strip_edges().replace("_", " ")
        elif line and not line.begins_with("//"):
            break
    var known_words:PackedStringArray = NAME_FILLER_WORDS.duplicate()
    var parts:PackedStringArray = []
    if scenario_name.length() > NAME_MAX_LENGTH:
        scenario_name = ""
    for mark:String in SENTENCE_MARKS:
        if scenario_name.contains(mark):
            scenario_name = ""
    for part:String in [scenery_name, scenario_name, humanize_name(filename.get_basename())]:
        var part_words:PackedStringArray = part.split(" ", false)
        var words:PackedStringArray = []
        for word:String in part_words:
            # a separator ("Macierzewo - Całkowo - Wili") goes with the word dropped beside it
            if word == NAME_SEPARATOR:
                if words and not words[-1] == NAME_SEPARATOR:
                    words.append(word)
                continue
            if not known_words.has(_simplify(word)):
                words.append(word)
        if words and words[-1] == NAME_SEPARATOR:
            words.remove_at(words.size() - 1)
        if words:
            parts.append(" ".join(words))
        for word:String in part_words:
            known_words.append(_simplify(word))
    return PART_SEPARATOR.join(parts)


## Lowercased and without the Polish letters, for comparing the words of the parts of a name
static func _simplify(word:String) -> String:
    var simplified:String = word.to_lower()
    for replacement:Array in [
        ["ą", "a"], ["ć", "c"], ["ę", "e"], ["ł", "l"], ["ń", "n"],
        ["ó", "o"], ["ś", "s"], ["ź", "z"], ["ż", "z"],
    ]:
        simplified = simplified.replace(replacement[0], replacement[1])
    return simplified


## "stary_jawor_noc" -> "Stary Jawor Noc", "calkowo_sm42_v2" -> "Calkowo SM42 V2" (a word with
## a digit in it is a vehicle or a version, those are written in capitals); a word already written
## with a capital ("SKM", "Całkowo") is kept as it is
static func humanize_name(text:String) -> String:
    var words:PackedStringArray = []
    for token:String in text.replace("_", " ").split(" ", false):
        # "-" standing alone separates names ("Macierzewo - Wili"), inside a word it is a space
        if token == NAME_SEPARATOR:
            words.append(token)
            continue
        for word:String in token.split("-", false):
            if _has_digit(word):
                words.append(word.to_upper())
            elif word == word.to_lower():
                words.append(word.capitalize())
            else:
                words.append(word)
    return " ".join(words)


static func _has_digit(word:String) -> bool:
    for character:String in word:
        if character.is_valid_int():
            return true
    return false


## Reads the header of scenery/<filename> and the trainsets declared in it
static func read(filename:String) -> MaszynaSceneryInfo:
    var info := MaszynaSceneryInfo.new()
    var scenery_dir:String = UserSettings.get_maszyna_game_dir().path_join("scenery")
    var file:FileAccess = FileAccess.open(
        scenery_dir.path_join(MaszynaDataPath.resolve(scenery_dir, filename)), FileAccess.READ
    )
    if not file:
        return info
    # the name the scenario selector and the loading screen show it by
    info.title = read_display_name(filename)
    # raw bytes - FileAccess.get_line() decodes UTF-8 and would lose the cp1250 characters
    var bytes:PackedByteArray = file.get_buffer(mini(file.get_length(), MAX_BYTES))
    var description_lines:PackedStringArray = []
    var mission_lines:PackedStringArray = []
    var in_header:bool = true
    var trainset:Trainset = null
    var start:int = 0
    while start < bytes.size():
        var end:int = bytes.find(10, start)  # "\n"
        if end < 0:
            end = bytes.size()
        var line:String = Windows1250.decode(bytes.slice(start, end)).strip_edges()
        start = end + 1

        if line.begins_with("//$o"):
            mission_lines.append(line.substr(4).strip_edges())
            continue
        if in_header:
            if line and not line.begins_with("//"):
                in_header = false
            elif line.begins_with("//$d"):
                description_lines.append(line.substr(4).strip_edges())
                continue
            elif line.begins_with("//$i"):
                var images_dir:String = scenery_dir.path_join("images")
                var image_path:String = images_dir.path_join(
                    MaszynaDataPath.resolve(images_dir, line.substr(4).strip_edges())
                )
                if FileAccess.file_exists(image_path):
                    info.image_path = image_path
                continue
            else:
                continue

        var tokens:PackedStringArray = line.get_slice("//", 0).split(" ", false)
        if not tokens:
            continue
        match tokens[0].to_lower():
            "trainset":
                trainset = Trainset.new()
                trainset.name = tokens[1] if tokens.size() > 1 else ""
                trainset.track = tokens[2] if tokens.size() > 2 else ""
                mission_lines.clear()
                info.trainsets.append(trainset)
            "endtrainset":
                if trainset:
                    trainset.description = "\n".join(mission_lines)
                    mission_lines.clear()
                trainset = null
            "node":
                if trainset and tokens.size() > 9 and tokens[4].to_lower() == "dynamic":
                    var vehicle := Vehicle.new()
                    vehicle.train_id = tokens[3]
                    vehicle.data_path = _resolve_data_path(tokens[5])
                    # spelled as the .scn spells them, like maszyna_node_dynamic_importer.gd - the
                    # files are found by MaszynaDataPath.resolve() whatever the letter case
                    vehicle.skin = tokens[6]
                    vehicle.file_name = tokens[7]
                    vehicle.reversed = is_equal_approx(float(tokens[8]), -1.0)
                    vehicle.driver_type = tokens[9].to_lower()
                    trainset.vehicles.append(vehicle)
    info.description = "\n".join(description_lines)
    return info


## "PKP\\SU42_V1" -> "dynamic/pkp/su42_v1", same as maszyna_node_dynamic_importer.gd
static func _resolve_data_path(data_folder:String) -> String:
    var segments:PackedStringArray = data_folder.replace("\\", "/").split("/", false)
    if not segments or not segments[0].to_lower() == "dynamic":
        segments.insert(0, "dynamic")
    return MaszynaDataPath.resolve(UserSettings.get_maszyna_game_dir(), "/".join(segments))
