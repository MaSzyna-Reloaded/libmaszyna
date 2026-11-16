class_name MaszynaCabinImplementation
extends CabinImplementation

## MaSzyna's cabs as CabinSystem knows them (CabinSystem.implementation_register()): the roles of an
## MMD file's cab, as MmdSemanticCatalog has them, and their widgets with the catalog's defaults

## The name the cabs built of an MMD file give (Cabin3D.implementation)
const NAME:StringName = &"maszyna"
## The fields of a catalog entry that name a vehicle command the control sends
const COMMAND_FIELDS:Array[String] = ["command", "command_increase", "command_decrease"]


func _component_list() -> Array[CabinComponentInfo]:
    var list:Array[CabinComponentInfo] = []
    for label:String in MmdSemanticCatalog.get_labels():
        var entry:Dictionary = MmdSemanticCatalog.get_entry(label)
        var fields:Dictionary = entry.get("fixed_fields", {})
        var commands:PackedStringArray = []
        for field:String in COMMAND_FIELDS:
            var command:String = fields.get(field, "")
            if command and not command in commands:
                commands.append(command)
        var info := CabinComponentInfo.new()
        info.name = StringName(label)
        info.title = MmdCabControlCaptions.caption(StringName(label))
        info.widget_class = (entry["widget_class"] as Script).get_global_name()
        info.commands = commands
        list.append(info)
    return list


## The widget of a role with the catalog's defaults: a button of the plain type, a switch to the
## positions the catalog gives - no vehicle shapes it
func _component_create(name:StringName) -> Node3D:
    if not MmdSemanticCatalog.has_label(name):
        return null
    var widget:Node3D = MmdCabinInstancer.widget_create(name, CabinButton.ButtonType.TOGGLE, {})
    widget.name = name
    return widget
