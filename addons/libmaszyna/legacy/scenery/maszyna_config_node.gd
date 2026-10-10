@tool
extends Node
class_name MaszynaConfigNode

## Marker produced by a scenery's "config" section, carrying the entries a scenery is known to
## set there. Original engine's deserialize_config() (simulationstateserializer.cpp:300) hands the
## section to Global.ConfigParse(); MaszynaSceneryNode applies the values after a load.
const KEY_DAY_OF_YEAR:String = "movelight"
const KEY_TEMPERATURE:String = "scenario.weather.temperature"
const KEYS:Array[String] = [KEY_DAY_OF_YEAR, KEY_TEMPERATURE]

## Only the entries present in the section, as written in the file
@export var values:Dictionary[String, String] = {}
