@tool
extends RefCounted

## Where a model's file is looked for, in this order for each extension: its path as given, from
## the game directory ("models\linia053\peron_sandomierz.t3d", "dynamic\pkp\..."), then under
## models/ ("bud\dombale.t3d") - TModelsManager::find_on_disk(), MdlMngr.cpp:146-150
const MODEL_EXTENSIONS:Array[String] = ["e3d", "t3d"]
const MODELS_DIRECTORY:String = "models"
## What ends a `lights` or `lightcolors` list (TAnimModel::is_keyword(), AnimModel.cpp:268-277)
const KEYWORDS:PackedStringArray = ["endmodel", "lights", "lightcolors", "angles", "scale", "notransition"]


func import(p:MaszynaParser, context: MaszynaImporterContext) -> MaszynaModelData:
    var loc_x = p.next_token()
    var loc_y = p.next_token()
    var loc_z = p.next_token()
    var rot_y = p.next_token()
    var filename:String = p.next_token().replace("\\", "/")
    var data_path:String = filename.get_base_dir()

    var obj := MaszynaModelData.new()
    obj.model_filename = filename.get_file().get_basename()
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var lookups:Array[String] = []
    for extension:String in MODEL_EXTENSIONS:
        for directory:String in [data_path, MODELS_DIRECTORY.path_join(data_path)]:
            lookups.append(directory.path_join(obj.model_filename + "." + extension))
    var found:String = ""
    for lookup:String in lookups:
        var file:String = MaszynaDataPath.resolve(game_dir, lookup)
        if FileAccess.file_exists(game_dir.path_join(file)):
            found = file
            break
    # a model found nowhere keeps the models/ path, for the loader to report
    obj.data_path = (
        found.get_base_dir() if found else MaszynaDataPath.resolve(game_dir, MODELS_DIRECTORY.path_join(data_path))
    )

    obj.position = Vector3(float(loc_x), float(loc_y), float(loc_z))
    obj.rotation = Vector3(0.0, deg_to_rad(float(rot_y)), 0.0)
    var skins = p.next_token()
    if not skins.to_lower() == "none":
        obj.skins = skins.split("|")

    # `lights <mode> ...` and `lightcolors <hex> ...` run until the next keyword, which is then
    # read as the node goes on (TAnimModel::Load(), AnimModel.cpp:335-371): `lights 4.5 angles 0 80
    # 0 endmodel` is a light and a rotation. A mode is ls_Off/ls_On/ls_Blink/ls_Dark/ls_Home plus an
    # optional fraction, one per light in Light_On00..07 order; a colour is an RGB hex literal, with
    # -1 meaning "leave the model's own colour alone".
    var token:String = p.next_token().to_lower()
    while token and not token == "endmodel":
        match token:
            "lights":
                var modes:PackedFloat32Array = []
                token = p.next_token().to_lower()
                while token and not KEYWORDS.has(token):
                    modes.append(float(token))
                    token = p.next_token().to_lower()
                obj.lights = modes
                continue
            "lightcolors":
                var colors:PackedColorArray = []
                token = p.next_token().to_lower()
                while token and not KEYWORDS.has(token):
                    colors.append(_parse_light_color(token))
                    token = p.next_token().to_lower()
                obj.light_colors = colors
                continue
            "angles":
                var angles:Array = p.get_tokens(3)
                obj.rotation = Vector3(
                    deg_to_rad(float(angles[0])),
                    deg_to_rad(float(angles[1])),
                    deg_to_rad(float(angles[2])),
                )
        token = p.next_token().to_lower()

    return obj


## A `lightcolors` entry is an RGB hex literal; -1 keeps the colour the model carries
## (AnimModel.cpp:356). That "no override" is passed on as a negative colour.
func _parse_light_color(token:String) -> Color:
    if token == "-1":
        return Color(-1.0, -1.0, -1.0)
    var value:int = token.hex_to_int()
    return Color8((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff)
