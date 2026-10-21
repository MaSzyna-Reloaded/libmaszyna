@tool
extends Node

## Reads the particle emitter templates the original keeps in the game's [code]data/[/code]
## directory into the [SmokeSource]s [MaszynaE3DRenderingServer] builds its emitters of through
## [SmokeServer].
##
## An emitter is a model submodel named [code]smokesource_<template>[/code]; the whole submodel
## name is the file name of its template ([code]particle_manager::find()[/code],
## [code]particles.cpp:465[/code]). There are twelve of them in the data set, so they are memoised
## in memory and not cached on disk.


## The one smoke texture the original uses for every emitter (opengl33renderer.cpp:105)
const SMOKE_TEXTURE:String = "fx/smoke"

## Which sprite a particle is drawn with. ORIGINAL is the single round blob the original binds for
## every emitter; MODERN is the core's own smoke - SmokeServer's flipbook.
enum GeneratorMode {ORIGINAL, MODERN}

const GENERATOR_MODE_SETTING:String = "maszyna/smoke/generator_mode"

## cParser stop characters of the template grammar (particles.cpp:24); the braces are tokens of
## their own and must not be listed here
const _STOP_CHARS:Array = [" ", "\t", "\n", "\r", ";", ",", "[", "]"]

## Block key -> property of [SmokeSource], the shape the original's own variablemaps have
## (particles.cpp:24-33, particles.h:222-226)
const _INITIALIZER_FIELDS:Dictionary = {
    "min_inclination:": "inclination_min",
    "max_inclination:": "inclination_max",
    "min_velocity:": "velocity_min",
    "max_velocity:": "velocity_max",
    "min_size:": "size_min",
    "max_size:": "size_max",
    "min_opacity:": "opacity_min",
    "max_opacity:": "opacity_max",
}
const _SIZE_CHANGE_FIELDS:Dictionary = {
    "step:": "size_step",
    "min:": "size_limit_min",
    "max:": "size_limit_max",
}
const _OPACITY_CHANGE_FIELDS:Dictionary = {
    "step:": "opacity_step",
    "min:": "opacity_limit_min",
    "max:": "opacity_limit_max",
}

var _sources:Dictionary[String, SmokeSource] = {}
## The original's sprite, read once from the game's data
var _sprite:Texture2D = null
## What the emitters were last built with (uses_atlas())
var _atlas_used:bool = false


## MaszynaE3DRenderingServer resolves the emitters of an instance through this
func _ready() -> void:
    MaszynaE3DRenderingServer.smoke_set_source_resolver(resolve)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)
    _atlas_used = uses_atlas()
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)


## Another generator mode, or the core's flipbook given or taken away: every emitter is drawn with
## the other sprite
func _on_project_settings_changed() -> void:
    var atlas_used:bool = uses_atlas()
    if ProjectSettings.get_setting(GENERATOR_MODE_SETTING, GeneratorMode.ORIGINAL) == GeneratorMode.MODERN \
            and not atlas_used:
        push_warning("[MaszynaSmokeSourceLibrary] Modern smoke needs the core's smoke atlas; falling back to the original sprite")
    if atlas_used == _atlas_used:
        return
    _atlas_used = atlas_used
    MaszynaE3DRenderingServer.smoke_rebuild()


## What the emitter of [param template_name] is made of and drawn with: [code]source[/code], null
## for a template that cannot be read, and [code]sprite[/code], null for the core's own smoke. The
## original's sprite is read from the game's data once, so every emitter shares one.
func resolve(template_name:String) -> Dictionary:
    if uses_atlas():
        return {"source": get_source(template_name), "sprite": null}
    if not _sprite:
        _sprite = MaszynaMaterialManager.load_texture("", SMOKE_TEXTURE)
    return {"source": get_source(template_name), "sprite": _sprite}


## Whether the particles are the core's own smoke: MODERN, and the project gives its flipbook
func uses_atlas() -> bool:
    return ProjectSettings.get_setting(GENERATOR_MODE_SETTING, GeneratorMode.ORIGINAL) == GeneratorMode.MODERN \
            and SmokeServer.smoke_get_atlas() != null


func get_source(template_name:String) -> SmokeSource:
    if _sources.has(template_name):
        return _sources[template_name]

    var data_dir:String = MaszynaGameDirectory.get_game_dir().path_join("data")
    var path:String = data_dir.path_join(ReloadedPluginServer.data_path_resolve(data_dir, template_name + ".txt"))
    var source:SmokeSource = parse_file(path)
    if source:
        source.resource_name = template_name
    _sources[template_name] = source
    return source


## The templates and the sprite are read again when next asked for
func _on_data_unload_requested() -> void:
    _sources.clear()
    _sprite = null


## Parses one template file. Returns null when it cannot be read.
static func parse_file(abs_path:String) -> SmokeSource:
    var file:FileAccess = FileAccess.open(abs_path, FileAccess.READ)
    if not file:
        push_error("[MaszynaSmokeSourceLibrary] Particle source template not found: %s" % abs_path)
        return null

    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(file.get_buffer(file.get_length()), _STOP_CHARS)
    var tokens:Array[String] = []
    while not parser.eof_reached():
        var token:String = parser.next_token()
        if token:
            tokens.append(token)
    return _parse_tokens(tokens)


static func _parse_tokens(tokens:Array[String]) -> SmokeSource:
    var source:SmokeSource = SmokeSource.new()
    var i:int = 0
    while i < tokens.size():
        var key:String = tokens[i].to_lower()
        i += 1
        match key:
            "spawn_rate:":
                if i < tokens.size():
                    source.spawn_rate = float(tokens[i])
                    i += 1
            "initializer:":
                i = _parse_block(tokens, i, source, _INITIALIZER_FIELDS, true)
            "size_change:":
                i = _parse_block(tokens, i, source, _SIZE_CHANGE_FIELDS, false)
            "opacity_change:":
                i = _parse_block(tokens, i, source, _OPACITY_CHANGE_FIELDS, false)
    return source


## Reads a `{ key: value ... }` block into the properties [param fields] maps the keys to.
## Returns the index just past the closing brace.
static func _parse_block(
        tokens:Array[String], start:int, source:SmokeSource,
        fields:Dictionary, with_color:bool) -> int:
    var i:int = start
    if i < tokens.size() and tokens[i] == "{":
        i += 1

    while i < tokens.size() and not tokens[i] == "}":
        var key:String = tokens[i].to_lower()
        i += 1
        if with_color and key == "color:" and i + 2 < tokens.size():
            source.color = Color8(int(tokens[i]), int(tokens[i + 1]), int(tokens[i + 2]))
            i += 3
        elif fields.has(key) and i < tokens.size():
            source.set(fields[key], float(tokens[i]))
            i += 1
    return i + 1
