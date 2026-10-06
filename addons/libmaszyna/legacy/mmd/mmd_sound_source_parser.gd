extends RefCounted
class_name MmdSoundSourceParser

## Parses the MMD's vehicle-wide `sounds:`...`endsounds` section into one MmdSoundSourceDefinition
## per top-level label, mirroring the original engine's audio/sound.cpp `sound_source::
## deserialize_mapping()` block-form dispatch. Reuses MmdCabinInstancer's whole-file tokenizer
## (include/(pN) expansion, BOM handling, label:value splitting) and MmdImportContext - the
## `sounds:` section shares that same include/token grammar with `internaldata:`, only the label
## shapes below it differ.
##
## Bare (non-block) label lines have no self-describing arity ("oilpump: file.wav" is 1 token,
## "tractionmotor: file.wav 10.0 0.01 0.5 0.16 0.1" is 6 - confirmed against real dynamic/pkp
## data), and only Tier 1/2 labels (see MmdSoundCatalog) are actually interpreted here - so rather
## than a per-label arity table for all ~46 possible labels, a bare label's value tokens are
## consumed generically up to the next "label:"-shaped token. Every real bare value (filename,
## float, [NNNN] prefix) is confirmed to never itself contain ':', so this can't misfire on a
## following genuine label.

## label -> bare value (if any) is this label's sound_main - matches audio/sound.cpp's own
## sound_type::single legacy-form dispatch for these exact call sites (DynObj.cpp:6015/6036/6041/
## 6705/6708 - engine/oil_pump/fuel_pump/engine_ignition/engine_shutdown all deserialize() with no
## extra Legacyparameters, i.e. just one filename token in bare form). "battery" is confirmed
## always block-form in real data instead.
const _BARE_SINGLE_LABELS:Dictionary = {
    "oilpump": true,
    "fuelpump": true,
    "ignition": true,
    "shutdown": true,
    # DynObj.cpp:6290, 6300 - sound_type::single
    "pantographup": true,
    "pantographdown": true,
    # buzzer:/buzzershp: (Train.cpp:10362-10363's own "buzzer:"/"buzzershp:" -> dsbBuzzer/
    # dsbBuzzerShp mapping table, sound_type::single) - bare form is usually a bracketed
    # random-choice list of variants (confirmed real: dynamic/pkp/sm42_v1/6d.mmd's
    # "buzzer: [ buczek.wav 24150_buczek_cashp.wav ... ]") but _read_random_set() already
    # handles that transparently, same as every other bare label here.
    "buzzer": true,
    "buzzershp": true,
    # radiostop: (Train.cpp:10340, sound_type::single)
    "radiostop": true,
    "brake": true,
    "brakesound": true,
    "unbrake": true,
    "emergencybrake": true,
    "slipperysound": true,
    "airsound": true,
    "airsound2": true,
    "airsound3": true,
    "airsound4": true,
    "airsound5": true,
    "localbrakesound": true,
    "localbrakesound2": true,
    "brakecylinderinc": true,
    "brakecylinderdec": true,
    "epbrakeinc": true,
    "epbrakedec": true,
    "brakeacc": true,
    "springbrake": true,
    "springbrakeoff": true,
    # running sounds (DynObj.cpp:5710, 5801, 5916, 6123; Train.cpp:8571)
    "tractionmotor": true,
    "ventilator": true,
    "curve": true,
    "outernoise": true,
    "runningnoise": true,
}

## label -> bare form is 3 values (begin, main, end) + an optional trailing range - confirmed real
## (dynamic/pkp/401da_v1/401da_dumb.mmd: "horn1: horn-sn61-start.wav horn-sn61.wav
## horn-sn61-stop.wav 220"), matching audio/sound.cpp's own sound_type::multipart +
## sound_parameters::range legacy-form dispatch (DynObj.cpp's sHorn1/sHorn2/sHorn3.deserialize()
## call sites) exactly - begin/main/end in that fixed order (audio/sound.h's sound_id enum).
const _BARE_MULTIPART_LABELS:Dictionary = {
    "horn1": true,
    "horn2": true,
    "horn3": true,
    "releaser": true,
    # DynObj.cpp:6321, 6353 - read as sound_type::multipart with sound_parameters::range
    "compressor": true,
    "converter": true,
    "small-compressor": true,
    # DynObj.cpp:6257 - sound_type::multipart with sound_parameters::range
    "turbo": true,
    # DynObj.cpp:6359
    "departuresignal": true,
}

const _BARE_PARAMETERS:Dictionary = {
    "horn1": [&"range"],
    "horn2": [&"range"],
    "horn3": [&"range"],
    "brake": [&"range", &"amplitude_factor", &"amplitude_offset"],
    "brakesound": [&"amplitude_factor", &"amplitude_offset", &"frequency_factor", &"frequency_offset"],
    "unbrake": [&"range"],
    "departuresignal": [&"range"],
    "slipperysound": [&"amplitude_factor", &"amplitude_offset"],
    "airsound": [&"amplitude_factor", &"amplitude_offset"],
    "airsound2": [&"amplitude_factor", &"amplitude_offset"],
    "airsound3": [&"amplitude_factor", &"amplitude_offset"],
    "airsound4": [&"amplitude_factor", &"amplitude_offset"],
    "airsound5": [&"amplitude_factor", &"amplitude_offset"],
    "localbrakesound": [&"amplitude_factor", &"amplitude_offset"],
    "localbrakesound2": [&"amplitude_factor", &"amplitude_offset"],
    "releaser": [&"range"],
    "compressor": [&"range"],
    "turbo": [&"range"],
    "converter": [&"range"],
    "small-compressor": [&"range"],
    "tractionmotor": [&"range", &"amplitude_factor", &"amplitude_offset", &"frequency_factor", &"frequency_offset"],
    "ventilator": [&"range", &"amplitude_factor", &"amplitude_offset", &"frequency_factor", &"frequency_offset"],
    "curve": [&"range"],
    "outernoise": [&"amplitude_factor", &"amplitude_offset", &"frequency_factor", &"frequency_offset"],
    "runningnoise": [&"amplitude_factor", &"amplitude_offset", &"frequency_factor", &"frequency_offset"],
}


static func parse(abs_mmd_path:String, context:MmdImportContext) -> Array[MmdSoundSourceDefinition]:
    var tokens:Array[String] = MmdCabinInstancer._tokenize_file(abs_mmd_path, context, context.parameters)
    var start_index:int = MmdCabinInstancer._find_label_index(tokens, "sounds:")
    if start_index == -1:
        return []
    start_index += 1
    var end_index:int = MmdCabinInstancer._find_label_index(tokens, "endsounds", start_index)
    if end_index == -1:
        end_index = tokens.size()
    return _parse_labels_in_range(tokens, start_index, end_index, context, abs_mmd_path, {})


## ignition:/shutdown:/buzzer:/buzzershp: (see MmdSoundBankInstancer) live in the MMD's
## internaldata: section, BEFORE cab1definition:/cab2definition: - confirmed real, NOT inside
## sounds:/endsounds (dynamic/pkp/sm42_v1/6d.mmd: "ignition: { range: 150 soundmain:
## 697_sm42-start-v2.wav }" at line 92 and "buzzer: [ buczek.wav ... ]" at line 85, both well past
## endsounds at line 81). The brake-related labels (brakesound:/slipperysound:/airsound(2-5):/
## localbrakesound(2):) are the original engine's TTrain-owned cab-local pneumatic sounds
## (Train.cpp:8548-8574) and are likewise real here (dynamic/pkp/su45_v2/301d.mmd:119-127) - kept
## for MmdSoundBankInstancer/MmdSoundCatalog same as the other four. internaldata: has dozens of
## other sound-shaped fields (rainsound/tachoclock/couplerattach/...) out of scope here - only
## these labels are kept, everything else is parsed just enough to stay token-aligned (reusing the
## exact same generic block/bare consumption as parse() above) and then discarded, with no
## diagnostic (this region isn't otherwise covered by MmdSoundCatalog).
static func parse_internal_data(abs_mmd_path:String, context:MmdImportContext) -> Array[MmdSoundSourceDefinition]:
    var tokens:Array[String] = MmdCabinInstancer._tokenize_file(abs_mmd_path, context, context.parameters)
    var start_index:int = MmdCabinInstancer._find_label_index(tokens, "internaldata:")
    if start_index == -1:
        return []
    start_index += 1
    var end_index:int = MmdCabinInstancer._find_label_index(tokens, "cab1definition:", start_index)
    var cab2_index:int = MmdCabinInstancer._find_label_index(tokens, "cab2definition:", start_index)
    if not cab2_index == -1 and (end_index == -1 or cab2_index < end_index):
        end_index = cab2_index
    if end_index == -1:
        end_index = tokens.size()
    return _parse_labels_in_range(tokens, start_index, end_index, context, abs_mmd_path, {
        "ignition": true, "shutdown": true, "buzzer": true, "buzzershp": true, "tachoclock": true,
        "brakesound": true, "slipperysound": true, "localbrakesound": true, "localbrakesound2": true,
        "airsound": true, "airsound2": true, "airsound3": true, "airsound4": true, "airsound5": true,
        "runningnoise": true, "radiostop": true,
    })


## Offsets along the vehicle of `tractionmotors:`, `bogies:` and `doors:` from the `locations:`
## section (DynObj.cpp:6289-6326, 6594-6639), as MMD values - negative means ahead of the centre.
## A door is a pair, its offset and its sides; the offset is kept - one door speaker each.
static func parse_locations(abs_mmd_path:String, context:MmdImportContext) -> Dictionary:
    var locations:Dictionary = {
        "tractionmotors": PackedFloat32Array(), "bogies": PackedFloat32Array(), "doors": PackedFloat32Array()}
    var tokens:Array[String] = MmdCabinInstancer._tokenize_file(abs_mmd_path, context, context.parameters)
    var start_index:int = MmdCabinInstancer._find_label_index(tokens, "locations:")
    if start_index == -1:
        return locations
    var end_index:int = MmdCabinInstancer._find_label_index(tokens, "endlocations", start_index + 1)
    if end_index == -1:
        end_index = tokens.size()
    var i:int = start_index + 1
    while i < end_index:
        var key:String = tokens[i].to_lower().trim_suffix(":")
        i += 1
        if not locations.has(key):
            continue
        var offsets:PackedFloat32Array = locations[key]
        # a door's offset is followed by its sides (right, left, both)
        var step:int = 2 if key == "doors" else 1
        while i < end_index and not tokens[i].to_lower() == "end":
            offsets.append(float(tokens[i]))
            i += step
        locations[key] = offsets
    return locations


static func parse_vehicle_soundproofing(abs_mmd_path:String, context:MmdImportContext) -> Array[PackedFloat32Array]:
    var tokens:Array[String] = MmdCabinInstancer._tokenize_file(abs_mmd_path, context, context.parameters)
    var start_index:int = MmdCabinInstancer._find_label_index(tokens, "internaldata:")
    var end_index:int = MmdCabinInstancer._find_label_index(tokens, "cab1definition:", start_index + 1)
    if start_index < 0 or end_index < 0:
        return []
    var depth:int = 0
    for i:int in range(start_index + 1, end_index):
        if tokens[i] == "{":
            depth += 1
        elif tokens[i] == "}":
            depth -= 1
        elif depth == 0 and tokens[i].to_lower() == "soundproofing:":
            var result:Dictionary = _read_float_list(tokens, i + 1, 30)
            var values:PackedFloat32Array = result["value"]
            if values.size() < 30:
                return []
            var profile:Array[PackedFloat32Array] = []
            for row:int in range(5):
                profile.append(values.slice(row * 6, row * 6 + 6))
            return profile
    return []


static func _parse_labels_in_range(
        tokens:Array[String], start_index:int, end_index:int, context:MmdImportContext,
        abs_mmd_path:String, only_labels:Dictionary) -> Array[MmdSoundSourceDefinition]:
    var definitions:Array[MmdSoundSourceDefinition] = []
    var i := start_index
    while i < end_index:
        var label:String = tokens[i].to_lower()
        i += 1
        if not label.ends_with(":"):
            continue # stray value token from a desync earlier - stay aligned, same as MmdCabinInstancer
        if label == "wheel_clatter:":
            var axles:Array[MmdSoundSourceDefinition] = []
            i += _parse_wheel_clatter(tokens, i, axles, context, abs_mmd_path)
            if not only_labels or only_labels.has("wheel_clatter"):
                definitions.append_array(axles)
            continue
        var definition := MmdSoundSourceDefinition.new()
        definition.label = label.trim_suffix(":")
        definition.source_file = abs_mmd_path
        i += _parse_one(tokens, i, definition, context, abs_mmd_path)
        if not only_labels or only_labels.has(definition.label):
            definitions.append(definition)
    return definitions


## `wheel_clatter: <range> <axle offset> <sound> ... end` (DynObj.cpp:5643-5666): one definition
## per axle, its sound a single sample or a block, placed at the axle - offset.z is the MMD offset
## negated, the original vehicle frame (+Z forward, negative MMD offsets are ahead of the centre).
static func _parse_wheel_clatter(
        tokens:Array[String], i:int, axles:Array[MmdSoundSourceDefinition],
        context:MmdImportContext, source_file:String) -> int:
    var start:int = i
    var clatter_range:float = float(tokens[i]) if i < tokens.size() else 50.0
    i += 1
    while i < tokens.size() and not tokens[i].to_lower() == "end":
        var definition := MmdSoundSourceDefinition.new()
        definition.label = "wheel_clatter"
        definition.source_file = source_file
        var axle_offset:float = -float(tokens[i])
        definition.range = clatter_range
        definition.range_defined = true
        i += 1
        if i < tokens.size() and tokens[i] == "{":
            i += 1
            while i < tokens.size() and not tokens[i] == "}":
                i += _parse_block_field(tokens, i, definition, context, source_file)
            i += 1 # consume "}"
        else:
            var result:Dictionary = _read_random_set(tokens, i, context, source_file, "wheel_clatter_%d" % axles.size())
            definition.sound_main = result["value"]
            i += int(result["consumed"])
        definition.offset = Vector3(0.0, 0.0, axle_offset)
        axles.append(definition)
    return mini(i + 1, tokens.size()) - start # consume "end"


static func _parse_one(
        tokens:Array[String], i:int, definition:MmdSoundSourceDefinition,
        context:MmdImportContext, source_file:String) -> int:
    var start:int = i
    if definition.label == "unbrake":
        definition.amplitude_factor = 20.0
    if i < tokens.size() and tokens[i] == "{":
        i += 1
        while i < tokens.size() and not tokens[i] == "}":
            i += _parse_block_field(tokens, i, definition, context, source_file)
        if i < tokens.size():
            i += 1 # consume "}"
        return i - start

    # bare form
    if _BARE_SINGLE_LABELS.has(definition.label):
        var result:Dictionary = _read_random_set(tokens, i, context, source_file, definition.label)
        definition.sound_main = result["value"]
        i += int(result["consumed"])
    elif _BARE_MULTIPART_LABELS.has(definition.label) and i < tokens.size() and "," in tokens[i]:
        # The original reads a legacy sound's files with "," among the delimiters
        # (audio/sound.cpp:105-111), so "a.wav,b.wav,c.wav" is begin, main and end
        # (e.g. e186_v2 small-compressor:).
        var parts:PackedStringArray = tokens[i].split(",", false)
        definition.sound_begin = _normalize_sound_filename(parts[0])
        if parts.size() > 1:
            definition.sound_main = _normalize_sound_filename(parts[1])
        if parts.size() > 2:
            definition.sound_end = _normalize_sound_filename(parts[2])
        i += 1
    elif _BARE_MULTIPART_LABELS.has(definition.label):
        for part:String in ["begin", "main", "end"]:
            var result:Dictionary = _read_random_set(tokens, i, context, source_file, "%s_%s" % [definition.label, part])
            i += int(result["consumed"])
            match part:
                "begin": definition.sound_begin = result["value"]
                "main": definition.sound_main = result["value"]
                "end": definition.sound_end = result["value"]

    var parameters:Array = _BARE_PARAMETERS.get(definition.label, [])
    for parameter:StringName in parameters:
        if i >= tokens.size() or tokens[i].to_lower().ends_with(":"):
            break
        definition.set(parameter, float(tokens[i]))
        if parameter == &"range":
            definition.range_defined = true
        i += 1

    # any remaining bare values (range, amplitude/frequency pairs, ...) aren't consumed in v1 -
    # skip up to the next "label:"-shaped token so the stream stays aligned.
    while i < tokens.size() and not tokens[i].to_lower().ends_with(":"):
        i += 1
    return i - start


## `tokens[i]` is one block-internal key (already known to not be "}"). Returns tokens consumed,
## including the key itself.
static func _parse_block_field(
        tokens:Array[String], i:int, definition:MmdSoundSourceDefinition,
        context:MmdImportContext, source_file:String) -> int:
    var key:String = tokens[i].to_lower()
    var start:int = i
    i += 1

    match key:
        "soundmain:":
            var result:Dictionary = _read_random_set(tokens, i, context, source_file, key)
            definition.sound_main = result["value"]
            i += int(result["consumed"])
        "soundbegin:":
            var result:Dictionary = _read_random_set(tokens, i, context, source_file, key)
            definition.sound_begin = result["value"]
            i += int(result["consumed"])
        "soundend:":
            var result:Dictionary = _read_random_set(tokens, i, context, source_file, key)
            definition.sound_end = result["value"]
            i += int(result["consumed"])
        "soundset:":
            var result:Dictionary = _read_random_set(tokens, i, context, source_file, key)
            var parts:PackedStringArray = String(result["value"]).split("|")
            if parts.size() > 0:
                definition.sound_begin = _normalize_sound_filename(parts[0])
            if parts.size() > 1:
                definition.sound_main = _normalize_sound_filename(parts[1])
            if parts.size() > 2:
                definition.sound_end = _normalize_sound_filename(parts[2])
            i += int(result["consumed"])
        "crossfade:":
            if i < tokens.size():
                definition.crossfade_percent = clampi(int(tokens[i]), 0, 100)
                i += 1
        "soundproofing:":
            var value:Dictionary = _read_float_list(tokens, i, 6)
            definition.soundproofing = value["value"]
            i += int(value["consumed"])
        "offset:":
            var value:Dictionary = _read_float_list(tokens, i, 3)
            var numbers:PackedFloat32Array = value["value"]
            if numbers.size() == 3:
                definition.offset = Vector3(numbers[0], numbers[1], numbers[2])
            i += int(value["consumed"])
        "amplitudefactor:":
            definition.amplitude_factor = float(tokens[i])
            i += 1
        "amplitudeoffset:":
            definition.amplitude_offset = float(tokens[i])
            i += 1
        "frequencyfactor:":
            definition.frequency_factor = float(tokens[i])
            i += 1
        "frequencyoffset:":
            definition.frequency_offset = float(tokens[i])
            i += 1
        "range:":
            definition.range = float(tokens[i])
            definition.range_defined = true
            i += 1
        "placement:":
            definition.placement = StringName(tokens[i].to_lower())
            definition.placement_defined = true
            i += 1
        "pitchvariation:":
            definition.pitch_variation = clampf(float(tokens[i]), 0.0, 1.0)
            i += 1
        "startoffset:":
            definition.start_offset = clampf(float(tokens[i]), 0.0, 1.0)
            i += 1
        _:
            if key.begins_with("sound") and not _threshold_from_key(key) == null:
                var result:Dictionary = _read_random_set(tokens, i, context, source_file, key)
                definition.chunks.append({"threshold": _threshold_from_key(key), "filename": result["value"], "pitch": 0.0})
                i += int(result["consumed"])
            elif key.begins_with("pitch") and not _threshold_from_key(key) == null:
                var threshold:int = _threshold_from_key(key)
                if i < tokens.size():
                    var pitch:float = float(tokens[i])
                    i += 1
                    for chunk:Dictionary in definition.chunks:
                        if int(chunk["threshold"]) == threshold:
                            chunk["pitch"] = pitch
                            break
            else:
                # amplitudefactor:/amplitudeoffset:/frequencyfactor:/frequencyoffset:/range:/
                # placement:/pitchvariation:/startoffset: and anything else unrecognized - not
                # consumed by MmdSoundEventBuilder in v1, but still skipped generically (single
                # token, or a bracketed/nested value) so the token stream stays aligned.
                i += _skip_value(tokens, i)

    return i - start


static func _read_float_list(tokens:Array[String], i:int, count:int) -> Dictionary:
    var values := PackedFloat32Array()
    var start:int = i
    var bracketed:bool = i < tokens.size() and tokens[i] == "["
    if bracketed:
        i += 1
    while i < tokens.size() and values.size() < count and (not bracketed or not tokens[i] == "]"):
        values.append(float(tokens[i]))
        i += 1
    if bracketed and i < tokens.size() and tokens[i] == "]":
        i += 1
    return {"value": values, "consumed": i - start}


## "sound1:" -> 1, "sound-3:" -> -3, "sound400:" -> 400, "pitch600:" -> 600. Returns null for
## "soundmain:"/"soundbegin:"/"soundend:"/"soundset:"/"pitchvariation:" (no valid-int remainder).
static func _threshold_from_key(key:String) -> Variant:
    var middle:String = key.substr(5, key.length() - 6) # strip "sound"/"pitch" prefix (5 chars) and trailing ":"
    if not middle or not middle.is_valid_int():
        return null
    return int(middle)


## Skips one value at tokens[i]: a bracketed `[ ... ]` list (soundproofing:/offset:), or a single
## token otherwise. Returns tokens consumed (NOT including the key, which the caller already did).
static func _skip_value(tokens:Array[String], i:int) -> int:
    if i >= tokens.size():
        return 0
    if tokens[i] == "[":
        var j:int = i + 1
        while j < tokens.size() and not tokens[j] == "]":
            j += 1
        if j < tokens.size():
            j += 1 # consume "]"
        return j - i
    return 1


## Mirrors the original engine's deserialize_random_set(): either a single bare token, or a
## bracketed `[ a, b, c ]` random-choice list resolved to one entry (frozen via
## context.random_choices, keyed by this field's own content - same mechanism already used for
## MMD `include [...] end` and cabin soundinc:/sounddec: lists). Candidates are comma-separated in
## real data (MaszynaParser's default stop chars don't include ',', so a trailing comma ends up
## glued onto the preceding candidate token and must be stripped).
static func _read_random_set(
        tokens:Array[String], i:int, context:MmdImportContext, source_file:String, field_key:String) -> Dictionary:
    if i >= tokens.size():
        return {"value": "", "consumed": 0}

    if not tokens[i] == "[":
        return {"value": _normalize_sound_filename(tokens[i]), "consumed": 1}

    var candidates:Array[String] = []
    var j:int = i + 1
    while j < tokens.size() and not tokens[j] == "]":
        var candidate:String = tokens[j].trim_suffix(",")
        if candidate:
            candidates.append(candidate)
        j += 1
    if j < tokens.size():
        j += 1 # consume "]"
    if not candidates:
        return {"value": "", "consumed": j - i}

    var choice_key:String = "%s#%s#%s" % [source_file, field_key, "|".join(candidates)]
    if not context.random_choices.has(choice_key):
        context.random_choices[choice_key] = candidates[randi() % candidates.size()]
    return {"value": _normalize_sound_filename(context.random_choices[choice_key]), "consumed": j - i}


## Strips a trailing ".wav"/".ogg"/".flac" - everything else ("[NNNN]" numeric prefixes included) is kept
## verbatim, same rule as MmdCabinInstancer._normalize_sound_filename(). Does NOT split on "|" -
## soundset:'s caller does that itself after this returns the whole chosen candidate.
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
