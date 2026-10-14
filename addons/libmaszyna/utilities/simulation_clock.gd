class_name SimulationClock
extends RefCounted

## The simulation's seconds between a node's frames, for whatever runs on frames but moves with the
## simulation - a cab element's animation, a lamp's refresh, a vehicle's running sound: it stands
## while the simulation stands and runs at its speed. The editor has no simulation: a frame there is
## the preview's own time.

var _simulation_time:float = SimulationServer.simulation_get_time()


## The simulated seconds since the node's previous frame - `frame_delta` in the editor
func advance(frame_delta:float) -> float:
    if Engine.is_editor_hint():
        return frame_delta
    var now:float = SimulationServer.simulation_get_time()
    var seconds:float = now - _simulation_time
    _simulation_time = now
    return seconds
