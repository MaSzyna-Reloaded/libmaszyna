extends RefCounted
class_name MmdCabinDefinition

## Neutral, parsed representation of one MMD file's cabin data, for one selected cab number.
## Produced by MmdCabinInstancer.parse() and consumed by MmdCabinInstancer.build_into() - this
## class knows nothing about Cabin3D, E3DModelInstance, or VehicleController.

## MMD cab section number: 0 (machine room), 1 or 2.
var cab_number:int = 1

## Raw MMD camera bounds - MaszynaPlayer adds the +0.5/+1.8 Y offset itself,
## do not add it again here.
var bounds_min:Vector3 = Vector3.ZERO
var bounds_max:Vector3 = Vector3.ZERO
var driver_pos:Vector3 = Vector3.ZERO
## Where the cab camera starts (drivermode.cpp:1224) - driverNpos: unless driverNsitpos: follows it
var driver_sitpos:Vector3 = Vector3.ZERO
## driverNangle: - yaw, then pitch, in degrees, of the cab camera's start (Train.cpp:10529-10535)
var driver_angle:Vector2 = Vector2.ZERO
## The cab radio's Radio-Stop alarm (internaldata:'s radiostop:, Train.cpp:10340); null without one
var radio_stop_sound:MmdSoundSourceDefinition = null

## Original MaSzyna camera spring parameters from the MMD preamble.
## The cab's interior light - internaldata:'s cablight:, its middle (base) colour (DynObj.cpp:
## 6924-6932); tungsten without it (InteriorLight, DynObj.h:258)
const DEFAULT_INTERIOR_LIGHT:Color = Color(0.9, 0.9 * 216.0 / 255.0, 0.9 * 176.0 / 255.0)
var interior_light:Color = DEFAULT_INTERIOR_LIGHT
var shake_spring_stiffness:float = 125.0
var shake_spring_damping:float = 0.002
var shake_jolt_scale:Vector3 = Vector3(0.2, 0.2, 0.1)
var shake_jolt_limit:float = 2.0
var shake_angle_scale:Vector2 = Vector2(0.05, 0.1)
var engine_shake_scale:float = 2.0
var engine_shake_fade_in_rpm:float = 90.0
var engine_shake_fade_in_factor:float = 0.3
var engine_shake_fade_out_rpm:float = 600.0
var engine_shake_fade_out_factor:float = 0.5

## Cab model, relative to the MMD's own directory, with ".t3d" already swapped for ".e3d" and
## backslashes normalized - still needs case-insensitive filesystem resolution by the builder.
var model_relpath:String = ""

## Ordered, duplicate-preserving list of every instrument/manipulator line found between this
## cab's `cabNdefinition:` and the following `cab0definition:`/EOF.
var instruments:Array[MmdInstrumentDescriptor] = []

## Every `pyscreen:` of this cab, in file order, with the update interval already resolved.
var python_screens:Array[MmdPythonScreenDescriptor] = []

## Parse-time diagnostics (severity/code/source_file/line/cabin_number/mmd_label/
## submodel_name/message), collected while building this definition. MmdCabinInstancer.
## build_into() appends its own build-time diagnostics to the same shape separately.
var diagnostics:Array[Dictionary] = []
