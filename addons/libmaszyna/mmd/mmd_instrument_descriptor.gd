extends RefCounted
class_name MmdInstrumentDescriptor

## One parsed MMD instrument/manipulator line, in the raw shape shared by every label:
## `label: submodel animation scale offset friction` (or the equivalent `{ ... }` block form).
## Duplicate labels (e.g. two `tachometer:` entries driving two needles) are kept as separate
## descriptors in MmdCabinDefinition.instruments, in file order - this class never deduplicates.

var label:String = ""
var submodel_name:String = ""
## The MMD's animation: rot, mov, wip, dgt, rotvar or movvar (TGauge::Load, Gauge.cpp:212-221)
var animation_type:String = ""
var scale:float = 0.0
var offset:float = 0.0
var friction:float = 0.0
## rotvar/movvar: the value at which the scale has become end_scale (Gauge.cpp:116-122, 448-456)
var end_value:float = 0.0
var end_scale:float = 0.0
## brakes:/eimscreen: - the two numbers before the shape: which car of the train and which of its
## values (Train.cpp:12147-12166)
var leading_numbers:PackedInt32Array = []
## "return"/"impulse"/"push"/"toggle"/"pushtoggle"/"delayed", or "" when the label has no
## explicit `type:` field (plain 5-token form).
var button_type:String = ""
var source_file:String = ""
var line:int = 0
## Which way a drag moves the control up, per mouse axis - not the MMD's: a game's patch's
## (MaszynaCabControlPatch.mouse_drag_signs); ZERO follows its grip on screen
var mouse_drag_signs:Vector2 = Vector2.ZERO
## The turn at each whole value [deg] - not the MMD's: a game's patch's
## (MaszynaCabControlPatch.position_angles); none, the scale's
var position_angles:PackedFloat32Array = []
## A game's patch's control: it turns about its own axes as its parent sees them (CabinKnob.turn_unscaled)
var patched:bool = false

## Extension-less, normalized filenames from the instrument's `{ ... }` block sound fields
## ("" when absent). A bracketed random-choice list (`soundinc: [ a.wav b.wav ]`) resolves to one
## entry, chosen once and persisted via MmdImportContext.random_choices - same treatment as MMD's
## random `include` lists. A nested sub-block (`soundinc: { soundmain: ... }`) keeps only its
## soundmain: filename.
var sound_increase:String = ""
var sound_decrease:String = ""
## Position (int, can be negative) -> filename, from numbered "soundN:"/"sound-N:" fields.
var sound_positions:Dictionary = {}
