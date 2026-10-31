extends RefCounted
class_name MmdManualWiperHandles

## Quirk: the wipers of a vehicle whose data names none (no animwiperprefix:, no WiperList:) and the
## handles that move them by hand (RailVehicleWipers.DRIVE_MANUAL) - an old cab's, the EU07's
## wycieracz01/02 and mech_wyc_raczka01/02 - have no key of the MMD: the original moves none of
## them. They are found by the names the game's models give them (a number after a name counts
## them; _lewa and _prawa a pair) and mapped: a cab's handles and wipers get a reloaded_manualwiper<N>
## knob of each (MmdSemanticCatalog) - the handle turned by the mouse, the wiper of the same number
## following it across its sweep - and a vehicle model's wipers are its wiper arms
## (MaszynaRailVehicle3DInstancer). When the data gets a key of its own for them, the key is mapped
## there and this finds none to add.

const HANDLE_NAMES:Array[String] = ["mech_wyc_raczka", "raczka_wyc", "mech_wyc_r"]
const WIPER_NAMES:Array[String] = ["wycieraczka", "wycieracz", "wiper"]
## The suffixes of a pair of wipers, the left one first
const PAIR_SUFFIXES:Array[String] = ["_lewa", "_prawa"]
const LABEL_PREFIX:String = "reloaded_manualwiper"
## How far a handle and a wiper turn across the sweep, of a full turn (an MMD "rot" scale): the
## models give no range
const SWEEP_TURN:float = 0.25


## The knobs of the cab model's handles and wipers added to the cab's definition - none of a cab
## with them already, or of no model
static func add_to(definition:MmdCabinDefinition, data_path:String) -> void:
    if not definition.model_relpath or definition.instruments.any(
            func(descriptor:MmdInstrumentDescriptor) -> bool: return descriptor.label.begins_with(LABEL_PREFIX)):
        return
    var model:MaszynaE3DModel = MaszynaE3DModelManager.load_model(data_path, definition.model_relpath)
    if not model:
        return
    var names:Array[String] = []
    _index_names(model.submodels, names)
    # the handle first: the first widget of a label is its control (MmdCabinInstancer.build_into)
    for parts:PackedStringArray in [find(names, HANDLE_NAMES), find(names, WIPER_NAMES)]:
        for index:int in mini(parts.size(), RailVehicleWipers.MANUAL_WIPERS_MAX):
            var descriptor:MmdInstrumentDescriptor = MmdInstrumentDescriptor.new()
            descriptor.label = LABEL_PREFIX + str(index + 1)
            descriptor.submodel_name = parts[index]
            descriptor.animation_type = "rot"
            descriptor.scale = SWEEP_TURN
            definition.instruments.append(descriptor)


## The names among `names` (the model's submodels') of one of `prefixes` numbered or paired, in
## their order
static func find(names:Array[String], prefixes:Array[String]) -> PackedStringArray:
    var found:PackedStringArray = []
    for submodel_name:String in names:
        var name_lower:String = submodel_name.to_lower()
        for prefix:String in prefixes:
            if not name_lower.begins_with(prefix):
                continue
            var rest:String = name_lower.trim_prefix(prefix).trim_prefix("_")
            if name_lower == prefix or rest.is_valid_int() or name_lower.trim_prefix(prefix) in PAIR_SUFFIXES:
                found.append(submodel_name)
                break
    found.sort()
    return found


static func _index_names(submodels:Array, names:Array[String]) -> void:
    for submodel:MaszynaE3DSubModel in submodels:
        names.append(submodel.resource_name)
        if submodel.submodels:
            _index_names(submodel.submodels, names)
