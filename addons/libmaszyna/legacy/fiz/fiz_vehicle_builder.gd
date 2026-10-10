@tool
extends Node
class_name FizVehicleBuilder

## Top-level FIZ file orchestrator, mirroring scenery_instancer.gd's role for scenery files.
##
## FIZ's key=value micro-language is detected by line-PREFIX match (mirrors MaSzyna's own
## `issection()`, Mover.cpp:9035-9039), not by the exact-token `register_handler`/`parse()`
## loop the scenery importers use - a section header and its first key=value pair share one
## token (e.g. `Cntrl.BCPN=6`), so section identification happens per physical line, and only
## the remainder of that line (and, for table sections, the following lines up to an end
## marker) gets tokenized.

const _INCLUDE_KEYWORD := "include"
const _INCLUDE_END_KEYWORD := "end"

## Bump whenever a fiz_train_*_parser.gd file's parsing/field-mapping LOGIC changes (not just
## FIZ data) - mirrors E3DModel.FORMAT_VERSION (addons/libmaszyna/legacy/e3d/e3d_model_manager.gd), the
## same on-disk-cache-invalidation pattern for the same reason: _make_cache_hash() below is keyed
## on the source .fiz file's own mtime, so a parser bugfix with no change to the .fiz data itself
## is otherwise silently served from a stale pre-fix cache entry until something touches that
## specific vehicle's file. Confirmed the hard way: a MotorParamTable0/nmax column-mapping fix
## had zero effect in a running game because of exactly this.
const FIZ_PARSER_FORMAT_VERSION := 45

## Ordered (longest-prefix-first where ambiguity is possible) table of recognized FIZ section
## headers. `parser` is a section parser instance (see fiz_train_*_parser.gd) exposing
## `parse(p: MaszynaParser, context, prefix)` and, for table sections, `parse_row(p, context)` +
## `end_table(context)` - `p` is a throwaway MaszynaParser scoped to exactly one line (header
## line with its prefix stripped, or one table row), mirroring the original LoadFIZ_* code's
## habit of constructing "a fresh cParser parser(line)" per line/row. `table_end` is the
## literal end-of-table token for table sections, "" for plain scalar sections. `parser ==
## null` means "recognized but not yet implemented / has no Godot class" - the section (and
## any table rows up to table_end, if set) is skipped with a single warning per vehicle.
static var _sections: Array[Dictionary] = []


## The section table is fixed - it is the FIZ grammar, not per-vehicle data - so it is built once
## when the class is loaded rather than re-checked on every parse.
static func _static_init() -> void:
    var controller_parser := FizTrainControllerParser.new()
    var wheels_parser := FizTrainWheelsParser.new()
    var doors_parser := FizTrainDoorsParser.new()
    var buff_coupl_parser := FizTrainBuffCouplParser.new()
    var brake_parser := FizTrainBrakeParser.new()
    var cntrl_parser := FizTrainCntrlParser.new(controller_parser, brake_parser)
    var lighting_parser := FizTrainLightingParser.new()
    var heating_parser := FizTrainHeatingParser.new()
    var power_parser := FizTrainPowerParser.new()
    var engine_parser := FizTrainEngineParser.new()
    var turbo_parser := FizTrainTurboParser.new()
    var electric_series_parser := engine_parser.electric_series_parser
    var security_system_parser := FizTrainSecuritySystemParser.new()
    var spring_brake_parser := FizTrainSpringBrakeParser.new()
    var ep_dynamic_brake_parser := FizTrainElectroPneumaticDynamicBrakeParser.new()
    var speed_control_parser := FizTrainSpeedControlParser.new()
    var switches_parser := FizTrainSwitchesParser.new()
    var ai_hints_parser := FizTrainAIHintsParser.new()
    var load_parser := FizTrainLoadParser.new()
    var wipers_parser := FizTrainWipersParser.new()
    var universal_controller_parser := FizTrainUniversalControllerParser.new()
    var diesel_engine_parser := FizTrainDieselEngineParser.new()

    _sections = [
        # controller-mapped
        {"prefix": "Param.", "parser": controller_parser, "table_end": ""},
        {"prefix": "Dimensions:", "parser": controller_parser, "table_end": ""},
        {"prefix": "Load:", "parser": load_parser, "table_end": ""},
        {"prefix": "Wheels:", "parser": wheels_parser, "table_end": ""},
        # brake family (Cntrl. also starts the BPT table)
        {"prefix": "Brake:", "parser": brake_parser, "table_end": ""},
        {"prefix": "Cntrl.", "parser": cntrl_parser, "table_end": ""}, # BPT rows start after this line, see below
        {"prefix": "SpringBrake:", "parser": spring_brake_parser, "table_end": ""},
        {"prefix": "Blending:", "parser": ep_dynamic_brake_parser, "table_end": ""},
        {"prefix": "DCEMUED:", "parser": ep_dynamic_brake_parser, "table_end": ""},
        {"prefix": "CompressorList:", "parser": brake_parser, "table_end": "endCL"},
        # doors / couplers
        {"prefix": "Doors:", "parser": doors_parser, "table_end": ""},
        {"prefix": "BuffCoupl1.", "parser": buff_coupl_parser, "table_end": ""},
        {"prefix": "BuffCoupl2.", "parser": buff_coupl_parser, "table_end": ""},
        {"prefix": "BuffCoupl.", "parser": buff_coupl_parser, "table_end": ""},
        # lighting / heating / power
        {"prefix": "Headlights:", "parser": lighting_parser, "table_end": ""},
        {"prefix": "LightsList:", "parser": lighting_parser, "table_end": "endL"},
        {"prefix": "Light:", "parser": lighting_parser, "table_end": ""},
        {"prefix": "Clima:", "parser": heating_parser, "table_end": ""},
        {"prefix": "Power:", "parser": power_parser, "table_end": ""},
        {"prefix": "SpeedControl:", "parser": speed_control_parser, "table_end": ""},
        {"prefix": "Switches:", "parser": switches_parser, "table_end": ""},
        {"prefix": "DimmerList:", "parser": switches_parser, "table_end": "endDimmerList"},
        {"prefix": "AI:", "parser": ai_hints_parser, "table_end": ""},
        {"prefix": "Security:", "parser": security_system_parser, "table_end": ""},
        {"prefix": "WiperList:", "parser": wipers_parser, "table_end": "endwl"},
        {"prefix": "UCList:", "parser": universal_controller_parser, "table_end": "END-UCL"},
        # engine family
        {"prefix": "Engine:", "parser": engine_parser, "table_end": ""},
        # MotorParamTable0: has the initial constants of a series motor (readMPT0); MotorParamTable:
        # is read by the engine type (readMPT, Mover.cpp:9120) - FizTrainEngineParser picks the parser
        {"prefix": "MotorParamTable0:", "parser": electric_series_parser, "table_end": "END-MPT"},
        {"prefix": "MotorParamTable:", "parser": engine_parser, "table_end": "END-MPT"},
        {"prefix": "Circuit:", "parser": electric_series_parser, "table_end": ""},
        {"prefix": "RList:", "parser": electric_series_parser, "table_end": "END-RL"},
        {"prefix": "DList:", "parser": diesel_engine_parser, "table_end": "END-DL"},
        {"prefix": "DMList:", "parser": diesel_engine_parser, "table_end": "END-DML"},
        {"prefix": "HTCList:", "parser": diesel_engine_parser, "table_end": "END-HTCL"},
        {"prefix": "PmaxList:", "parser": engine_parser.electric_induction_parser, "table_end": "END-PML"},
        {"prefix": "WWList:", "parser": engine_parser.diesel_electric_parser, "table_end": "END-WWL"},
        {"prefix": "V2NList:", "parser": diesel_engine_parser, "table_end": "END-V2NL"},
        # TurboPos: LoadFIZ_TurboPos (Mover.cpp:10714) sets TurboTest, which the turbo sound reads
        # (DynObj.cpp:8267)
        {"prefix": "TurboPos:", "parser": turbo_parser, "table_end": ""},
        # ffList:/ffBrakeList: share electric_induction_parser's wwlist target (DElist/
        # RlistSize, read by TractionForce()'s ElectricInductionMotor branch) - first-write-
        # wins if a file has both, see FizTrainElectricInductionEngineParser.end_table().
        {"prefix": "ffBrakeList:", "parser": engine_parser.electric_induction_parser, "table_end": "endff"},
        {"prefix": "ffList:", "parser": engine_parser.electric_induction_parser, "table_end": "endff"},
    ]


## Parses a FIZ file into a fresh VehicleController: root-level properties (Param./Dimensions:/
## Cntrl. general subset/...) are applied to `target` and every section's VehicleComponent is
## attached to it with add_component(). `target` is expected to carry no components yet - a
## second run would attach a second component of the same type - which is why its one caller,
## build_description_at(), hands it a controller it has just created, which becomes the
## description. `fiz_path` must already be a fully resolved, openable path (res://, user://, or
## absolute) - e.g. UserSettings.get_maszyna_game_dir().path_join("pkp/eu04_v1/eu04-01.fiz").
## `include` directives inside the file resolve relative to its own containing directory.
static func build_into(target: VehicleController, fiz_path: String) -> void:
    var context := FizImportContext.new()
    context.base_dir = fiz_path.get_base_dir()
    context.controller = target

    var table_state: Dictionary = {"prefix": "", "parser": null, "end": ""}
    _parse_file(fiz_path, context.base_dir, context, table_state)

    if table_state["parser"] != null:
        table_state["parser"].end_table(context)

    # a diesel's legacy "main" compressor runs off its engine (CheckLocomotiveParameters, Mover.cpp:11738)
    var brake: RailVehicleBrake = context.get_part("RailVehicleBrake") as RailVehicleBrake
    if brake and brake.compressor_power == RailVehicleBrake.COMPRESSOR_POWER_MAIN \
            and context.engine_type in [RailVehicleEngine.DIESEL, RailVehicleEngine.DIESEL_ELECTRIC]:
        brake.compressor_power = RailVehicleBrake.COMPRESSOR_POWER_ENGINE

    # an EZT's reverser steps past "forward" to the high start (DirectionForward, Mover.cpp:719)
    var diesel_engine: RailVehicleDieselEngine = context.get_part("RailVehicleEngine") as RailVehicleDieselEngine
    if diesel_engine:
        diesel_engine.turbo_position = context.turbo_position

    var series_engine: RailVehicleElectricSeriesEngine = context.get_part("RailVehicleEngine") as RailVehicleElectricSeriesEngine
    if series_engine:
        series_engine.direction_switches_circuit_imin_high = context.train_type == RailVehicleController.TRAIN_TYPE_EZT

    for part_name: String in context.parts:
        target.add_component(context.parts[part_name])

    var power_supply: RailVehiclePowerSupply = FizTrainPowerSupplyParser.create_node(context)
    if power_supply:
        target.add_component(power_supply)

    var engine_power_source: RailVehicleEnginePowerSource = FizTrainPowerParser.create_node(context)
    if engine_power_source:
        target.add_component(engine_power_source)

    # The horns and the train radio have no FIZ section to trigger on: the horns are implied by the
    # MMD's horn buttons and sounds (RailVehicleHorns.hpp), the radio is the cab's (TTrain's channel
    # and volume). Both belong to a vehicle with a cab - one with a master controller
    # (FizTrainCntrlParser.MASTER_CONTROLLER_MIN_POSITIONS, MASZYNA_ORIGINAL_QUIRKS.md).
    if context.get_part("RailVehicleMasterController"):
        target.add_component(MoverRailVehicleHorns.new())
        target.add_component(MoverRailVehicleRadio.new())

## Same on-disk cache used by E3DModelManager for parsed E3D models (addons/libmaszyna/legacy/e3d/
## e3d_model_manager.gd) - keyed by mtime+path like that cache's own _make_cache_hash(), so an
## edited .fiz (or an `include`d one - mtime isn't recursive, but editing a shared .fiz.inc
## while iterating is rare enough not to warrant walking every include) invalidates the entry.
## Stored as the vehicle's description (a MoverRailVehicleController with its components) - the
## parse result, which is the expensive part; a vehicle is built from a copy of it
## (VehiclePhysicsNode).
static var _cache = ResourceCache.create("fiz")

static func clear_cache() -> void:
    _cache.clear()

static func _make_cache_path(fiz_path: String) -> String:
    var relative_path: String = fiz_path.trim_prefix(UserSettings.get_maszyna_game_dir().path_join(""))
    return relative_path + ".res"

static func _make_cache_hash(fiz_path: String) -> String:
    return ("%s:%s:%s" % [
        FileAccess.get_modified_time(fiz_path),
        str(FIZ_PARSER_FORMAT_VERSION),
        fiz_path
    ]).md5_text()


## The vehicle a .fiz describes, parsed once and cached on disk - the shape
## E3DModelManager.load_model() has, and for the same reason: a scenery repeats the same file
## across many trainset entries, and parsing it is the expensive part.
static func build_description(data_path: String, fiz_filename: String) -> VehicleController:
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_path:String = data_path.path_join(fiz_filename + ".fiz")
    return build_description_at(game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_path)))


static func build_description_at(fiz_path: String) -> VehicleController:
    var cache_path: String = _make_cache_path(fiz_path)
    var cache_hash: String = _make_cache_hash(fiz_path)
    var description: VehicleController = _cache.get(cache_path, cache_hash) as VehicleController
    if description:
        return description

    # FIZ is the Mover's own format, so the vehicle it describes is built on the Mover; its
    # components are kept in the order the FIZ sections built them
    description = MoverRailVehicleController.new()
    build_into(description, fiz_path)
    _cache.set(cache_path, description, cache_hash)
    return description


## Reads one logical line off a MaszynaParser's byte stream, mirroring the original
## `getToken<std::string>(false, "\n\r")` line reader (Mover.cpp:9661) - a raw, un-tokenized
## line, stopping at \n or \r. Unlike MaszynaParser.get_line() (which treats \r and \n as two
## independent terminators, producing a spurious empty "line" for every CRLF pair), this
## consumes a \n that immediately follows a \r as part of the same terminator, so blank-line
## detection (used to end the brake-position table) isn't corrupted by CRLF encoding.
static func _read_fiz_line(p: MaszynaParser) -> String:
    var bytes := PackedByteArray()
    while not p.eof_reached():
        var c: int = p.get8()
        if c == -1 or c == 10: # EOF or \n
            break
        if c == 13: # \r - swallow a paired \n, if any
            if not p.eof_reached():
                var c2: int = p.get8()
                if c2 != 10 and c2 != -1:
                    bytes.append(c2) # not a CRLF pair (lone CR); keep the byte we peeked
            break
        bytes.append(c)
    # the data is cp1250 ("wagonów" in dynamic/pkp/11xa_v2/111a_old.fiz)
    return Windows1250.decode(bytes)


## Reads the (possibly multi-line) `include <file> [params...] end` directive - real data (e.g.
## dynamic/pkp/sm42_v1/6da.fiz) spreads the filename and each positional param across separate
## physical lines, unlike the common single-line form. `first_line_parser` must already be
## positioned right after the "include" keyword on the current line; this keeps pulling further
## lines from `p` whenever the current line's tokens run out. Returns the collected tokens
## (filename first, then any params), excluding the trailing "end".
static func _read_include_directive(p: MaszynaParser, first_line_parser: MaszynaParser) -> Array[String]:
    var tokens: Array[String] = []
    var current_parser := first_line_parser
    while true:
        var token: String = current_parser.next_token()
        if not token:
            if p.eof_reached():
                break
            var next_line: String = _read_fiz_line(p).strip_edges(true, false)
            if not next_line or next_line.find("#") != -1:
                continue
            current_parser = MaszynaParser.new()
            current_parser.initialize(Windows1250.encode(next_line))
            continue
        if token.to_lower() == _INCLUDE_END_KEYWORD:
            break
        tokens.append(token)
    return tokens


## `parameters` substitutes "(p1)", "(p2)", etc. directly in each raw line's text before any
## tokenizing happens - positional args from an `include file p1 p2 end` directive that brought
## this file in (empty for a top-level, non-included file). Confirmed necessary against real
## data: dynamic/pkp/sm42_v1/6d.fiz.inc's `Param. ... M=(p1) ...` / `Brake: ... MaxBP=(p2) ...`
## left mass at 0 (and cascaded into NaN velocity) with no substitution at all.
static func _parse_file(
        abs_path: String, dir: String, context: FizImportContext, table_state: Dictionary,
        parameters: Dictionary = {}) -> void:
    context.include_depth += 1
    if context.include_depth > 32:
        push_error("FIZ include depth exceeded (circular include?): " + abs_path)
        context.include_depth -= 1
        return

    var file := FileAccess.open(abs_path, FileAccess.READ)
    if not file:
        push_error("Cannot open FIZ file: " + abs_path)
        context.include_depth -= 1
        return

    var p := MaszynaParser.new()
    p.initialize(file.get_buffer(file.get_length()))
    file.close()

    while not p.eof_reached():
        var raw_line: String = _read_fiz_line(p)
        var line: String = raw_line.strip_edges(true, false)
        if parameters:
            for key: String in parameters:
                line = line.replace("(%s)" % key, str(parameters[key]))

        # FIZ-specific: a line containing an unescaped '#' anywhere is fully ignored. This is
        # distinct from `//`/`/* */`, which MaszynaParser's tokenizer already strips on its
        # own - a `//`-only line naturally yields zero tokens below, no special-casing needed.
        if line.find("#") != -1:
            if table_state["prefix"] == "BPT":
                table_state["parser"].end_table(context)
                table_state["prefix"] = ""
                table_state["parser"] = null
                table_state["end"] = ""
            continue

        if not line:
            if table_state["prefix"] == "BPT":
                table_state["parser"].end_table(context)
                table_state["prefix"] = ""
                table_state["parser"] = null
                table_state["end"] = ""
            continue

        var line_parser := MaszynaParser.new()
        line_parser.initialize(Windows1250.encode(line))
        var first_token: String = line_parser.next_token()
        if not first_token:
            continue # line was entirely a `//`/`/* */` comment

        # include <file> [params...] end - splices the referenced file's lines in place,
        # sharing the same table_state so an in-progress table can (rarely) continue across
        # the include boundary, matching the original cParser's transparent splicing. Real data
        # (dynamic/pkp/sm42_v1/6da.fiz) spreads "include", the filename, and each param across
        # separate physical lines rather than one line - _read_include_directive() keeps pulling
        # further lines from `p` as needed instead of assuming everything fits on this one line.
        if first_token == _INCLUDE_KEYWORD:
            var include_tokens: Array[String] = _read_include_directive(p, line_parser)
            if include_tokens:
                var include_filename: String = include_tokens[0]
                var include_params: Dictionary = {}
                for i in range(1, include_tokens.size()):
                    include_params["p%d" % i] = include_tokens[i]
                var include_path: String = dir.path_join(MaszynaDataPath.resolve(dir, include_filename))
                _parse_file(include_path, include_path.get_base_dir(), context, table_state, include_params)
            continue
        if first_token == _INCLUDE_END_KEYWORD:
            continue

        var matched_section: Dictionary = {}
        for section: Dictionary in _sections:
            if line.begins_with(section["prefix"]):
                matched_section = section
                break

        if matched_section:
            _dispatch_header(matched_section, line, context, table_state)
            continue

        if table_state["end"] != "" and line.strip_edges() == table_state["end"]:
            if table_state["parser"] != null:
                table_state["parser"].end_table(context)
            table_state["prefix"] = ""
            table_state["parser"] = null
            table_state["end"] = ""
            continue

        if table_state["parser"] != null:
            var row_parser := MaszynaParser.new()
            row_parser.initialize(Windows1250.encode(line))
            table_state["parser"].parse_row(row_parser, context)
        # else: unrecognized line outside any table - ignored, matching original tolerance.

    context.include_depth -= 1


static func _dispatch_header(
        section: Dictionary, line: String, context: FizImportContext, table_state: Dictionary) -> void:
    var prefix: String = section["prefix"]
    var line_parser := MaszynaParser.new()
    line_parser.initialize(Windows1250.encode(line.substr(prefix.length())))

    # A header ends whatever table is still open, also one that opens a table of its own - a
    # LightsList: without its endL runs straight into WiperList: (dynamic/pkp/e186_v2/p160dc.fiz),
    # as every section header ends the list before it in the original (Mover.cpp:9681)
    if table_state["parser"] != null:
        table_state["parser"].end_table(context)
    table_state["prefix"] = ""
    table_state["parser"] = null
    table_state["end"] = ""

    if section["parser"] == null:
        context.warn_unmapped_section(prefix)
    else:
        section["parser"].parse(line_parser, context, prefix)

    # Cntrl. additionally opens the brake-position table (only when BrakeSystem != Individual,
    # decided by Brake:/Cntrl. themselves inside RailVehicleBrake's own parser)
    if prefix == "Cntrl." and section["parser"] != null and section["parser"].wants_bpt_table(context):
        table_state["prefix"] = "BPT"
        table_state["parser"] = section["parser"]
        table_state["end"] = ""
    elif section["table_end"] != "":
        table_state["prefix"] = prefix
        table_state["parser"] = section["parser"]
        table_state["end"] = section["table_end"]
