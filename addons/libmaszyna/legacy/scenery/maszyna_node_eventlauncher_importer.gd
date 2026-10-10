@tool
extends RefCounted

const NONE:String = "none"
## Words that end the list of events when the second one is left out (EvLaunch.cpp:89-99)
const EVENTS_END:Array[String] = ["end", "condition", "traintriggered"]
## `<memcell> <text> <value1> <value2>`
const CONDITION_TOKENS:int = 4


func import(p:MaszynaParser, context: MaszynaImporterContext) -> MaszynaEventLauncherData:
    var launcher:MaszynaEventLauncherData = MaszynaEventLauncherData.new()
    var x:float = float(p.next_token())
    var y:float = float(p.next_token())
    var z:float = float(p.next_token())
    launcher.position = Vector3(x, y, z)
    launcher.radius = float(p.next_token())
    var key:String = p.next_token().to_lower()
    launcher.key = "" if key == NONE else key
    launcher.delta_time = float(p.next_token())
    var event1:String = p.next_token().to_lower()
    launcher.event1 = "" if event1 == NONE else event1
    var token:String = p.next_token().to_lower()
    if not token in EVENTS_END:
        launcher.event2 = "" if token == NONE else token
        token = p.next_token().to_lower()
    if token == "condition":
        for i:int in CONDITION_TOKENS:
            launcher.condition.append(p.next_token().to_lower())
        token = p.next_token().to_lower()
    launcher.train_triggered = token == "traintriggered"
    if not token == "end":
        p.get_tokens_until("end")
    return launcher
