extends Node
class_name CabinWindscreenWipers

## Feeds the wipers of the occupied vehicle to the "rain_windscreen" materials, as the original
## renderer does for the owner of the camera (opengl33renderer.cpp:755-789): per wiper its
## position (0..1 sweeping out, 1..2 on the way back) and the time since it last left the parked
## position and the far end. The original passes the two as points in time; the shader clock is
## not available here, so they are passed as the time elapsed.

const WIPERS:int = 4
const NEVER:float = 1000.0

## Set by the factory before the node enters the tree.
## Which vehicle these wipers belong to; the state comes through CabinSystem.
var vehicle_rid:RID

var _since_out:Vector4 = Vector4(NEVER, NEVER, NEVER, NEVER)
var _since_return:Vector4 = Vector4(NEVER, NEVER, NEVER, NEVER)
## The windscreen's time - the simulation's, as the wipers it follows
var _clock:SimulationClock = SimulationClock.new()


func _process(frame_delta:float) -> void:
    var delta:float = _clock.advance(frame_delta)
    var positions:PackedFloat64Array = CabinSystem.vehicle_state(vehicle_rid).get("wiper_positions", PackedFloat64Array())
    var cab:int = CabinSystem.vehicle_state(vehicle_rid).get("cabin", 0)
    var wiper_pos:Vector4 = Vector4.ZERO
    for i:int in WIPERS:
        if cab == 0 or i >= positions.size():
            _since_out[i] = NEVER
            _since_return[i] = NEVER
            continue
        # the wipers are numbered from the side of the active cab
        var position:float = positions[i if cab > 0 else positions.size() - 1 - i]
        var sweep:float = position if position <= 1.0 else position - 1.0
        # The original hands the shader the plain position while the arms move eased
        # (smoothInterpolate(), DynObj.cpp:726), so the wiped edge runs up to a tenth of the sweep
        # away from the blade. Eased here as well, to keep the two together.
        var eased:float = sweep * sweep * (3.0 - 2.0 * sweep)
        wiper_pos[i] = eased if position <= 1.0 else eased + 1.0
        _since_out[i] = 0.0 if sweep < 0.025 else _since_out[i] + delta
        _since_return[i] = 0.0 if sweep > 0.975 else _since_return[i] + delta
    _apply(wiper_pos)


func _exit_tree() -> void:
    _since_out = Vector4(NEVER, NEVER, NEVER, NEVER)
    _since_return = Vector4(NEVER, NEVER, NEVER, NEVER)
    _apply(Vector4.ZERO)


func _apply(wiper_pos:Vector4) -> void:
    RenderingServer.global_shader_parameter_set("maszyna_wiper_pos", wiper_pos)
    RenderingServer.global_shader_parameter_set("maszyna_wiper_since_out", _since_out)
    RenderingServer.global_shader_parameter_set("maszyna_wiper_since_return", _since_return)
