extends RefCounted
class_name MmdCabinInstancer

## MMD cabin file parser/builder, mirroring FizVehicleBuilder's role for FIZ files.
## Scope is Etap A+B of dynamic_train_cabin_feasibility.md: cab1/cab2 model+camera resolution,
## plus "rot"/"mov" animation for the labels in MmdSemanticCatalog only ("wip"/"dgt"/"rotvar"/
## "movvar" math, audio, pyscreen, and any label outside the catalog are parsed just enough to
## keep the token stream aligned, then discarded with a diagnostic).
##
## Unlike FizVehicleBuilder (one MaszynaParser per physical line, because FIZ's
## grammar is line-oriented key=value), MMD's grammar is a plain token stream (label: value
## value value...), so this reads the whole file - with includes spliced in - into one flat
## token array first, then walks it by index. The trade-off: diagnostics below carry line=0
## (MaszynaParser has no cursor/position accessor to reconstruct it from mid-stream) - source
## file is still tracked. Re-add line numbers if MaszynaParser ever grows a position getter.

## How far an aimed indicator light is moved out of its lamp mesh (see _aim_spotlight_at_driver()).
const INDICATOR_LIGHT_OFFSET:float = 0.05
## The radio's lamp, where the cab's radio sounds (Train.cpp:10734, 11617)
const RADIO_LAMP_LABEL:String = "i-radio"
## pantfactors: - its four numbers, and the first slider height taken for a mistake above this
## (DynObj.cpp:5580-5590)
const PANTOGRAPH_FACTOR_COUNT:int = 4
const PANTOGRAPH_FACTOR_MAX_HEIGHT:float = 0.5
## coupleradapter:'s model, length and height (DynObj.cpp:5286-5290), and the directory the model is
## under (TModelsManager::GetModel())
const COUPLER_ADAPTER_TOKENS:int = 3
const MODELS_DIRECTORY:String = "models/"
## The pendulums a vehicle model has at most (DynObj.cpp:5706)
const PENDULUM_COUNT:int = 4
## The cabNdefinition: each kind of cabin is read from (TTrain::InitializeCab(), Train.cpp:8684):
## the front cab is cab1definition:, the rear one cab2definition:, the machine room cab0definition: -
## in the order a vehicle gets its cabins
const CAB_DEFINITIONS:Dictionary[RailVehicleCabinKind.Kind, int] = {
    RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT: 1,
    RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR: 2,
    RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE: 0,
}
## The labels with a car's number and a value's number before their shape (Train.cpp:12147-12166)
const LEADING_NUMBER_LABELS:Array[String] = ["brakes", "eimscreen"]
const LEADING_NUMBER_COUNT:int = 2
## internaldata:'s Radio-Stop alarm (Train.cpp:10340)
const RADIO_STOP_SOUND_LABEL:String = "radiostop"
## Spacing and limit of the lights spread along a long ceiling lamp (see _light_points_along_submodel()).
const LAMP_LIGHT_SPACING:float = 2.0
## An indicator lamp's glow into the cab: its brightness and reach. The defaults are SM42's
## hand-authored radio LED (the "i-radio" entry of MmdSemanticCatalog).
const INDICATOR_GLOW_ENABLED_SETTING:StringName = &"maszyna/cabin/indicator_glow_enabled"
const INDICATOR_GLOW_ENERGY_SETTING:StringName = &"maszyna/cabin/indicator_glow_energy"
const INDICATOR_GLOW_ENERGY_DEFAULT:float = 0.05
const INDICATOR_GLOW_RANGE_SETTING:StringName = &"maszyna/cabin/indicator_glow_range"
const INDICATOR_GLOW_RANGE_DEFAULT:float = 0.1
## Whether a lamp of several pieces gets a light at each (MmdSemanticCatalog.IslandLights)
const REAL_INSTRUMENTS_LIGHTS_SETTING:StringName = &"maszyna/cabin/real_instruments_lights"
## The glow of each piece of an instrument backlight (MmdSemanticCatalog.IslandLights.GLOW); the
## defaults and the fixed parameters are SM42's hand-authored backlight light
const INSTRUMENT_GLOW_ENERGY_SETTING:StringName = &"maszyna/cabin/instrument_glow_energy"
const INSTRUMENT_GLOW_ENERGY_DEFAULT:float = 0.002
const INSTRUMENT_GLOW_RANGE_SETTING:StringName = &"maszyna/cabin/instrument_glow_range"
const INSTRUMENT_GLOW_RANGE_DEFAULT:float = 0.564628
const INSTRUMENT_GLOW_ATTENUATION:float = 1.41
const INSTRUMENT_GLOW_SIZE:float = 0.078
const INSTRUMENT_GLOW_INDIRECT_ENERGY:float = 0.525
## The spotlight parameters a WIDGET_LIGHT island light copies from its widget
const WIDGET_LIGHT_PROPERTIES:Array[StringName] = [
    &"light_color", &"light_size", &"light_specular", &"light_volumetric_fog_energy", &"shadow_enabled",
    &"spot_range", &"spot_attenuation", &"spot_angle", &"spot_angle_attenuation",
]
const LAMP_LIGHT_MAX_COUNT:int = 6
const _RANDOM_INCLUDE_OPEN := "["
const _RANDOM_INCLUDE_CLOSE := "]"
const _INCLUDE_END_KEYWORD := "end"
## The highest (pN) the datapack names; past what an include passes each is "none", as any (pN) is
## in the original (parser.cpp:280)
const MAX_INCLUDE_PARAMETERS:int = 12
const _VARIABLE_ANIMATION_TYPES := ["rotvar", "movvar"]
## Original engine: Globals.h:123 PythonScreenUpdateRate - the shortest interval a Python screen is
## redrawn at, in milliseconds
const PYTHON_SCREEN_UPDATE_TIME_MSEC:int = 200
## A control's lit and unlit submodels: "<name>_on", "<name>_off" (Gauge.cpp:192, 204; Button.cpp:32-33)
const ON_SUFFIX:String = "_on"
const OFF_SUFFIX:String = "_off"
## The colours of a cab's cablight: (TCab::Load, Train.cpp:140) - three RGB triples
const CAB_LIGHT_COLOUR_COUNT:int = 9
const RGB_COMPONENTS:int = 3
## An MMD colour component's range (InteriorLight / 255.f, DynObj.cpp:6930)
const RGB_MAX:float = 255.0
## How far a control's click carries (EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound.h:17; Gauge.h:109,
## Button.h:57-58)
const CONTROL_SOUND_MAX_DISTANCE:float = 7.5
## Voices of the cab's control sounds together, so clicks of several controls overlap
const CONTROL_SOUND_VOICE_COUNT:int = 16


## Parses an MMD file (with includes expanded) into a neutral MmdCabinDefinition for one cab.
## `random_choices` is owned by the caller and reused verbatim across repeated parse() calls
## (e.g. a later cab1<->cab2 rebuild) so a random include set isn't re-rolled each time.
static func parse(
        abs_mmd_path:String, parameters:Dictionary, cab_number:int, random_choices:Dictionary) -> MmdCabinDefinition:
    var context := MmdImportContext.new()
    context.base_dir = abs_mmd_path.get_base_dir()
    context.cab_number = cab_number
    context.random_choices = random_choices

    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)

    # TTrain::InitializeCab() (Train.cpp:10504-10517, 10700-10704): the cab's own cab<N>definition:
    # wherever it stands, read from there until the next cab0definition: or the end of the file -
    # so a cab reads every cab section that follows its own (the original's "TODO: enable full
    # per-cab deserialization"), and the preamble before it is never scanned
    var cab_label:String = "cab%ddefinition:" % cab_number
    var start_index:int = _find_label_index(tokens, cab_label)
    if start_index == -1:
        context.add_diagnostic("error", "MMD_INVALID_CAB_DEFINITION", "No %s found" % cab_label, abs_mmd_path)
        start_index = tokens.size()
    var end_index:int = _find_label_index(tokens, "cab0definition:", start_index + 1)
    if end_index == -1:
        end_index = tokens.size()

    var cab_data:Dictionary = {
        0: _empty_cab_data(),
        1: _empty_cab_data(),
        2: _empty_cab_data(),
    }
    var instruments:Array[MmdInstrumentDescriptor] = []
    var python_screens:Array[MmdPythonScreenDescriptor] = []
    var python_screen_update_time_msec:int = 0

    var i := start_index
    while i < end_index:
        var token:String = tokens[i]
        var label:String = token.to_lower()
        i += 1

        match label:
            "cab0definition:", "cab1definition:", "cab2definition:":
                var n:int = int(label.substr(3, 1))
                # TCab::Load (Train.cpp:130-157): an optional cablight: and its 9 colours come
                # before the bounds, and are not used
                if i < tokens.size() and tokens[i].to_lower() == "cablight:":
                    i += 1 + CAB_LIGHT_COLOUR_COUNT
                var values:Array = _read_floats(tokens, i, 6)
                i += 6
                cab_data[n]["bounds_min"] = Vector3(values[0], values[1], values[2])
                cab_data[n]["bounds_max"] = Vector3(values[3], values[4], values[5])
            "cablight:":
                # anywhere else in the cab it is no control: the original passes over the label
                # alone (InitializeCab, Train.cpp:10530-10700)
                pass
            "driver0angle:", "driver1angle:", "driver2angle:":
                var n:int = int(label.substr(6, 1))
                var values:Array = _read_floats(tokens, i, 2)
                i += 2
                cab_data[n]["driver_angle"] = Vector2(values[0], values[1])
            "driver0pos:", "driver1pos:", "driver2pos:":
                var n:int = int(label.substr(6, 1))
                var values:Array = _read_floats(tokens, i, 3)
                i += 3
                cab_data[n]["driver_pos"] = Vector3(values[0], values[1], values[2])
                # the seat is where the driver stands unless driverNsitpos: says otherwise (Train.cpp:10548)
                cab_data[n]["driver_sitpos"] = cab_data[n]["driver_pos"]
            "driver0sitpos:", "driver1sitpos:", "driver2sitpos:":
                var n:int = int(label.substr(6, 1))
                var values:Array = _read_floats(tokens, i, 3)
                i += 3
                cab_data[n]["driver_sitpos"] = Vector3(values[0], values[1], values[2])
            "cab0model:", "cab1model:", "cab2model:":
                var n:int = int(label.substr(3, 1))
                var model_token:String = tokens[i] if i < tokens.size() else NO_SUBMODEL
                i += 1
                cab_data[n]["model_relpath"] = _resolve_model_relpath(model_token)
                # cab 1 without a model of its own is the vehicle's model - its controls are looked
                # for there (Train.cpp:10611-10615)
                var models_index:int = _find_label_index(tokens, "models:")
                if n == 1 and model_token.to_lower() == NO_SUBMODEL and not models_index == -1 \
                        and models_index + 1 < tokens.size():
                    cab_data[n]["model_relpath"] = _resolve_model_relpath(tokens[models_index + 1])
            "clock:":
                i += 1 # analog/digital clock type, not an instrument definition
            "pyscreen:":
                # TTrain::screen_entry::deserialize_mapping() (Train.cpp:93): either
                # "{ script target: x updatetime: n parameters: a=1&b=2 }" or the legacy
                # "target script". The script keeps the spelling authored in the MMD.
                var screen := MmdPythonScreenDescriptor.new()
                var script:String = ""
                while i < end_index:
                    var key:String = tokens[i].to_lower()
                    i += 1
                    if key == "}":
                        break
                    if key == "{":
                        script = tokens[i]
                    elif key == "target:":
                        screen.target = tokens[i].to_lower()
                    elif key == "updatetime:":
                        screen.update_time_msec = int(tokens[i])
                    elif key == "parameters:":
                        for pair:String in tokens[i].to_lower().split("&", false):
                            # "$timetable=" pulls another vehicle's timetable in - not ported
                            if not pair.begins_with("$"):
                                screen.parameters[pair.get_slice("=", 0)] = pair.substr(pair.find("=") + 1)
                    else:
                        screen.target = key
                        script = tokens[i]
                        i += 1
                        break
                    i += 1
                # a script given without a directory lives next to the vehicle (Train.cpp:10667)
                var script_base_dir:String = (
                    context.base_dir if not script.get_base_dir()
                    else UserSettings.get_maszyna_game_dir()
                )
                var script_file:String = MaszynaDataPath.resolve(script_base_dir, script + ".py")
                screen.script_path = script_base_dir.path_join(script_file).trim_suffix(".py")
                python_screens.append(screen)
            "pyscreenupdatetime:":
                python_screen_update_time_msec = int(tokens[i])
                i += 1
            "{":
                # DATA QUIRK: a block with no label in front of it - real data has a label that
                # lost its colon, `radiocall3_sw { radio_3 rot 0 0 0 soundinc: ... }`
                # (dynamic/pkp/e186_v2/base.mmd.inc:210). The original only reacts to labels it
                # knows and walks over every other token (TTrain::InitializeCab), so it passes
                # over the whole block. Read token by token here, its `soundinc:` looked like a
                # label and every block after it was taken apart from the wrong end - the rest of
                # the E186 cab (radiostop_sw, universal*, battery_sw, ...) never got built. The
                # block is skipped whole instead.
                while i < end_index and not tokens[i] == "}":
                    i += 1
                i += 1
            _:
                if not label.ends_with(":"):
                    continue # stray value token, not a label - most likely a leftover from a
                    # desync caused by some other unrecognized, non-uniform-shaped label earlier
                    # in this same section (see the comment above start_index/end_index)
                var descriptor := MmdInstrumentDescriptor.new()
                descriptor.label = label.trim_suffix(":")
                descriptor.source_file = abs_mmd_path
                # "i-*:" indicator lights (Train.cpp's TButton::Load(), Button.cpp:40-56) use a
                # completely different, single-token shape ("i-security_aware: czuwak", no
                # animation/scale/offset/friction at all) than every other instrument label -
                # confirmed real and CONFIRMED to previously desync whatever follows it when force-
                # fed through _parse_instrument()'s 5-token read (both fall inside the same
                # cab1definition:/cab0definition: bounds this parser scans).
                # brakes:/eimscreen: name a car and a value of it before the shape (Train.cpp:12147-12166)
                if descriptor.label in LEADING_NUMBER_LABELS:
                    for _number:int in range(LEADING_NUMBER_COUNT):
                        descriptor.leading_numbers.append(int(tokens[i]) if i < end_index else 0)
                        i += 1
                var consumed:int = (
                        _parse_indicator(tokens, i, descriptor, context, abs_mmd_path) if descriptor.label.begins_with("i-")
                        else _parse_instrument(tokens, i, descriptor, context, abs_mmd_path))
                i += consumed
                if descriptor.submodel_name:
                    instruments.append(descriptor)

    var definition := MmdCabinDefinition.new()
    # internaldata:'s cablight: - low power, base and dimmed light, the base one is the cab's
    # (DynObj.cpp:6924-6932), before the cab definitions
    var internal_data_index:int = _find_label_index(tokens, "internaldata:")
    var interior_light_index:int = _find_label_index(tokens, "cablight:", internal_data_index + 1) \
            if not internal_data_index == -1 else -1
    var cab_labels:Array[String] = ["cab0definition:", "cab1definition:", "cab2definition:"]
    var first_cab_index:int = _first_label_index(tokens, cab_labels)
    if not interior_light_index == -1 and interior_light_index < first_cab_index:
        var colours:Array = _read_floats(tokens, interior_light_index + 1 + RGB_COMPONENTS, RGB_COMPONENTS)
        definition.interior_light = Color(
                clampf(colours[0] / RGB_MAX, 0.0, 1.0), clampf(colours[1] / RGB_MAX, 0.0, 1.0),
                clampf(colours[2] / RGB_MAX, 0.0, 1.0))
    var mechspring_index:int = _find_label_index(tokens, "mechspring:")
    if mechspring_index >= 0:
        var values:Array = _read_floats(tokens, mechspring_index + 1, 8)
        definition.shake_spring_stiffness = values[0]
        definition.shake_spring_damping = values[1]
        definition.shake_jolt_scale = Vector3(values[2], values[3], values[4])
        definition.shake_jolt_limit = values[5]
        definition.shake_angle_scale = Vector2(values[6], values[7])
    var enginespring_index:int = _find_label_index(tokens, "enginespring:")
    if enginespring_index >= 0:
        var values:Array = _read_floats(tokens, enginespring_index + 1, 5)
        definition.engine_shake_scale = values[0]
        definition.engine_shake_fade_in_rpm = values[1]
        definition.engine_shake_fade_in_factor = values[2]
        definition.engine_shake_fade_out_rpm = values[3]
        definition.engine_shake_fade_out_factor = values[4]
    definition.cab_number = cab_number
    definition.bounds_min = cab_data[cab_number]["bounds_min"]
    definition.bounds_max = cab_data[cab_number]["bounds_max"]
    definition.driver_pos = cab_data[cab_number]["driver_pos"]
    definition.driver_sitpos = cab_data[cab_number]["driver_sitpos"]
    definition.driver_angle = cab_data[cab_number]["driver_angle"]
    definition.model_relpath = cab_data[cab_number]["model_relpath"]
    definition.instruments = instruments
    # Train.cpp:10729-10740 - the screen's own interval, bounded by the global one, or the
    # vehicle's pyscreenupdatetime: when it has none; below -1 it is taken as it is, unbounded,
    # and -1 stays: the screen is drawn once (TTrain::update_screens(), Train.cpp:10297)
    for screen:MmdPythonScreenDescriptor in python_screens:
        if screen.update_time_msec > 0:
            screen.update_time_msec = maxi(screen.update_time_msec, PYTHON_SCREEN_UPDATE_TIME_MSEC)
        elif screen.update_time_msec == 0:
            screen.update_time_msec = maxi(PYTHON_SCREEN_UPDATE_TIME_MSEC, python_screen_update_time_msec)
        elif screen.update_time_msec < -1:
            screen.update_time_msec = -screen.update_time_msec
    definition.python_screens = python_screens
    # the cab radio's Radio-Stop alarm, internaldata:'s radiostop: (Train.cpp:10340)
    for sound:MmdSoundSourceDefinition in MmdSoundSourceParser.parse_internal_data(abs_mmd_path, context):
        if sound.label == RADIO_STOP_SOUND_LABEL:
            definition.radio_stop_sound = sound
    definition.diagnostics = context.diagnostics
    return definition


## A model can need more than one dynamic-material skin slot. MaSzyna first looks for
## "<skin>,1.mat" and, if present, maps consecutive numbered materials directly to slots 0-3.
## The unnumbered "<skin>.mat" is only the fallback for a single-material model.
static func resolve_skins(data_path:String, skin:String) -> Array:
    if not skin:
        return [skin]
    if skin.contains("|"):
        return Array(skin.split("|", false, 4))
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var resolved_data_path:String = MaszynaDataPath.resolve(game_dir, data_path.trim_prefix("/"))
    var base_dir:String = game_dir.path_join(resolved_data_path)
    var skins:Array = []
    var n:int = 1
    while n <= 4 and _skin_slot_exists(base_dir, "%s,%d" % [skin, n]):
        skins.append("%s,%d" % [skin, n])
        n += 1
    return skins if skins else [skin]


## A slot of a skin is a material or a plain texture (TextureTest(), DynObj.cpp:60 - the wrapper
## reads .mat and .dds of the extensions tried there). Real data: the skins of dynamic/pkp/e186_v2
## are "<skin>,1.dds" and "<skin>,2.dds" with no .mat, next to a 1x1 "<skin>.dds" placeholder.
static func _skin_slot_exists(base_dir:String, slot:String) -> bool:
    for extension:String in [".mat", ".dds"]:
        var relative_path:String = MaszynaDataPath.resolve(base_dir, slot + extension)
        if FileAccess.file_exists(base_dir.path_join(relative_path)):
            return true
    return false


## The (pN) a vehicle's own MMD is read with: the original parses the text
## "include <TypeName>.mmd <name> <TypeName> <skin> end" (DynObj.cpp:5260) - SN61's MMD is only
## "include sn61.mmd.inc (p2)", SM42 6D names its attachments by (p3)
static func vehicle_parameters(vehicle_name:String, type_name:String, skin:String) -> Dictionary:
    var values:Array[String] = [vehicle_name, type_name, skin]
    return _include_parameters(values)


## Whether what the MMD reads depends on the vehicle's own name - its text names (p1), which no
## include of it can name unless passed it. What is read from it then belongs to that vehicle alone.
static func names_vehicle(abs_mmd_path:String) -> bool:
    return FileAccess.get_file_as_bytes(abs_mmd_path).get_string_from_ascii().contains("(p1)")


## Reads just the exterior body model filename from the MMD's own top-level `models:` section
## (e.g. "models: 6da.t3d" as the file's very first line) - this is a DIFFERENT filename than
## the vehicle's shared .fiz/.mmd base name in general (confirmed against real data:
## dynamic/pkp/st44_v2's body model is not named "st44-700"), so MaszynaRailVehicle3D must read
## it from here rather than assuming it equals file_name. Returns "" if the file can't be read
## or has no `models:` section - the caller decides the fallback.
static func parse_body_model(abs_mmd_path:String, parameters:Dictionary) -> String:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var index:int = _find_label_index(tokens, "models:")
    if index == -1 or index + 1 >= tokens.size():
        return ""
    return _resolve_model_relpath(tokens[index + 1])


## Reads the low-poly interior model filename from the MMD's own top-level `models:` section
## (e.g. "lowpolyinterior: 6da_interior.t3d") - the lower-detail interior visible from outside
## the cabin (through windows) before the player enters, matching
## RailVehicleAppearance.low_poly_model_filename. Returns "" if the MMD has no such entry.
static func parse_lowpoly_interior_model(abs_mmd_path:String, parameters:Dictionary) -> String:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var index:int = _find_label_index(tokens, "lowpolyinterior:")
    if index == -1 or index + 1 >= tokens.size():
        return ""
    return _resolve_model_relpath(tokens[index + 1])


## The kinds of animation the MMD's `animations:` line counts, in its order (DynObj.h:32-41)
enum AnimationType { WHEELS, DOORS, LEVERS, BUFFERS, BOGIES, PANTOGRAPHS, STEAM, DOORSTEPS, MIRRORS, WIPERS }

## The labels of a pantograph's elements, in the order the original keeps them (smElement[0..4],
## DynObj.cpp:5404-5575): lower arm 1, lower arm 2, upper arm 1, upper arm 2, slider
const PANTOGRAPH_ELEMENT_LABELS:Array[String] = [
    "animpantrd1prefix:", "animpantrd2prefix:", "animpantrg1prefix:", "animpantrg2prefix:", "animpantslprefix:",
]


## The wheel submodels of the vehicle model: `animwheelprefix:` numbered from 1, as many as the
## `animations:` line declares (DynObj.cpp:5343-5359)
static func parse_wheel_names(abs_mmd_path:String, parameters:Dictionary) -> PackedStringArray:
    var context := MmdImportContext.new()
    return _numbered_names(_tokenize_file(abs_mmd_path, context, parameters), "animwheelprefix:", AnimationType.WHEELS)


## The wipers of the vehicle model: `animwiperprefix:` numbered from 1, as many as the `animations:`
## line declares (DynObj.cpp:5832-5873); each has its elements "_p1", "_p2" and "_p3"
static func parse_wiper_names(abs_mmd_path:String, parameters:Dictionary) -> PackedStringArray:
    var context := MmdImportContext.new()
    return _numbered_names(_tokenize_file(abs_mmd_path, context, parameters), "animwiperprefix:", AnimationType.WIPERS)


## The mirror submodels of the vehicle model: `animmirrorprefix:` numbered from 1, as many as the
## `animations:` line declares (DynObj.cpp:5799-5830)
static func parse_mirror_names(abs_mmd_path:String, parameters:Dictionary) -> PackedStringArray:
    var context := MmdImportContext.new()
    return _numbered_names(_tokenize_file(abs_mmd_path, context, parameters), "animmirrorprefix:", AnimationType.MIRRORS)


## pantfactors: - the first and the second pantograph's place along the vehicle, then their slider
## heights, for a pantograph the model cannot be measured by - with the original's corrections: a
## first height above PANTOGRAPH_FACTOR_MAX_HEIGHT is the second's, and a first place behind the
## centre with the second ahead are both turned (DynObj.cpp:5577-5596); empty without the key
static func parse_pantograph_factors(abs_mmd_path:String, parameters:Dictionary) -> PackedFloat64Array:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var index:int = _find_label_index(tokens, "pantfactors:")
    if index == -1:
        return PackedFloat64Array()
    var values:Array = _read_floats(tokens, index + 1, PANTOGRAPH_FACTOR_COUNT)
    var first_place:float = values[0]
    var second_place:float = values[1]
    var first_height:float = values[2]
    var second_height:float = values[3]
    if first_height > PANTOGRAPH_FACTOR_MAX_HEIGHT:
        first_height = second_height
    if first_place < 0.0 and second_place > 0.0:
        first_place = -first_place
        second_place = -second_place
    return PackedFloat64Array([first_place, second_place, first_height, second_height])


## coupleradapter: - the model of the adapter this vehicle hands a neighbour of another coupler type,
## its length and its height (DynObj.cpp:5284-5292): {"model": String, "length": float, "height":
## float}, empty without the key. The data writes it with commas ("models/tabor/polsprzeg.t3d, 0.085,
## 0.84"); the model is named as RailVehicleRenderingServer takes it: under models/, no extension
static func parse_coupler_adapter(abs_mmd_path:String, parameters:Dictionary) -> Dictionary:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var index:int = _find_label_index(tokens, "coupleradapter:")
    if index == -1 or index + COUPLER_ADAPTER_TOKENS >= tokens.size():
        return {}
    var model:String = tokens[index + 1].trim_suffix(",").replace("\\", "/").trim_prefix("/")
    if model.to_lower().begins_with(MODELS_DIRECTORY):
        model = model.substr(MODELS_DIRECTORY.length())
    return {
        "model": model.get_basename(),
        "length": float(tokens[index + 2].trim_suffix(",")),
        "height": float(tokens[index + 3].trim_suffix(",")),
    }


## The pendulums of the vehicle model: `animpendulumprefix:` numbered 1 to 4 and the
## `pendulumamplitude:` after it [deg] - swinging only while the `animations:` line declares levers
## (DynObj.cpp:1121-1125, 5702-5719): {"names": PackedStringArray, "amplitude": float}
static func parse_pendulums(abs_mmd_path:String, parameters:Dictionary) -> Dictionary:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var pendulums:Dictionary = {"names": PackedStringArray(), "amplitude": 0.0}
    var index:int = _find_label_index(tokens, "animpendulumprefix:")
    if index == -1 or index + 1 >= tokens.size() or _animation_count(tokens, AnimationType.LEVERS) <= 0:
        return pendulums
    var names:PackedStringArray = []
    for number:int in range(1, PENDULUM_COUNT + 1):
        names.append(tokens[index + 1] + str(number))
    pendulums["names"] = names
    if index + 3 < tokens.size() and tokens[index + 2].to_lower() == "pendulumamplitude:":
        pendulums["amplitude"] = float(tokens[index + 3])
    return pendulums


## The door submodels of the vehicle model: `animdoorprefix:` numbered from 1, as many as the
## `animations:` line declares (DynObj.cpp:5721-5760)
static func parse_door_names(abs_mmd_path:String, parameters:Dictionary) -> PackedStringArray:
    var context := MmdImportContext.new()
    return _numbered_names(_tokenize_file(abs_mmd_path, context, parameters), "animdoorprefix:", AnimationType.DOORS)


## The door step submodels: `animstepprefix:` numbered from 1 (DynObj.cpp:5763-5790)
static func parse_door_step_names(abs_mmd_path:String, parameters:Dictionary) -> PackedStringArray:
    var context := MmdImportContext.new()
    return _numbered_names(_tokenize_file(abs_mmd_path, context, parameters), "animstepprefix:", AnimationType.DOORSTEPS)


## The elements of every pantograph of the vehicle model, by PANTOGRAPH_ELEMENT_LABELS: for each
## label its prefix numbered from 1, as many as the `animations:` line declares pantographs
## (DynObj.cpp:5404-5575) - empty for a label the MMD does not declare
static func parse_pantograph_element_names(abs_mmd_path:String, parameters:Dictionary) -> Array[PackedStringArray]:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var elements:Array[PackedStringArray] = []
    for label:String in PANTOGRAPH_ELEMENT_LABELS:
        elements.append(_numbered_names(tokens, label, AnimationType.PANTOGRAPHS))
    return elements


## How many animations of a kind the MMD declares: the `animations:` counts end at their first
## negative number, and every kind after it - or without the line at all - has none (DynObj.cpp:5226-5245)
static func _animation_count(tokens:Array[String], type:AnimationType) -> int:
    var counts_index:int = _find_label_index(tokens, "animations:")
    if counts_index == -1:
        return 0
    for kind:int in range(type + 1):
        var token_index:int = counts_index + 1 + kind
        if token_index >= tokens.size() or int(tokens[token_index]) < 0:
            return 0
    return int(tokens[counts_index + 1 + type])


## A label's prefix numbered from 1 as many times as the MMD declares animations of the kind, as the
## original names them (token + std::to_string(i + 1)); none without the label
static func _numbered_names(tokens:Array[String], label:String, type:AnimationType) -> PackedStringArray:
    var names:PackedStringArray = PackedStringArray()
    var index:int = _find_label_index(tokens, label)
    if index == -1 or index + 1 >= tokens.size():
        return names
    for number:int in range(1, _animation_count(tokens, type) + 1):
        names.append(tokens[index + 1] + str(number))
    return names


## Reads `jointcabs:` from the MMD (DynObj.cpp:6626) - all virtual cabs share one location and
## model, so the whole low-poly cab is hidden from inside any of them.
static func parse_joint_cabs(abs_mmd_path:String, parameters:Dictionary) -> bool:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var index:int = _find_label_index(tokens, "jointcabs:")
    if index == -1 or index + 1 >= tokens.size():
        return false
    return tokens[index + 1].to_lower() in ["true", "yes", "1"]


## The cabNdefinition: of a kind of cabin (CAB_DEFINITIONS); -1 for a kind the MMD has none for
static func cab_definition(kind:RailVehicleCabinKind.Kind) -> int:
    return CAB_DEFINITIONS.get(kind, -1)


## The kinds of cabin the MMD defines a cab for - its cabNdefinition: labels, in CAB_DEFINITIONS'
## order
static func parse_cabin_kinds(abs_mmd_path:String, parameters:Dictionary) -> Array[RailVehicleCabinKind.Kind]:
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var kinds:Array[RailVehicleCabinKind.Kind] = []
    for kind:RailVehicleCabinKind.Kind in CAB_DEFINITIONS:
        if not _find_label_index(tokens, "cab%ddefinition:" % CAB_DEFINITIONS[kind]) == -1:
            kinds.append(kind)
    return kinds


## The MMD's top-level `loads:` block, as the cargo names it maps to their own models. A vehicle
## declares one entry per cargo it can carry (`logs: loads/eaos_vrz-99_logs`), and `passengers` is
## one of those entries - which is why the passenger model comes out of here too. 235 vehicles of
## the datapack declare the block; the rest rely on the model simply being named after the cargo.
static func parse_loads(abs_mmd_path:String, parameters:Dictionary) -> Dictionary[String, String]:
    var models:Dictionary[String, String] = {}
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var loads_index:int = _find_label_index(tokens, "loads:")
    if loads_index == -1 or loads_index + 1 >= tokens.size() or not tokens[loads_index + 1] == "{":
        return models

    var depth:int = 0
    for i:int in range(loads_index + 1, tokens.size()):
        var token:String = tokens[i]
        if token == "{":
            depth += 1
        elif token == "}":
            depth -= 1
            if depth == 0:
                break
        elif depth == 1 and token.ends_with(":") and i + 1 < tokens.size():
            var name:String = token.substr(0, token.length() - 1).to_lower()
            models[name] = _resolve_model_relpath(tokens[i + 1])
    return models


## The models the MMD's `attachments:` draws with the exterior (DynObj.cpp:5384-5398), one per entry
## up to its `}`; an entry "[a b ...]" is one of them picked at random (deserialize_random_set(),
## utilities.cpp:436). The pick is made once for the vehicle's structure, shared by every vehicle of
## the type and skin - no attachment in the datapack is a random set.
static func parse_attachments(abs_mmd_path:String, parameters:Dictionary) -> PackedStringArray:
    var models:PackedStringArray = PackedStringArray()
    var context := MmdImportContext.new()
    var tokens:Array[String] = _tokenize_file(abs_mmd_path, context, parameters)
    var index:int = _find_label_index(tokens, "attachments:")
    if index == -1:
        return models

    var random_set:Array[String] = []
    var in_random_set:bool = false
    for i:int in range(index + 1, tokens.size()):
        var token:String = tokens[i]
        if in_random_set:
            if token == _RANDOM_INCLUDE_CLOSE:
                in_random_set = false
                if random_set:
                    models.append(_resolve_model_relpath(random_set.pick_random()))
                random_set.clear()
            else:
                random_set.append(token)
            continue
        if token == "}":
            break
        if token == _RANDOM_INCLUDE_OPEN:
            in_random_set = true
        elif not token == "{":
            models.append(_resolve_model_relpath(token))
    return models


## Builds real, interactive cabin widgets (CabinButton/CabinSwitch/CabinKnob/CabinGauge) plus
## the cab's own E3D model as children of `generated_root`, which must already be inside the
## scene tree. Appends any build-time diagnostics to `diagnostics` (caller-owned, merged with
## `definition.diagnostics` by MaszynaDynamicTrainCabin.get_diagnostics()).
static func build_into(
        generated_root:Node3D, definition:MmdCabinDefinition, vehicle_rid:RID,
        data_path:String, skin:String, diagnostics:Array[Dictionary]) -> void:
    # the cab's radio, a model or none: at the middle of the cab a metre up until its lamp places it
    # (Train.cpp:10730-10739)
    var radio := CabinRadio3D.new()
    radio.name = "Radio"
    radio.position = (definition.bounds_min + definition.bounds_max) * 0.5 + Vector3.UP
    if definition.radio_stop_sound:
        radio.radio_stop_alarm = MmdSoundEventBuilder.build(definition.radio_stop_sound, CabinRadio3D.RADIO_STOP)
        radio.radio_stop_alarm.spatial_config = null
    generated_root.add_child(radio)
    radio.set_vehicle_rid(vehicle_rid)
    if not definition.model_relpath:
        # Valid in the original (e.g. su46 cab0) - the low-poly interior is shown instead.
        diagnostics.append(_diag("info", "MMD_MODEL_NOT_FOUND", "Cab %d has no model (model: none)" % definition.cab_number, definition.cab_number))
        return

    # where the cab's sounds are looked for first (Global.asCurrentDynamicPath, audio.cpp:115)
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var vehicle_dir:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, data_path.trim_prefix("/")))
    var model := E3DModelInstance.new()
    model.name = "CabModel"
    # the original loads a cab as a dynamic model (Train.cpp:10599), which hides its "_on" controls
    model.instance_kind = E3DRenderingServer.INSTANCE_KIND_DYNAMIC
    model.data_path = data_path
    # the resource itself rather than its filename, so the indicator lights can read its submodels
    model.model = E3DModelManager.load_model(data_path, definition.model_relpath)
    model.skins = resolve_skins(data_path, skin)
    # the cab loads under its own texture size limit (Train.cpp:660)
    model.max_texture_size = int(ProjectSettings.get_setting("maszyna/import/dds_max_cab_texture_size", 4096))
    # the cab's own sun lights this layer alone (maszyna/cabin/improve_shadows_quality)
    model.layers = MaszynaEnvironmentNode.CABIN_RENDER_LAYER
    # A cabin interior is self-contained (glass, instrument backlight glow, ...) and, unlike
    # mixed-purpose exterior E3D content, alpha-scissor's crisp cutout looks wrong across the
    # board here - real alpha blending for every already-transparent-flagged submodel instead.
    model.force_alpha = true
    # Must be set before add_child() triggers the actual E3D build (E3DModelInstance._ready()) -
    # the resolved submodel needs real alpha blending from the moment its material is first
    # created, not as a later refresh.
    model.force_alpha_submodel_paths = _resolve_force_alpha_submodel_paths(
            data_path, definition.model_relpath, definition.instruments)
    generated_root.add_child(model)

    if not model.is_e3d_loaded():
        diagnostics.append(_diag(
            "error", "MMD_MODEL_NOT_FOUND",
            "Could not load cab model '%s'" % definition.model_relpath,
            definition.cab_number
        ))
        return

    var submodel_index:Dictionary = {}
    _index_submodels(model, submodel_index)

    # One bank for every control of this cab, rebuilt with it; each control's sounds are its events
    var sound_bank := SfxBank.new()
    var sound_player := SfxPlayer3D.new()
    sound_player.name = "CabinControlsSfxPlayer3D"
    sound_player.bus = &"Cabin"
    sound_player.max_tracks = CONTROL_SOUND_VOICE_COUNT
    sound_player.bank = sound_bank
    generated_root.add_child(sound_player)
    var sound_events:Array[SfxEvent] = []

    for descriptor:MmdInstrumentDescriptor in definition.instruments:
        if not MmdSemanticCatalog.has_label(descriptor.label):
            diagnostics.append(_diag("info", "MMD_BINDING_UNSUPPORTED", "MMD label '%s' is not in the supported catalog" % descriptor.label, definition.cab_number, descriptor.label, descriptor.submodel_name))
            continue
        var entry:Dictionary = MmdSemanticCatalog.get_entry(descriptor.label)
        if entry.get("position_at_submodel", false):
            # Some real cabins have multiple physical lamp housings sharing the SAME MMD-declared
            # submodel base name (confirmed real: sm_42_cabin.tscn's own hand-authored reference
            # has 3 "CzuwakOmni" lights for its one "i-security_aware:" label) - one widget per
            # matched submodel instance, not just the first, unlike every other instrument label
            # (which only ever has one real target mesh).
            _build_indicator_lights(
                    descriptor, entry, vehicle_rid, submodel_index, model, generated_root,
                    definition.cab_number, definition.driver_pos, sound_player, sound_events, vehicle_dir,
                    diagnostics)
            continue
        # a label repeated in one cab (EP07 cab0 has two cablight_sw switches) is one control with
        # one state in the original (e.g. "cablight_sw:" -> Cabine[].bLight, Train.cpp:10237): its
        # key is the cab logic's, once, and every widget of it follows the cabin state
        var widget:Node = _build_widget(descriptor, vehicle_rid, definition.cab_number, diagnostics)
        generated_root.add_child(widget)
        # mesh_path must be resolved AFTER the widget has a place in the tree - the widget shares
        # no common ancestor with `model`'s submodels until it's actually parented under the same
        # generated_root.
        _wire_mesh_path(widget, descriptor, submodel_index, entry["mesh_path_field"], definition.cab_number, diagnostics)
        # a control sounds where its submodel is, or at the cab's origin without one (Gauge.cpp:75-95)
        var mesh_path:NodePath = widget.get(entry["mesh_path_field"])
        var sound_position:Vector3 = (
                generated_root.to_local((widget.get_node(mesh_path) as Node3D).global_position) if mesh_path
                else Vector3.ZERO)
        _apply_sound(widget, descriptor, sound_player, sound_position, sound_events, vehicle_dir)
        widget.set_vehicle_rid(vehicle_rid)
        # A gauge's own lamp: TGauge takes "<name>_on" as the lit state of the control, shown
        # instead of the control while the flag its entry names is set (Gauge.cpp:204-210, 386-392)
        var on_matches:Array = submodel_index.get(descriptor.submodel_name.validate_node_name().to_lower() + ON_SUFFIX, [])
        if entry.has("state_light") and on_matches:
            var lamp := CabinIndicator3D.new()
            lamp.name = "%s_%s_on" % [descriptor.label, descriptor.submodel_name]
            for field_name:String in entry["state_light"]:
                lamp.set(field_name, entry["state_light"][field_name])
            generated_root.add_child(lamp)
            lamp.on_target_path = lamp.get_path_to(on_matches[0])
            if widget.get("mesh_path"):
                lamp.off_target_path = lamp.get_path_to(widget.get_node(widget.get("mesh_path")))
            lamp.set_vehicle_rid(vehicle_rid)

    # the gauges of the train's cars read their pressures from it (Train.cpp:8670-8680)
    var pressure_keys:PackedStringArray = []
    for descriptor:MmdInstrumentDescriptor in definition.instruments:
        if MmdSemanticCatalog.has_label(descriptor.label) \
                and MmdSemanticCatalog.get_entry(descriptor.label).has("state_property_of_leading_numbers") \
                and descriptor.leading_numbers.size() == LEADING_NUMBER_COUNT:
            pressure_keys.append(LegacyCabinTrainsetPressures.state_key(
                    descriptor.leading_numbers[0], descriptor.leading_numbers[1]))
    if pressure_keys:
        var pressures := LegacyCabinTrainsetPressures.new()
        pressures.name = "TrainsetPressures"
        pressures.state_keys = pressure_keys
        generated_root.add_child(pressures)
        pressures.set_vehicle_rid(vehicle_rid)

    # the radio sounds at its lamp (btLampkaRadio.model_offset(), Train.cpp:10734)
    for descriptor:MmdInstrumentDescriptor in definition.instruments:
        if descriptor.label == RADIO_LAMP_LABEL:
            var base_name:String = descriptor.submodel_name.validate_node_name().to_lower()
            var lamps:Array = submodel_index.get(base_name + ON_SUFFIX, []) + submodel_index.get(base_name + OFF_SUFFIX, [])
            if lamps:
                radio.position = generated_root.to_local((lamps[0] as Node3D).global_position)

    for descriptor:MmdPythonScreenDescriptor in definition.python_screens:
        if not FileAccess.file_exists(descriptor.script_path + ".py"):
            diagnostics.append(_diag("warning", "MMD_PYTHON_SCRIPT_NOT_FOUND", "Python screen script '%s.py' not found" % descriptor.script_path, definition.cab_number, "pyscreen", descriptor.target))
            continue
        var mesh:MeshInstance3D = null
        if not descriptor.target == "none":
            # Train.cpp:10676-10689 - a screen whose submodel is missing or has no texture is dropped
            var matches:Array = submodel_index.get(descriptor.target.validate_node_name().to_lower(), [])
            mesh = matches[0] as MeshInstance3D if matches else null
            if not mesh or not mesh.material_override is ShaderMaterial:
                diagnostics.append(_diag("warning", "MMD_SUBMODEL_NOT_FOUND", "Python screen submodel '%s' not found or has no texture" % descriptor.target, definition.cab_number, "pyscreen", descriptor.target))
                continue
        var screen := CabinPythonScreen.new()
        screen.name = "PythonScreen_" + descriptor.target.validate_node_name()
        screen.mesh = mesh
        screen.vehicle_rid = vehicle_rid
        screen.script_path = descriptor.script_path
        screen.parameters = descriptor.parameters
        screen.update_time_msec = descriptor.update_time_msec
        generated_root.add_child(screen)

    sound_bank.events = sound_events


static func _empty_cab_data() -> Dictionary:
    return {
        "bounds_min": Vector3.ZERO,
        "bounds_max": Vector3.ZERO,
        "driver_pos": Vector3.ZERO,
        "driver_sitpos": Vector3.ZERO,
        "driver_angle": Vector2.ZERO,
        "model_relpath": "",
    }


static func _diag(severity:String, code:String, message:String, cab_number:int, mmd_label:String = "", submodel_name:String = "") -> Dictionary:
    return {
        "severity": severity,
        "code": code,
        "source_file": "",
        "line": 0,
        "cabin_number": cab_number,
        "mmd_label": mmd_label,
        "submodel_name": submodel_name,
        "message": message,
    }


## The first of `labels` in the tokens; tokens.size() without any
static func _first_label_index(tokens:Array[String], labels:Array[String]) -> int:
    var first:int = tokens.size()
    for label:String in labels:
        var index:int = _find_label_index(tokens, label)
        if not index == -1:
            first = mini(first, index)
    return first


static func _find_label_index(tokens:Array[String], needle:String, from:int = 0) -> int:
    for i in range(from, tokens.size()):
        if tokens[i].to_lower() == needle:
            return i
    return -1


static func _read_floats(tokens:Array[String], start_i:int, count:int) -> Array:
    var result:Array = []
    for k in range(count):
        var idx:int = start_i + k
        result.append(float(tokens[idx]) if idx < tokens.size() else 0.0)
    return result


## Parses one instrument line's raw shape - `submodel animation scale offset friction`, or the
## `{ submodel animation scale offset friction  type: ...  ... }` block form - starting at
## `tokens[i]`. Returns the number of tokens consumed so the caller's index stays in sync even
## for a label this parser doesn't otherwise understand.
static func _parse_instrument(
        tokens:Array[String], i:int, descriptor:MmdInstrumentDescriptor,
        context:MmdImportContext, source_file:String) -> int:
    var start:int = i
    var in_block:bool = false
    if i < tokens.size() and tokens[i] == "{":
        in_block = true
        i += 1

    if i + 5 > tokens.size():
        context.add_diagnostic(
                "error", "MMD_INVALID_CAB_DEFINITION",
                "Truncated instrument definition for '%s'" % descriptor.label, source_file, 0, descriptor.label)
        return tokens.size() - start

    descriptor.submodel_name = tokens[i]
    descriptor.animation_type = tokens[i + 1].to_lower()
    descriptor.scale = float(tokens[i + 2])
    descriptor.offset = float(tokens[i + 3])
    descriptor.friction = float(tokens[i + 4])
    i += 5

    if descriptor.animation_type in _VARIABLE_ANIMATION_TYPES:
        descriptor.end_value = float(tokens[i]) if i < tokens.size() else 0.0
        descriptor.end_scale = float(tokens[i + 1]) if i + 1 < tokens.size() else 0.0
        i += 2

    if in_block:
        while i < tokens.size() and tokens[i] != "}":
            var token_lower:String = tokens[i].to_lower()
            if token_lower == "type:" and i + 1 < tokens.size():
                descriptor.button_type = tokens[i + 1].to_lower()
                i += 2
            elif token_lower == "soundinc:":
                var result:Dictionary = _read_sound_field_value(tokens, i + 1, context, source_file, "soundinc")
                descriptor.sound_increase = result["value"]
                i += 1 + int(result["consumed"])
            elif token_lower == "sounddec:":
                var result:Dictionary = _read_sound_field_value(tokens, i + 1, context, source_file, "sounddec")
                descriptor.sound_decrease = result["value"]
                i += 1 + int(result["consumed"])
            else:
                var position:Variant = _parse_sound_position_label(token_lower)
                if position != null:
                    var result:Dictionary = _read_sound_field_value(tokens, i + 1, context, source_file, token_lower)
                    descriptor.sound_positions[position] = result["value"]
                    i += 1 + int(result["consumed"])
                else:
                    i += 1
        if i < tokens.size():
            i += 1 # consume "}"

    return i - start


## Parses an "i-*:" indicator light's shape (Train.cpp's TButton::Load(), Button.cpp:40-56): a
## bare submodel base name, OR a `{ submodel soundinc: ... sounddec: ... }` block whose FIRST
## token (not the label itself) is the submodel name - the original engine peeks for "{" BEFORE
## reading anything (Button.cpp:44-48: `if (Parser.peek() != "{") { Parser >> submodelname; } else
## { submodelname = Parser.getToken(...); ... }`), so the submodel name always comes from INSIDE
## the block in block form, never before it. Confirmed real and required: dynamic/pkp/su45_v2/
## 301d.mmd uses exactly this block shape ("i-security_aware: { i-czuwak soundinc: ... sounddec:
## ... }"), which a "submodel-name-then-optional-block" read (every other instrument label's
## order) misparses as submodel_name="{" and never enters the block at all - not the "submodel
## animation scale offset friction" shape every other instrument label uses (the original engine
## shows/hides a matching "<name>_on"/"<name>_off" submodel pair rather than animating one).
## MmdSemanticCatalog widgets for these labels ignore animation_type/scale/offset/friction
## entirely (left at their MmdInstrumentDescriptor defaults).
static func _parse_indicator(
        tokens:Array[String], i:int, descriptor:MmdInstrumentDescriptor,
        context:MmdImportContext, source_file:String) -> int:
    var start:int = i
    if i >= tokens.size():
        context.add_diagnostic(
                "error", "MMD_INVALID_CAB_DEFINITION",
                "Truncated indicator definition for '%s'" % descriptor.label, source_file, 0, descriptor.label)
        return 0

    if tokens[i] != "{":
        descriptor.submodel_name = tokens[i]
        return i + 1 - start

    i += 1 # consume "{"
    if i < tokens.size():
        descriptor.submodel_name = tokens[i]
        i += 1

    while i < tokens.size() and tokens[i] != "}":
        var token_lower:String = tokens[i].to_lower()
        if token_lower == "soundinc:":
            var result:Dictionary = _read_sound_field_value(tokens, i + 1, context, source_file, "soundinc")
            descriptor.sound_increase = result["value"]
            i += 1 + int(result["consumed"])
        elif token_lower == "sounddec:":
            var result:Dictionary = _read_sound_field_value(tokens, i + 1, context, source_file, "sounddec")
            descriptor.sound_decrease = result["value"]
            i += 1 + int(result["consumed"])
        else:
            i += 1
    if i < tokens.size():
        i += 1 # consume "}"

    return i - start


## Returns the position for a "soundN:"/"sound-N:" field label (e.g. "sound-3:" -> -3, "sound0:"
## -> 0), or null if `label` isn't in that shape (rules out "soundinc:"/"sounddec:"/"soundmain:"/
## "type:", none of which have a valid-int remainder after stripping "sound" and ":").
static func _parse_sound_position_label(label:String) -> Variant:
    if not label.begins_with("sound") or not label.ends_with(":"):
        return null
    var middle:String = label.substr(5, label.length() - 6)
    if not middle or not middle.is_valid_int():
        return null
    return int(middle)


## Reads one sound field's value starting at tokens[i] (right after its "soundX:" label) - a bare
## filename, a bracketed random-choice list (resolved to one entry, frozen via
## context.random_choices exactly like MMD's random `include` lists), or a nested `{ ... }`
## sub-block (only its soundmain: is kept; other sub-fields are reported and discarded). Returns
## {"value": normalized filename ("" if absent), "consumed": token count NOT including tokens[i]
## itself - i.e. the caller's index should advance by 1 (the label) + this "consumed"}.
static func _read_sound_field_value(
        tokens:Array[String], i:int, context:MmdImportContext, source_file:String, field_key:String) -> Dictionary:
    if i >= tokens.size():
        return {"value": "", "consumed": 0}

    if tokens[i] == "[":
        var candidates:Array[String] = []
        var j:int = i + 1
        while j < tokens.size() and tokens[j] != "]":
            candidates.append(tokens[j])
            j += 1
        if j < tokens.size():
            j += 1 # consume "]"
        if not candidates:
            return {"value": "", "consumed": j - i}
        var choice_key:String = "%s#%s#%s" % [source_file, field_key, "|".join(candidates)]
        if not context.random_choices.has(choice_key):
            context.random_choices[choice_key] = candidates[randi() % candidates.size()]
        return {"value": _normalize_sound_filename(context.random_choices[choice_key]), "consumed": j - i}

    if tokens[i] == "{":
        var soundmain:String = ""
        var j:int = i + 1
        while j < tokens.size() and tokens[j] != "}":
            if tokens[j].to_lower() == "soundmain:" and j + 1 < tokens.size():
                soundmain = tokens[j + 1]
                j += 2
            else:
                j += 1
        if j < tokens.size():
            j += 1 # consume "}"
        context.add_diagnostic(
                "info", "MMD_ANIMATION_UNSUPPORTED",
                "Sound field '%s' uses a nested sub-block - only soundmain: is used, other parameters (amplitudefactor/range/etc.) are ignored" % field_key,
                source_file, 0, field_key)
        return {"value": _normalize_sound_filename(soundmain), "consumed": j - i}

    return {"value": _normalize_sound_filename(tokens[i]), "consumed": 1}


## Strips a trailing ".wav"/".ogg"/".flac" - everything else (including a "[NNNN]" numeric prefix, which
## is confirmed to be part of the literal filename on disk) is kept verbatim.
static func _normalize_sound_filename(token:String) -> String:
    if not token:
        return ""
    var lower:String = token.to_lower()
    if lower.ends_with(".wav") or lower.ends_with(".ogg"):
        return token.substr(0, token.length() - 4)
    # the game data ships every sound as .ogg, also those an MMD still names *.flac
    if lower.ends_with(".flac"):
        return token.substr(0, token.length() - 5)
    return token


## Strips ".t3d"/".e3d" and normalizes backslashes. Case-insensitive filesystem resolution
## (feasibility doc section 3.1's 59 non-matching-case models) is not attempted here - if the
## exact case doesn't resolve, E3DModelInstance.reload() simply fails to load and build_into()
## reports MMD_MODEL_NOT_FOUND, same as any other missing model.
##
## Real data (dynamic/pkp/st44_v2/st44.mmd.inc) has model tokens glued directly to a trailing
## "#" with no space (e.g. "main/(p1).t3d#") - strip it before touching the extension, or the
## ".t3d"/".e3d" suffix check below never matches and the resolved path is left corrupted.
static func _resolve_model_relpath(model_token:String) -> String:
    if not model_token or model_token.to_lower() == "none":
        return ""
    var normalized:String = model_token.replace("\\", "/")
    if normalized.ends_with("#"):
        normalized = normalized.substr(0, normalized.length() - 1)
    var lower:String = normalized.to_lower()
    if lower.ends_with(".t3d") or lower.ends_with(".e3d"):
        normalized = normalized.substr(0, normalized.length() - 4)
    return normalized


## Resolves E3DModelInstance.force_alpha_submodel_paths for the on/off submodel pairs backing
## "i-*:" indicator descriptors whose MmdSemanticCatalog entry has "force_alpha" set (currently
## just i-instrumentlight - see mmd_semantic_catalog.gd). Runs BEFORE the cab's E3DModelInstance
## is built, by loading the same (cached) E3DModel resource it will use, so the resolved paths
## can be assigned before add_child() triggers the actual build.
##
## Walks the submodel tree at most once (via _index_submodel_paths()), skipped entirely when no
## instrument in this cab needs it - a per-name recursive search repeated per label/suffix would
## re-walk the tree from the root every time instead.
static func _resolve_force_alpha_submodel_paths(
        data_path:String, model_relpath:String, instruments:Array[MmdInstrumentDescriptor]) -> Array[NodePath]:
    var paths:Array[NodePath] = []
    var needs_force_alpha:bool = instruments.any(
            func(descriptor:MmdInstrumentDescriptor) -> bool:
                return (
                        descriptor.label.begins_with("i-") and MmdSemanticCatalog.has_label(descriptor.label)
                        and MmdSemanticCatalog.get_entry(descriptor.label).get("force_alpha", false)))
    if not needs_force_alpha:
        return paths

    var e3d_model:E3DModel = E3DModelManager.load_model(data_path, model_relpath)
    if not e3d_model:
        return paths

    var path_index:Dictionary = {}
    _index_submodel_paths(e3d_model.submodels, path_index)

    for descriptor:MmdInstrumentDescriptor in instruments:
        if not descriptor.label.begins_with("i-") or not MmdSemanticCatalog.has_label(descriptor.label):
            continue
        if not MmdSemanticCatalog.get_entry(descriptor.label).get("force_alpha", false):
            continue
        for suffix:String in ["_on", "_off"]:
            var path:NodePath = path_index.get((descriptor.submodel_name + suffix).to_lower(), NodePath(""))
            if path:
                paths.append(path)
    return paths


## Single-pass equivalent of _index_submodels() (below), but over the E3DSubModel resource tree
## before it is instantiated, keyed by lowercased name to a NodePath from the model root instead
## of by node reference. First match wins on a name collision, same as a DFS "find by name" would.
static func _index_submodel_paths(submodels:Array, index:Dictionary, path_prefix:String = "") -> void:
    for submodel:E3DSubModel in submodels:
        var current_path:String = (
                path_prefix.path_join(submodel.resource_name) if path_prefix else submodel.resource_name)
        var name_lower:String = submodel.resource_name.to_lower()
        if not index.has(name_lower):
            index[name_lower] = NodePath(current_path)
        if submodel.submodels:
            _index_submodel_paths(submodel.submodels, index, current_path)


## Indexes by node reference, not NodePath - the correct mesh_path (a path FROM the widget TO
## the submodel) can only be computed once the widget itself has a place in the tree, which
## happens later, in _wire_mesh_path().
##
## The NODES instancer adds every submodel node as an INTERNAL child (INTERNAL_MODE_BACK, since
## `editable` is false at runtime) - get_children() without `true` silently returns none of them,
## making every lookup fail.
##
## Indexed by LOWERCASED name, matching the original engine's own TSubModel::GetFromName(search,
## i=true), which is case-insensitive by default. Confirmed necessary against real data: ST44's
## submodel names happen to match MMD's declared case exactly ("nastawnik"/"zasadniczy"), but
## su45_v2/kabina-su45-a.e3d's brake gauge submodels are actually "przglknob06"/"przglknob05"
## while 301d.mmd declares them "PrzGlKnob06"/"PrzGlKnob05" - a case-sensitive lookup silently
## fails to bind these, leaving those gauges dead with no diagnostic (not matches still
## fires correctly, but only after realizing the exact-case assumption was wrong).
static func _index_submodels(node:Node, index:Dictionary) -> void:
    for child:Node in node.get_children(true):
        var child_name:String = child.name.to_lower()
        if not index.has(child_name):
            index[child_name] = []
        index[child_name].append(child)
        _index_submodels(child, index)


## MMD `type:` -> the control's gauge type, as TGauge::Load reads it (Gauge.cpp:243); no `type:`
## is a toggle (Gauge.h:89)
## An instrument's submodel of "none": a control with no model of its own
const NO_SUBMODEL:String = "none"

const BUTTON_TYPES:Dictionary[String, CabinButton.ButtonType] = {
    "push": CabinButton.ButtonType.PUSH,
    "impulse": CabinButton.ButtonType.PUSH,
    "return": CabinButton.ButtonType.PUSH,
    "delayed": CabinButton.ButtonType.PUSH_DELAYED,
    "pushtoggle": CabinButton.ButtonType.PUSH_TOGGLE,
    "toggle": CabinButton.ButtonType.TOGGLE,
}


static func _build_widget(
        descriptor:MmdInstrumentDescriptor, vehicle_rid:RID,
        cab_number:int, diagnostics:Array[Dictionary]) -> Node:
    var entry:Dictionary = MmdSemanticCatalog.get_entry(descriptor.label)
    var widget:Node = entry["widget_class"].new()
    widget.name = "%s_%s" % [descriptor.label, descriptor.submodel_name]
    if "control_id" in widget:
        widget.control_id = StringName(descriptor.label)

    var button_type:CabinButton.ButtonType = BUTTON_TYPES.get(descriptor.button_type, CabinButton.ButtonType.TOGGLE)
    var fields:Dictionary = MmdSemanticCatalog.resolve_fields(
            descriptor.label, button_type, CabinSystem.vehicle_config(vehicle_rid))
    for field_name:String in fields:
        widget.set(field_name, fields[field_name])
    # the keys are the cab logic's (LegacyCabinLogic); the widget only names them under its caption
    if "hint_actions" in widget:
        var hint_actions:PackedStringArray = []
        for field_name:String in LegacyCabinControls.ACTION_FIELDS:
            if fields.get(field_name, ""):
                hint_actions.append(fields[field_name])
        widget.hint_actions = hint_actions
    if "target" in widget:
        widget.target = entry.get("target", CabinState.Target.OCCUPIED)

    # names of positions that lie where the vehicle says - a brake valve's, per its handle type
    var position_names_config:Dictionary = entry.get("position_names_config", {})
    if position_names_config and "position_names" in widget:
        var names:Dictionary = {}
        var config:Dictionary = CabinSystem.vehicle_config(vehicle_rid)
        for config_key:String in position_names_config:
            if config.has(config_key):
                names[roundi(float(config[config_key]))] = position_names_config[config_key]
        widget.set("position_names", names)

    if widget is CabinButton:
        widget.button_type = button_type
    # a gauge of a car of the train reads the value its numbers name (LegacyCabinTrainsetPressures)
    if entry.has("state_property_of_leading_numbers") and descriptor.leading_numbers.size() == LEADING_NUMBER_COUNT:
        widget.set("state_property", LegacyCabinTrainsetPressures.state_key(
                descriptor.leading_numbers[0], descriptor.leading_numbers[1]))

    # "i-*:" indicator descriptors (see _parse_indicator()) never set animation_type - they have
    # no "rot"/"mov" shape at all, so there's nothing for _apply_animation_shape() to compute.
    if descriptor.animation_type:
        _apply_animation_shape(widget, descriptor, entry, vehicle_rid, cab_number, diagnostics)

    return widget


## Adds the MMD soundinc:/sounddec:/soundN: of a control to the cab's bank as events and hands the
## widget the player and the names of its events. Duck-typed the same way mesh_path/target_mesh_path
## are: CabinButton and CabinSpotLight3D take on/off, CabinKnob an event per position, CabinSwitch
## the override lists. CabinGauge has no sound, so this is a no-op for it.
static func _apply_sound(
        widget:Node, descriptor:MmdInstrumentDescriptor, sound_player:SfxPlayer3D,
        sound_position:Vector3, events:Array[SfxEvent], vehicle_dir:String) -> void:
    if not "sound_player" in widget:
        return
    widget.set("sound_player", sound_player)
    var increase:StringName = _add_control_sound(
            events, widget, "increase", descriptor.sound_increase, sound_position, vehicle_dir)
    var decrease:StringName = _add_control_sound(
            events, widget, "decrease", descriptor.sound_decrease, sound_position, vehicle_dir)
    if "sound_on_event" in widget:
        widget.set("sound_on_event", increase)
        widget.set("sound_off_event", decrease)
        return

    widget.set("sound_increase_event", increase)
    widget.set("sound_decrease_event", decrease)
    if "sound_position_events" in widget:
        var position_events:Dictionary[int, StringName] = {}
        for position:int in descriptor.sound_positions:
            var event:StringName = _add_control_sound(
                    events, widget, "position_%d" % position, descriptor.sound_positions[position], sound_position,
                    vehicle_dir)
            if event:
                position_events[position] = event
        widget.set("sound_position_events", position_events)
        return

    var positive:Array[StringName] = []
    var negative:Array[StringName] = []
    for position:int in descriptor.sound_positions:
        if position == 0:
            continue
        var position_events:Array[StringName] = positive if position > 0 else negative
        var index:int = absi(position) - 1
        if position_events.size() <= index:
            position_events.resize(index + 1)
        position_events[index] = _add_control_sound(
                events, widget, "position_%d" % position, descriptor.sound_positions[position], sound_position,
                vehicle_dir)
    widget.set("sound_override_events", positive)
    widget.set("sound_override_negative_events", negative)


## One event of the cab's bank - a control's one-shot, sounding at `sound_position`. Returns its
## name, or an empty one when the MMD gives no file.
static func _add_control_sound(
        events:Array[SfxEvent], widget:Node, sound_case:String, filename:String,
        sound_position:Vector3, vehicle_dir:String) -> StringName:
    if not filename:
        return &""
    var clip := SfxClip.new()
    clip.stream = MmdSoundEventBuilder.build_stream(filename, false, vehicle_dir)
    var clips:Array[SfxClip] = [clip]
    var event := SfxEvent.new()
    event.name = StringName("%s_%s" % [widget.name, sound_case])
    event.clips = clips
    event.spatial_config = SfxSpatialConfig.new()
    event.spatial_config.position = sound_position
    event.spatial_config.max_distance = CONTROL_SOUND_MAX_DISTANCE
    var emitter:Array[SfxEvent] = [event]
    MmdSoundEventBuilder.shape_emitter(emitter, null, 0.0)
    events.append(event)
    return event.name


## Sets the widget's mesh_rotation/mesh_position "full-swing" target directly from this vehicle's
## own MMD scale/animation_type - cabin geometry differs per vehicle, so this can't be a fixed
## per-label constant (see MmdSemanticCatalog's header comment). Matches the original engine's
## TGauge formula evaluated at value=1: `rot` -> rotation.y = scale*360 degrees; `mov` ->
## position.z = scale (MMD's own local Z axis convention). `offset` is a constant baseline shift,
## independent of value, so it is written as-is (offset*360 for `rot`, offset for `mov`) into the
## widget's mesh_rotation_offset/mesh_position_offset field - unlike `scale` it is never rescaled
## by range_scale (it doesn't depend on the value domain) or by mmd_scale_multiplier (the original
## engine's own gauge.Load(..., mul) only multiplies scale, per vehicle/Gauge.cpp:182). Widgets
## without an offset field (e.g. CabinSwitch) still report it as unsupported instead
## of silently dropping it.
##
## `entry["animation_range_config_properties"]`, when present, is `[min_key, max_key]` into
## CabinSystem.vehicle_config(vehicle_rid) - MMD's scale is calibrated against MaSzyna's raw value domain for a
## property (e.g. RailVehicleBrake's fBrakeCtrlPos), but the widget may be bound to an already-
## normalized (0..1) state_property instead (brakectrl: the command it sends,
## RailVehicleBrake::brake_level_set, itself expects a normalized level, so the widget's value/command
## domain has to stay normalized even though that's not what MMD's scale assumes). Multiplying
## the raw-domain-derived degrees/position by (max-min) - the same range the state's own
## normalization divides by - converts it to the correct per-normalized-unit amount without
## having to change the widget's value domain (and therefore without breaking its command).
##
## `entry["mmd_scale_multiplier"]` (default 1.0) mirrors the original engine's own
## TGauge::Load(..., mul) parameter (vehicle/Gauge.cpp:182, `scale *= mul`): Train.cpp calls
## gauge.Load(..., 0.1) specifically for pressure-family gauge labels (brakepress, pipepress,
## scndpress, compressor, ...), while every other gauge label (confirmed: tachometer, oilpress)
## uses the implicit default of 1.0. This is the original engine's own fixed per-label
## correction factor, not a per-vehicle guess - confirmed by reading vehicle/Train.cpp directly.
static func _apply_animation_shape(
        widget:Node, descriptor:MmdInstrumentDescriptor, entry:Dictionary, vehicle_rid:RID,
        cab_number:int, diagnostics:Array[Dictionary]) -> void:
    var range_scale:float = 1.0
    var range_properties:Array = entry.get("animation_range_config_properties", [])
    var config:Dictionary = CabinSystem.vehicle_config(vehicle_rid)
    # the range is the vehicle's - one without the component that has it gives none
    if range_properties.size() == 2 and config.has(range_properties[0]) and config.has(range_properties[1]):
        var range_min:float = float(config[range_properties[0]])
        var range_max:float = float(config[range_properties[1]])
        range_scale = range_max - range_min
        # the same raw range is where the knob's whole positions lie (a brake valve's BCPN rows)
        if "position_min" in widget:
            widget.set("position_min", range_min)
            widget.set("position_max", range_max)

    var mmd_scale:float = descriptor.scale * float(entry.get("mmd_scale_multiplier", 1.0))

    # rotvar/movvar turn and slide as rot/mov with a scale that runs to their end scale, multiplied
    # alike (Gauge.cpp:116-141, 182-185); wip turns as rot, and its two submodels below with it
    var shape:String = descriptor.animation_type
    if shape in _VARIABLE_ANIMATION_TYPES and "variable_end_value" in widget:
        widget.set("variable_end_value", descriptor.end_value)
        widget.set("variable_end_scale", descriptor.end_scale / descriptor.scale if not is_zero_approx(descriptor.scale) else 1.0)
    if shape == "wip" and "wiper_chain" in widget:
        widget.set("wiper_chain", true)
    match shape:
        "dgt":
            if "animation_type" in widget:
                widget.set("animation_type", CabinGauge.AnimationType.DIGITAL)
                widget.set("digital_scale", mmd_scale)
                widget.set("digital_offset", descriptor.offset)
            else:
                diagnostics.append(_diag(
                        "info", "MMD_ANIMATION_UNSUPPORTED",
                        "Label '%s' uses 'dgt' but its widget is no gauge" % descriptor.label,
                        cab_number, descriptor.label, descriptor.submodel_name))
        "rot", "rotvar", "wip":
            if "mesh_rotation" in widget:
                var rotation_vec:Vector3 = widget.get("mesh_rotation")
                rotation_vec.y = mmd_scale * 360.0 * range_scale
                widget.set("mesh_rotation", rotation_vec)
                if "mesh_rotation_offset" in widget:
                    var rotation_offset_vec:Vector3 = widget.get("mesh_rotation_offset")
                    rotation_offset_vec.y = descriptor.offset * 360.0
                    widget.set("mesh_rotation_offset", rotation_offset_vec)
                elif not is_zero_approx(descriptor.offset):
                    diagnostics.append(_diag(
                            "info", "MMD_ANIMATION_UNSUPPORTED",
                            "Label '%s' has a non-zero MMD offset (%s) but its widget has no mesh_rotation_offset field - ignored" % [descriptor.label, descriptor.offset],
                            cab_number, descriptor.label, descriptor.submodel_name))
            else:
                diagnostics.append(_diag(
                        "info", "MMD_ANIMATION_UNSUPPORTED",
                        "Label '%s' uses 'rot' but its widget has no mesh_rotation field" % descriptor.label,
                        cab_number, descriptor.label, descriptor.submodel_name))
        "mov", "movvar":
            if "animation_type" in widget:
                widget.set("animation_type", CabinGauge.AnimationType.MOVE)
            if "mesh_position" in widget:
                var position_vec:Vector3 = widget.get("mesh_position")
                position_vec.z = mmd_scale * range_scale
                widget.set("mesh_position", position_vec)
                if "mesh_position_offset" in widget:
                    var position_offset_vec:Vector3 = widget.get("mesh_position_offset")
                    position_offset_vec.z = descriptor.offset
                    widget.set("mesh_position_offset", position_offset_vec)
                elif not is_zero_approx(descriptor.offset):
                    diagnostics.append(_diag(
                            "info", "MMD_ANIMATION_UNSUPPORTED",
                            "Label '%s' has a non-zero MMD offset (%s) but its widget has no mesh_position_offset field - ignored" % [descriptor.label, descriptor.offset],
                            cab_number, descriptor.label, descriptor.submodel_name))
            else:
                diagnostics.append(_diag(
                        "info", "MMD_ANIMATION_UNSUPPORTED",
                        "Label '%s' uses 'mov' but its widget has no mesh_position field" % descriptor.label,
                        cab_number, descriptor.label, descriptor.submodel_name))
        _:
            diagnostics.append(_diag(
                    "info", "MMD_ANIMATION_UNSUPPORTED",
                    "Label '%s' uses unsupported animation type '%s'" % [descriptor.label, descriptor.animation_type],
                    cab_number, descriptor.label, descriptor.submodel_name))
            if not is_zero_approx(descriptor.offset):
                diagnostics.append(_diag(
                        "info", "MMD_ANIMATION_UNSUPPORTED",
                        "Label '%s' has a non-zero MMD offset (%s) which the reused cabin widgets cannot represent - ignored" % [descriptor.label, descriptor.offset],
                        cab_number, descriptor.label, descriptor.submodel_name))

    # TGauge::Update() (Gauge.cpp:364-376): value += dt * (target - value) / friction, no friction or a
    # step of half of it or more sets it outright - BaseCabinTool3D.friction_weight(), 0 for at once
    if "animation_speed" in widget:
        widget.set("animation_speed", 1.0 / descriptor.friction if descriptor.friction > 0.0 else 0.0)


## `widget` must already be inside the tree (a child of the same generated_root as `model`'s
## submodels) before this is called - the mesh-path property is a NodePath FROM the widget TO
## the target. Field name differs per widget class: CabinButton/CabinSwitch/CabinKnob all use
## `mesh_path`, but CabinGauge uses `target_mesh_path` - MmdSemanticCatalog entries carry
## `mesh_path_field` precisely so this doesn't have to special-case by class.
static func _wire_mesh_path(
        widget:Node, descriptor:MmdInstrumentDescriptor, submodel_index:Dictionary,
        mesh_path_field:String, cab_number:int, diagnostics:Array[Dictionary]) -> void:
    # A control declared with no submodel (`pantfrontoff_sw: none`, dynamic/pkp/e186_v2/
    # base.mmd.inc:187) is still a control of the cab - the original registers it all the same
    # (Train.cpp:11907, m_controlmapper), and only its presence matters; there is nothing to draw
    if descriptor.submodel_name.to_lower() == NO_SUBMODEL:
        return
    # TGauge::Load (Gauge.cpp:186-197): the submodel by its name, else by its name with "_off" - the
    # first one GetFromName meets. Submodel nodes carry Godot-validated names ("a.swmasz1" -> "a_swmasz1").
    var matches:Array = submodel_index.get(descriptor.submodel_name.validate_node_name().to_lower(), [])
    if not matches:
        matches = submodel_index.get((descriptor.submodel_name + OFF_SUFFIX).validate_node_name().to_lower(), [])
    if matches:
        widget.set(mesh_path_field, widget.get_path_to(matches[0]))
    else:
        diagnostics.append(_diag(
                "warning", "MMD_SUBMODEL_NOT_FOUND",
                "Submodel '%s' not found (label '%s')" % [descriptor.submodel_name, descriptor.label],
                cab_number, descriptor.label, descriptor.submodel_name))

    # Animation shape (mesh_rotation/max_value) comes from entry["fixed_fields"] above, not from
    # descriptor.scale/offset/friction - see mmd_semantic_catalog.gd's header comment for why.


## Builds one CabinSpotLight3D per matched "<base>_on"/"<base>_off" submodel INSTANCE, not just
## one widget total - some real cabins have multiple physical lamp housings sharing the same
## MMD-declared base name (confirmed real: sm_42_cabin.tscn's own hand-authored reference has 3
## "CzuwakOmni" lights for its one "i-security_aware:" label), unlike every other instrument label
## (which only ever has one real target mesh, so _wire_mesh_path() builds exactly one widget).
## on/off pairs are matched by index (real data doesn't guarantee they're declared in matching
## relative order - the best available heuristic without per-instance correlation data); an index
## missing one side just leaves that widget's corresponding target_path unset.
##
## "i-*:" labels declare a bare BASE name ("czuwak") that is never itself a real submodel - the
## original engine's own TButton::Init() (Button.cpp:32-33) always searches for "<name>_on" and
## "<name>_off" instead (confirmed real: SM42's own hand-authored cabin points its blinker at
## ".../czuwak_on" directly, and real-vehicle diagnostics confirmed the bare name is never found -
## EP09 uses base name "ca", so the real submodels there are "ca_on"/"ca_off").
static func _build_indicator_lights(
        descriptor:MmdInstrumentDescriptor, entry:Dictionary, vehicle_rid:RID,
        submodel_index:Dictionary, cab_model:E3DModelInstance, generated_root:Node3D, cab_number:int,
        driver_position:Vector3, sound_player:SfxPlayer3D, sound_events:Array[SfxEvent], vehicle_dir:String,
        diagnostics:Array[Dictionary]) -> void:
    var base_name:String = descriptor.submodel_name.validate_node_name().to_lower()
    var on_matches:Array = submodel_index.get(base_name + "_on", [])
    var off_matches:Array = submodel_index.get(base_name + "_off", [])
    var count:int = maxi(on_matches.size(), off_matches.size())

    if count == 0:
        diagnostics.append(_diag(
                "warning", "MMD_SUBMODEL_NOT_FOUND",
                "Submodel '%s_on'/'%s_off' not found (label '%s')" % [descriptor.submodel_name, descriptor.submodel_name, descriptor.label],
                cab_number, descriptor.label, descriptor.submodel_name))
        return

    for i in range(count):
        var widget:Node3D = entry["widget_class"].new()
        widget.name = "%s_%s_%d" % [descriptor.label, descriptor.submodel_name, i]
        for field_name:String in entry["fixed_fields"]:
            widget.set(field_name, entry["fixed_fields"][field_name])
        generated_root.add_child(widget)

        var on_node:Node3D = on_matches[i] if i < on_matches.size() else null
        var off_node:Node3D = off_matches[i] if i < off_matches.size() else null
        var submodel:Node3D = on_node if on_node else off_node
        _position_at_submodel_instance(widget, submodel)
        # unlike _build_widget(), this doesn't go through _apply_animation_shape() (indicator
        # descriptors never have a rot/mov shape - see _parse_indicator()) but DOES still need
        # _apply_sound() for soundinc:/sounddec: (confirmed real: SU45's own
        # "i-security_aware: { i-czuwak soundinc: ... sounddec: ... }" - the click sound that
        # plays on each on/off transition, CabinSpotLight3D's sound_on_event/sound_off_event),
        # sounding at the lamp's submodel
        _apply_sound(
                widget, descriptor, sound_player, generated_root.to_local(submodel.global_position),
                sound_events, vehicle_dir)
        if entry.get("aim_at_driver", false) and widget is SpotLight3D:
            _aim_spotlight_at_driver(widget as SpotLight3D, generated_root, driver_position)
        if on_node:
            widget.set("on_target_path", widget.get_path_to(on_node))
        if off_node:
            widget.set("off_target_path", widget.get_path_to(off_node))
        widget.set_vehicle_rid(vehicle_rid)

        # a lamp whose "_on" mesh is several pieces gets a light at each (MmdSemanticCatalog.IslandLights),
        # shining while REAL_INSTRUMENTS_LIGHTS_SETTING is on - the lights follow it themselves
        if entry.has("island_lights") and on_node:
            var island_lights:MmdSemanticCatalog.IslandLights = entry["island_lights"]
            var spot_widget:CabinSpotLight3D = widget as CabinSpotLight3D
            var lit_changed:Signal = (
                    spot_widget.lit_changed if spot_widget else (widget as CabinIndicator3D).lit_changed)
            var lit:bool = spot_widget.enabled if spot_widget else (widget as CabinIndicator3D).enabled
            for island:Dictionary in LegacyCabinLampIslands.submodel_islands(on_node):
                var island_light:Light3D
                if island_lights == MmdSemanticCatalog.IslandLights.GLOW:
                    var glow_light:CabinGlow = CabinGlow.new()
                    glow_light.enabled_setting = REAL_INSTRUMENTS_LIGHTS_SETTING
                    glow_light.energy_setting = INSTRUMENT_GLOW_ENERGY_SETTING
                    glow_light.energy_default = INSTRUMENT_GLOW_ENERGY_DEFAULT
                    glow_light.range_setting = INSTRUMENT_GLOW_RANGE_SETTING
                    glow_light.range_default = INSTRUMENT_GLOW_RANGE_DEFAULT
                    glow_light.light_color = island["color"]
                    glow_light.omni_attenuation = INSTRUMENT_GLOW_ATTENUATION
                    glow_light.light_size = INSTRUMENT_GLOW_SIZE
                    glow_light.light_indirect_energy = INSTRUMENT_GLOW_INDIRECT_ENERGY
                    glow_light.set_lit(lit)
                    lit_changed.connect(glow_light.set_lit)
                    island_light = glow_light
                else:
                    # the copies light the lamps, the widget only switches the meshes, blinks and sounds
                    var spot_light:CabinIslandSpotLight = CabinIslandSpotLight.new()
                    spot_light.enabled_setting = REAL_INSTRUMENTS_LIGHTS_SETTING
                    spot_light.widget = spot_widget
                    for property:StringName in WIDGET_LIGHT_PROPERTIES:
                        spot_light.set(property, spot_widget.get(property))
                    spot_light.light_energy = spot_widget.light_energy_on
                    spot_light.set_lit(lit)
                    lit_changed.connect(spot_light.set_lit)
                    island_light = spot_light
                island_light.name = "%s_%s_%d_island" % [descriptor.label, descriptor.submodel_name, i]
                generated_root.add_child(island_light)
                island_light.global_position = island["position"]
                if island_light is SpotLight3D:
                    _aim_spotlight_at_driver(island_light as SpotLight3D, generated_root, driver_position)

        # a lamp that is only its "_on" mesh also glows into the cab while it is lit, in its own
        # colour (the diffuse that tints the greyscale lamp texture, Model3d.cpp:1918); entries with
        # a light of their own (i-cablight, i-radio) or with island lights keep those instead
        # (shining while INDICATOR_GLOW_ENABLED_SETTING is on - the glow follows it itself)
        if widget is CabinIndicator3D and not entry.has("light_widget_class") and not entry.has("island_lights"):
            var glow:CabinGlow = CabinGlow.new()
            glow.name = "%s_%s_%d_glow" % [descriptor.label, descriptor.submodel_name, i]
            glow.enabled_setting = INDICATOR_GLOW_ENABLED_SETTING
            glow.energy_setting = INDICATOR_GLOW_ENERGY_SETTING
            glow.energy_default = INDICATOR_GLOW_ENERGY_DEFAULT
            glow.range_setting = INDICATOR_GLOW_RANGE_SETTING
            glow.range_default = INDICATOR_GLOW_RANGE_DEFAULT
            var glow_submodel:E3DSubModel = cab_model.model.get_node_or_null(cab_model.get_path_to(submodel))
            if glow_submodel:
                glow.light_color = glow_submodel.diffuse_color
            glow.shadow_enabled = false
            glow.set_lit((widget as CabinIndicator3D).enabled)
            generated_root.add_child(glow)
            _position_at_submodel_instance(glow, submodel)
            (widget as CabinIndicator3D).lit_changed.connect(glow.set_lit)

        if entry.has("light_widget_class"):
            var light_points:Array[Vector3] = []
            if entry.get("spread_light_along_submodel", false):
                light_points = _light_points_along_submodel(submodel)
            # the lamp's own colour: its diffuse tints the greyscale lamp texture (Model3d.cpp:1918,
            # openglrenderer.cpp:2779); the node path from the instance is the submodel's path
            var lamp_submodel:E3DSubModel = (
                    cab_model.model.get_node_or_null(cab_model.get_path_to(submodel))
                    if entry.get("light_color_from_submodel", false) else null)
            for j:int in maxi(light_points.size(), 1):
                var light:Light3D = entry["light_widget_class"].new()
                light.name = "%s_%s_%d_light%s" % [
                        descriptor.label, descriptor.submodel_name, i, "_%d" % j if j else ""]
                for field_name:String in entry["light_fixed_fields"]:
                    light.set(field_name, entry["light_fixed_fields"][field_name])
                if lamp_submodel:
                    light.light_color = lamp_submodel.diffuse_color
                generated_root.add_child(light)
                _position_at_submodel_instance(light, submodel)
                if light_points:
                    light.global_position = light_points[j]
                if entry.get("flip_upward_spotlight", false) and light is SpotLight3D:
                    _flip_spotlight_if_pointing_up(light as SpotLight3D, generated_root)
                light.set_vehicle_rid(vehicle_rid)


## Quirk for ceiling lamps: one lamp submodel may hold a whole row of bulbs (EP07 machine room
## corridor "lampy" runs along the cab), so one light at its center lights a fraction of it. Global
## points spread along the lamp mesh's longest axis, one per LAMP_LIGHT_SPACING; empty when the
## lamp has no usable bounds (the caller keeps the single centered light).
static func _light_points_along_submodel(lamp:Node3D) -> Array[Vector3]:
    var points:Array[Vector3] = []
    if not lamp is VisualInstance3D:
        return points
    var bounds:AABB = (lamp as VisualInstance3D).get_aabb()
    var start:Vector3 = lamp.to_global(bounds.position)
    var axis:Vector3 = lamp.to_global(bounds.position + bounds.size * Vector3(
            1.0 if bounds.get_longest_axis_index() == Vector3.AXIS_X else 0.0,
            1.0 if bounds.get_longest_axis_index() == Vector3.AXIS_Y else 0.0,
            1.0 if bounds.get_longest_axis_index() == Vector3.AXIS_Z else 0.0)) - start
    var center:Vector3 = lamp.to_global(bounds.get_center())
    if not _is_vector3_finite(axis) or not _is_vector3_finite(center):
        return points
    var count:int = clampi(ceili(axis.length() / LAMP_LIGHT_SPACING), 1, LAMP_LIGHT_MAX_COUNT)
    for k:int in count:
        points.append(center + axis * ((k + 0.5) / count - 0.5))
    return points


## Quirk for indicator lamps lighting the cab (alerter): the lamp submodel's own axes are arbitrary
## in legacy cab art (SU46's alerter lamp points at the windscreen) and one submodel may hold several
## bulbs, so its orientation can't be trusted. The light is aimed at the driver's eyes instead (MMD
## driverNpos:, same cab model space as generated_root) and moved a few centimetres towards them, out
## of the lamp's own shadow casting mesh.
static func _aim_spotlight_at_driver(light:SpotLight3D, generated_root:Node3D, driver_position:Vector3) -> void:
    var target:Vector3 = generated_root.to_global(driver_position)
    var direction:Vector3 = target - light.global_position
    if direction.length_squared() < 0.0001:
        return
    light.global_position += direction.normalized() * INDICATOR_LIGHT_OFFSET
    var up:Vector3 = generated_root.global_basis.y.normalized()
    if absf(direction.normalized().dot(up)) > 0.99:
        up = generated_root.global_basis.z.normalized()
    light.look_at(target, up)


## Legacy cabin models do not use a consistent local axis for ceiling-lamp meshes. Preserve the
## authored direction unless it points into the roof, in which case the useful cone is opposite.
static func _flip_spotlight_if_pointing_up(light:SpotLight3D, reference:Node3D) -> void:
    var cabin_up:Vector3 = reference.global_basis.y.normalized()
    var light_direction:Vector3 = -light.global_basis.z.normalized()
    if light_direction.dot(cabin_up) > 0.0:
        light.rotate_object_local(Vector3.RIGHT, PI)


## Positions `widget` at `submodel`'s visual AABB center rather than its raw transform
## origin/pivot (frequently off to one side, e.g. its mounting point). A directional "push forward
## off the surface" correction was tried and reverted - confirmed real that a submodel's local Z
## orientation is NOT consistent across vehicles' art (looked right on SU45, wrong on EP09/SM42),
## so there is no single fixed direction/amount that works generically; plain AABB center is the
## safer default even though it can leave the light slightly embedded in solid geometry on some
## vehicles. Some vehicles (confirmed: SM42) combine multiple physically scattered lamp bulbs into
## ONE submodel object - its AABB center is then a meaningless average point between them, a real
## data limitation this can't correct for from geometry alone.
static func _position_at_submodel_instance(widget:Node3D, submodel:Node3D) -> void:
    var target_transform:Transform3D = submodel.global_transform
    if submodel is VisualInstance3D:
        var local_center:Vector3 = (submodel as VisualInstance3D).get_aabb().get_center()
        if _is_vector3_finite(local_center):
            target_transform.origin = submodel.to_global(local_center)
    # Some real submodels (confirmed: indicator lamp meshes on at least one 303E cabin variant)
    # end up with a non-finite global_transform (broken parent pivot chain in the source art, or
    # the AABB-center fallback above) - RenderingServer rejects a NaN/Inf transform loudly
    # (instance_set_transform "!v.is_finite()") for every such widget on every rebuild. Leaving
    # the widget at its default (identity) transform is harmless here: none of these widgets
    # render anything of their own, they only hold references to other nodes
    # (CabinIndicator3D's on_target/off_target, Light3D's own separately-positioned instance).
    if _is_vector3_finite(target_transform.origin) \
            and _is_vector3_finite(target_transform.basis.x) \
            and _is_vector3_finite(target_transform.basis.y) \
            and _is_vector3_finite(target_transform.basis.z):
        widget.global_transform = target_transform


static func _is_vector3_finite(v:Vector3) -> bool:
    return is_finite(v.x) and is_finite(v.y) and is_finite(v.z)


static func _tokenize_file(abs_path:String, context:MmdImportContext, parameters:Dictionary = {}) -> Array[String]:
    context.include_depth += 1
    if context.include_depth > 32:
        context.add_diagnostic("error", "MMD_INCLUDE_CYCLE", "Include depth exceeded (circular include?): " + abs_path, abs_path)
        context.include_depth -= 1
        return []

    var game_dir:String = UserSettings.get_maszyna_game_dir().trim_suffix("/")
    var base_dir:String = abs_path.get_base_dir()
    var relative_path:String = abs_path.get_file()
    if abs_path.begins_with(game_dir + "/"):
        base_dir = game_dir
        relative_path = abs_path.trim_prefix(game_dir + "/")
    abs_path = base_dir.path_join(MaszynaDataPath.resolve(base_dir, relative_path))

    var file:FileAccess = FileAccess.open(abs_path, FileAccess.READ)
    if not file:
        context.add_diagnostic("error", "MMD_INCLUDE_NOT_FOUND", "Cannot open MMD file: " + abs_path, abs_path)
        context.include_depth -= 1
        return []
    var buffer:PackedByteArray = _strip_bom(file.get_buffer(file.get_length()))
    file.close()

    var p := MaszynaParser.new()
    p.initialize(buffer)
    if parameters:
        p.set_parameters(parameters)

    var dir:String = abs_path.get_base_dir()
    var tokens:Array[String] = []
    while not p.eof_reached():
        var token:String = p.next_token()
        if not token:
            continue
        # ":" is not a MaszynaParser stop char, so a label glued directly to its first value with
        # no space (real data: "radiostop_sw:radiostop") comes back as one token - split it into
        # the label (colon kept) and the remainder so the rest of this parser, which always
        # expects "label:" as its own token, still works.
        var colon_index:int = token.find(":")
        if colon_index != -1 and colon_index < token.length() - 1:
            tokens.append(token.substr(0, colon_index + 1))
            token = token.substr(colon_index + 1)
        if token.to_lower() == "include":
            tokens.append_array(_handle_include(p, dir, context, abs_path))
        else:
            tokens.append(token)

    context.include_depth -= 1
    return tokens


## `include filename p1 p2 ... end` or the random-file-set form `include [a.inc b.inc] end`.
static func _handle_include(p:MaszynaParser, dir:String, context:MmdImportContext, current_file:String) -> Array[String]:
    var first:String = p.next_token()
    var is_random:bool = first == _RANDOM_INCLUDE_OPEN
    var candidates:Array[String] = []
    var include_filename:String = first

    if is_random:
        var t:String = p.next_token()
        while t and t != _RANDOM_INCLUDE_CLOSE:
            candidates.append(t)
            t = p.next_token()

    var params:Array[String] = []
    var t2:String = p.next_token()
    while t2 and t2.to_lower() != _INCLUDE_END_KEYWORD:
        params.append(t2)
        t2 = p.next_token()

    if is_random:
        if not candidates:
            context.add_diagnostic("error", "MMD_INVALID_CAB_DEFINITION", "Empty random include list", current_file)
            return []
        # Keyed by the include site's own content (not call order), so a later re-parse of the
        # same file with the same random_choices dict reproduces the same choice.
        var choice_key:String = "%s#%s" % [current_file, "|".join(candidates)]
        if not context.random_choices.has(choice_key):
            context.random_choices[choice_key] = candidates[randi() % candidates.size()]
        include_filename = context.random_choices[choice_key]

    if not include_filename:
        context.add_diagnostic("error", "MMD_INVALID_CAB_DEFINITION", "Empty include filename", current_file)
        return []

    return _tokenize_file(dir.path_join(include_filename), context, _include_parameters(params))


## The (pN) of an included file - a missing one is "none" (parser.cpp:280)
static func _include_parameters(values:Array[String]) -> Dictionary:
    var parameters:Dictionary = {}
    for i:int in range(1, maxi(values.size(), MAX_INCLUDE_PARAMETERS) + 1):
        parameters["p%d" % i] = values[i - 1] if i <= values.size() else "none"
    return parameters


static func _strip_bom(buffer:PackedByteArray) -> PackedByteArray:
    if buffer.size() >= 3 and buffer[0] == 0xEF and buffer[1] == 0xBB and buffer[2] == 0xBF:
        return buffer.slice(3)
    return buffer
