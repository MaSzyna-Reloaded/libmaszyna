@tool
extends ScenarioEventAction
class_name MaszynaTextureAction

## The original's `texture` event (texture_event, Event.cpp:1474-1543): its models' replaceable skin
## of `skin_index` becomes `skin` - a texture, or one a script draws (`make:`, the material manager's
## Python screens). With a memory cell, the cell's text and values fill the skin's `(p1)`-`(p3)`, the
## values as whole numbers (Event.cpp:1521-1530).

## A parameter of the skin and what fills it: the cell's text, value 1, value 2
const PARAMETERS:Array[String] = ["(p1)", "(p2)", "(p3)"]

## The models' ModelRenderingServer instances
var instances:Array[RID] = []
## 1-4, the replaceable skin taken (TAnimModel::SkinSet(), AnimModel.cpp:690-693)
@export var skin_index:int = 1
@export var skin:String = ""
## The memory cell filling the skin's parameters, none for a skin used as it stands
var source:RID = RID()


func _run(_event:RID, _activator:RID) -> void:
    var name:String = skin
    if source.is_valid():
        var values:Array[String] = [
            MaszynaScenarioEventServer.memory_get_text(source),
            "%.0f" % MaszynaScenarioEventServer.memory_get_value1(source),
            "%.0f" % MaszynaScenarioEventServer.memory_get_value2(source),
        ]
        for index:int in PARAMETERS.size():
            name = name.replace(PARAMETERS[index], values[index])
    for instance:RID in instances:
        MaszynaE3DRenderingServer.instance_set_skin(instance, skin_index - 1, name)
