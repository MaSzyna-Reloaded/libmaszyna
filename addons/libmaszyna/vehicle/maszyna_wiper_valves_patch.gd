@tool
extends MaszynaVehicleModelElementPatch
class_name MaszynaWiperValvesPatch

## How the wipers of a FIZ of no WiperList go by their valves (reloaded_wipervalve<N>): each vehicle's
## air motors have their own - an SM42's valve very sensitive, its wiper at full "going as if to fall
## off"; an EU07's far calmer.

## A sweep's time at a valve just cracked open, with a full reservoir [s]
@export_range(0.01, 10.0, 0.01, "or_greater") var slowest_sweep_time:float = 1.0
## A sweep's time at a full valve, with a full reservoir [s]
@export_range(0.01, 10.0, 0.01, "or_greater") var fastest_sweep_time:float = 1.0
## How much of the way to the fastest the start of the turn gives: the share at an opening is
## 1 - (1 - opening)^this - 1 even along the turn, 3 most of it in the first quarter
@export_range(0.1, 10.0, 0.1, "or_greater") var sensitivity:float = 1.0
