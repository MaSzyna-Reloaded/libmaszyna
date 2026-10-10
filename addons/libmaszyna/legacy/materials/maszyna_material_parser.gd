@tool
extends Node

const STOP_KEY = [" ", ";", char(9), char(10), char(13)]
const STOP_VALUE = [" ", ";", char(9), char(10), char(13)]


func parse(model_path:String, material_file:String) -> MaszynaMaterial:
    var final_path:String = ""
    var project_data_dir:String = UserSettings.get_maszyna_game_dir()
    var material_name:String = material_file
    # FIXME: move this to MaterialManager
    var possible_paths:Array[String] = [
        model_path.trim_prefix("/").path_join(material_name + ".mat"),
        "textures".path_join(model_path.trim_prefix("/")).path_join(material_name + ".mat"),
        material_name + ".mat",
        "textures".path_join(material_name + ".mat"),
    ]
    for relative_path:String in possible_paths:
        relative_path = MaszynaDataPath.resolve(project_data_dir, relative_path)
        var full_path:String = project_data_dir.path_join(relative_path)
        if FileAccess.file_exists(full_path):
            final_path = full_path
            break

    var file:FileAccess = FileAccess.open(final_path, FileAccess.READ) as FileAccess
    var mat:MaszynaMaterial = MaszynaMaterial.new()

    if file:
        var p:MaszynaParser = MaszynaParser.new()
        p.initialize(file.get_buffer(file.get_length()))

        var data:Dictionary = {}
        var current:Array[Dictionary] = [data]
        var key:String = ""

        while not p.eof_reached():
            var token:String = p.next_token(STOP_VALUE)

            if token.ends_with(":"):
                key = token.split(":")[0]
            else:
                match token:
                    "}":
                        current.pop_front()
                        key = ""
                    "{":
                        var sub:Dictionary = {}
                        current[0][key] = sub
                        key = ""
                        current.push_front(sub)
                    "[":
                        current[0][key] = []
                    "]":
                        pass
                    _:
                        if token and key:
                            if current[0].has(key):
                                if not typeof(current[0][key]) == TYPE_ARRAY:
                                    current[0][key] = [current[0][key]]
                                current[0][key].push_back(token)
                            else:
                                current[0][key] = token
        _update_material(mat, data)
    else:
        mat.default.set_texture_path("diffuse", material_file)

    mat.transparent = mat.default.require_transparency
    for variant:MaszynaMaterial.MaszynaMaterialVariant in mat.variants.values():
        mat.transparent = mat.transparent or variant.require_transparency
    return mat

func _update_material(mat: MaszynaMaterial, data: Dictionary) -> void:
    mat.opacity = float(_pop_dict(data, "opacity", 0))
    mat.selfillum = float(_pop_dict(data, "selfillum", 0.0))
    mat.glossiness = float(_pop_dict(data, "glossiness", 0.0))
    mat.shadow_rank = int(_pop_dict(data, "glossiness", 0))
    mat.size = _parse_size(_pop_dict(data, "size", []))

    for key in data:
        var value = data[key]
        match typeof(value):
            TYPE_DICTIONARY:
                mat.variants[key] = _create_variant(value)
            _:
                _update_variant(mat.default, key, value)

func _create_variant(data: Dictionary) -> MaszynaMaterial.MaszynaMaterialVariant:
    var variant := MaszynaMaterial.MaszynaMaterialVariant.new()
    for key in data:
        var value = data[key]
        _update_variant(variant, key, value)
    return variant

func _update_variant(variant: MaszynaMaterial.MaszynaMaterialVariant, key: String, value: Variant) -> void:
    if key.begins_with("texture"):
        var cleaned_value
        if typeof(value) == TYPE_ARRAY:
            variant.require_transparency = (
                variant.require_transparency
                or value.map(_texture_requires_transparency).any(func(x): x)
            )
            cleaned_value = value.map(_clean_texture_path)
        else:
            variant.require_transparency = (
                variant.require_transparency or _texture_requires_transparency(value)
            )
            cleaned_value = _clean_texture_path(value)
        var texture_name = key.substr(7)
        if texture_name[0] == "_":
            texture_name = key.substr(8)
        if texture_name.is_valid_int():
            texture_name = "tex%s" % texture_name
        if typeof(value) == TYPE_ARRAY:
            var random_textures:Array[String] = []
            random_textures.append_array(cleaned_value)
            variant.set_random_texture_paths(texture_name, random_textures)
        else:
            variant.set_texture_path(texture_name, cleaned_value)
    elif key == "shader":
        # the original resolves "mat_<name>.frag" on a case insensitive file system
        # (opengl33renderer.cpp:2018) and the data relies on it ("shader: Default_1")
        variant.shader = str(value).to_lower()
    elif key.begins_with("param_"):
        var parameter_name := key.substr(6)
        if typeof(value) == TYPE_ARRAY:
            var values: Array = value
            if values.size() == 4:
                variant.set_parameter_vec4(parameter_name, _parse_vector4(values))
            else:
                # Quirk: besides a 4 component value, an array here is the same key repeated in one
                # block (real data, e.g. pods_grass.mat) - the original keeps the first definition,
                # a later one with the same priority is ignored (material.cpp:351-357).
                variant.set_parameter(parameter_name, float(values[0]))
        else:
            variant.set_parameter(parameter_name, float(value))

func _pop_dict(dict: Dictionary, key: Variant, default: Variant=null) -> Variant:
    var output:Variant = dict.get(key, default)
    dict.erase(key)
    return output

## Metres, fractions allowed ("size: 0.4 0.4"); -1 when absent (material.cpp:399-403)
func _parse_size(value: Array) -> Vector2:
    if value.size() == 2:
        return Vector2(float(value[0].strip_edges()), float(value[1].strip_edges()))
    return Vector2(-1.0, -1.0)

func _parse_vector4(value: Array) -> Vector4:
    if value.size() == 4:
        return Vector4(
            float(str(value[0]).strip_edges()),
            float(str(value[1]).strip_edges()),
            float(str(value[2]).strip_edges()),
            float(str(value[3]).strip_edges())
        )
    return Vector4.ZERO

func _clean_texture_path(path:String) -> String:
    # utilities.cpp:537 (deserialize_random_set) - the original swaps "\\" for "/" in texture paths.
    return path.split(":")[0].replace("\\", "/")

func _texture_requires_transparency(path:String) -> bool:
    var _parts:PackedStringArray = path.split(":")
    var texture:String = _parts[0]
    return "t" in _parts
