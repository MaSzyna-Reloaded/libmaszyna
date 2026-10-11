extends SignallingImplementation

## Demo automatic block: every `next` event moves each signal head of the system to the next aspect
## its kind declares (sbl_3.tres: S1, S5, S2, S3), and back to the first after the last.

const NEXT_EVENT: StringName = &"next"


func _handle_event(system: RID, event: StringName, _arguments: Dictionary) -> void:
    if not event == NEXT_EVENT:
        return
    for signal_head: RID in SignallingServer.system_get_signal_heads(system):
        var aspects: PackedStringArray = SignallingServer.signal_head_get_aspects(signal_head)
        if not aspects:
            continue
        var current: int = aspects.find(SignallingServer.signal_head_get_aspect(signal_head))
        SignallingServer.signal_head_set_aspect(signal_head, aspects[(current + 1) % aspects.size()])
