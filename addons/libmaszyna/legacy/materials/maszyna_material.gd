@tool
extends RefCounted
class_name MaszynaMaterial

const SEASON_WINTER_KEY = "winter"
const SEASON_SPRING_KEY = "spring"
const SEASON_SUMMER_KEY = "summer"
const SEASON_AUTUMN_KEY = "autumn"

const WEATHER_CLEAR_KEY = "clear"
const WEATHER_CLOUDY_KEY = "cloudy"
const WEATHER_RAIN_KEY = "rain"
const WEATHER_SNOW_KEY = "snow"


static var SEASONS_MAP: Dictionary[MaszynaEnvironment.Season, String] = {
    MaszynaEnvironment.Season.SEASON_WINTER: SEASON_WINTER_KEY,
    MaszynaEnvironment.Season.SEASON_SPRING: SEASON_SPRING_KEY,
    MaszynaEnvironment.Season.SEASON_SUMMER: SEASON_SUMMER_KEY,
    MaszynaEnvironment.Season.SEASON_AUTUMN: SEASON_AUTUMN_KEY,
}

static var WEATHER_MAP: Dictionary[MaszynaEnvironment.Weather, String] = {
    MaszynaEnvironment.Weather.WEATHER_CLEAR: WEATHER_CLEAR_KEY,
    MaszynaEnvironment.Weather.WEATHER_CLOUDY: WEATHER_CLOUDY_KEY,
    MaszynaEnvironment.Weather.WEATHER_RAIN: WEATHER_RAIN_KEY,
    MaszynaEnvironment.Weather.WEATHER_SNOW: WEATHER_SNOW_KEY,
}

class MaszynaMaterialVariant extends RefCounted:
    # Numbered textures ("tex1", "tex2", ...) have no alias: which slot of the shader they bind is
    # resolved by MaszynaMaterialFactory.
    const TEXTURE_ALIASES = {
        "normals": ["normalmap"],
        "normalmap": ["normals"],
    }

    var _textures: Dictionary[String, String] = {}
    var _random_textures: Dictionary[String, Array] = {}
    var _parameters: Dictionary[String, float] = {}
    var _parameters_vec4: Dictionary[String, Vector4] = {}
    var shader: String = ""
    var require_transparency := false

    func merge(other: MaszynaMaterialVariant) -> void:
        if other.shader:
            shader = other.shader
        _parameters.merge(other._parameters, true)
        _parameters_vec4.merge(other._parameters_vec4, true)
        _textures.merge(other._textures, true)
        _random_textures.merge(other._random_textures, true)

    func duplicate() -> MaszynaMaterialVariant:
        var copy = MaszynaMaterialVariant.new()
        copy.shader = self.shader
        copy._textures = self._textures.duplicate(true)
        copy._random_textures = self._random_textures.duplicate(true)
        copy._parameters = self._parameters.duplicate()
        copy._parameters_vec4 = self._parameters_vec4.duplicate()
        copy.require_transparency = require_transparency
        return copy

    func set_texture_path(name: String, path: String) -> void:
        _textures[name] = path
        for alias in TEXTURE_ALIASES.get(name, []):
            _textures[alias] = path

    func set_random_texture_paths(name: String, paths: Array[String]) -> void:
        _random_textures[name] = paths
        for alias in TEXTURE_ALIASES.get(name, []):
            _random_textures[alias] = paths

    func get_texture_path(name: String) -> String:
        if name in _textures:
            return _textures[name]
        elif name in _random_textures:
            return _random_textures[name][randf_range(0, _random_textures[name].size()-1)]
        else:
            return ""

    func has_parameter(name: String) -> bool:
        return name in _parameters

    func get_parameter(name: String, default: float = NAN) -> float:
        return _parameters.get(name, default)

    func set_parameter(name: String, value: float) -> void:
        _parameters.set(name, value)

    func has_parameter_vec4(name: String) -> bool:
        return name in _parameters_vec4

    func get_parameter_vec4(name: String, default: Vector4 = Vector4.ZERO) -> Vector4:
        return _parameters_vec4.get(name, default)

    func set_parameter_vec4(name: String, value: Vector4) -> void:
        _parameters_vec4.set(name, value)

var opacity: float = 1.0
var selfillum: float = 0.0
var glossiness: float = 0.0
var shadow_rank: int
var transparent: bool = false
## Physical size of the texture in metres, -1 where the .mat declares none (material.h:32)
var size:Vector2 = Vector2(-1.0, -1.0)

var default: MaszynaMaterialVariant = MaszynaMaterialVariant.new()
var variants: Dictionary[String, MaszynaMaterialVariant] = {}


func get_variant(season: MaszynaEnvironment.Season, weather: MaszynaEnvironment.Weather) -> MaszynaMaterialVariant:
    var output: MaszynaMaterialVariant = default.duplicate()
    var season_key := SEASONS_MAP[season]
    if season_key in variants:
        output.merge(variants[season_key])
    var weather_key := WEATHER_MAP[weather]
    if weather_key in variants:
        output.merge(variants[weather_key])
    return output
