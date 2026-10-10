extends RefCounted
class_name MmdSoundSourceDefinition

## pitch_variation when the MMD gives no pitchvariation:
const NO_PITCH_VARIATION:float = -1.0

## One parsed label from the MMD's own vehicle-wide `sounds:`...`endsounds` section, in the
## neutral shape the original engine's audio/sound.cpp `sound_source` class settles every one of
## its ~46 label syntaxes into: an optional single/begin/end sound (all three normalized,
## extension-less filenames, "" when absent) and/or a set of numbered `soundN:`/`pitchN:` chunks.
## `soundset:` (a single random-set value containing "a|b|c", confirmed real:
## dynamic/pkp/303e_v1/303e-ep-ic.mmd's compressor:) is unpacked into sound_begin/sound_main/
## sound_end at parse time, same as if the source had used soundbegin:/soundmain:/soundend:
## directly - MmdSoundEventBuilder only ever looks at the fields below, never at which source
## syntax produced them.

var label:String = ""
var sound_main:String = ""
var sound_begin:String = ""
var sound_end:String = ""
## {threshold:int, filename:String, pitch:float}, in file order (MmdSoundEventBuilder sorts by
## threshold - matches the original engine's own sound_source::deserialize() sort step).
var chunks:Array[Dictionary] = []
## Percentage (0-100) of the gap between adjacent chunk thresholds that the crossfade porting in
## MmdSoundEventBuilder blends over - mirrors m_crossfaderange, "crossfade:" in MMD.
var crossfade_percent:int = 0
## Where the last chunk ends at the latest, unless its threshold is higher - sound_source::
## deserialize()'s Chunkrange (sound.cpp:85, default 100 in sound.h:69; outernoise: takes the
## vehicle's Vmax, DynObj.cpp:6389)
const DEFAULT_CHUNK_RANGE:int = 100
var chunk_range:int = DEFAULT_CHUNK_RANGE
var amplitude_factor:float = 1.0
var amplitude_offset:float = 0.0
var frequency_factor:float = 1.0
var frequency_offset:float = 0.0
var range:float = 50.0
var range_defined:bool = false
var placement:StringName = &"general"
var placement_defined:bool = false
var offset:Vector3 = Vector3.ZERO
var soundproofing:PackedFloat32Array = PackedFloat32Array()
## pitchvariation: - the share of the pitch an emitter may be off by, 0-1; NO_PITCH_VARIATION when
## the MMD gives none, and the original's default range applies (sound.cpp:207-216, 374-377)
var pitch_variation:float = NO_PITCH_VARIATION
## startoffset: - where the sound starts in its sample, 0-1 (sound.cpp:218-222)
var start_offset:float = 0.0
var source_file:String = ""


## This definition under another label - the sound a vehicle plays for one it does not define
## (Train.cpp:9082-9089)
func copy_as(copy_label:String) -> MmdSoundSourceDefinition:
    var copy := MmdSoundSourceDefinition.new()
    for property:Dictionary in get_property_list():
        if property["usage"] & PROPERTY_USAGE_SCRIPT_VARIABLE:
            copy.set(property["name"], get(property["name"]))
    copy.label = copy_label
    return copy
