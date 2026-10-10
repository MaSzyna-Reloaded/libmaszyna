@tool
extends Resource
class_name SmokeSource

## Parameters of one particle emitter template, as the original reads them from
## [code]data/smokesource_*.txt[/code] ([code]smoke_source::deserialize_mapping()[/code],
## [code]particles.cpp:92[/code]). The defaults are the original's own
## ([code]particles.h:117-160[/code]).


## Particles per second
@export var spawn_rate:float = 0.0
## Polar angle off the emitter's up axis, in degrees
@export var inclination_min:float = 0.0
@export var inclination_max:float = 0.0
## Launch speed, m/s
@export var velocity_min:float = 1.0
@export var velocity_max:float = 1.0
## Billboard size multiplier at spawn
@export var size_min:float = 1.0
@export var size_max:float = 1.0
@export var opacity_min:float = 1.0
@export var opacity_max:float = 1.0
## Declared as 0-255 RGB in the template
@export var color:Color = Color8(16, 16, 16)
## Linear growth per second, clamped to the limits below
@export var size_step:float = 0.0
@export var size_limit_min:float = 0.0
@export var size_limit_max:float = INF
## Linear fade per second (negative), clamped to the limits below
@export var opacity_step:float = 0.0
@export var opacity_limit_min:float = 0.0
@export var opacity_limit_max:float = 1.0


## The original has no lifetime parameter: a particle dies once its opacity reaches zero, and the
## particle budget is derived from that (particles.cpp:122-133). Returns 0 for a template with no
## fade, which the original divides by (its own division by zero).
func get_particle_lifetime() -> float:
    if not opacity_step:
        return 0.0
    return opacity_max / absf(opacity_step)


## How many particles the emitter may hold at once - the spawn rate over one [param lifetime],
## capped the way the original caps it (particles.cpp:128). [param density] multiplies the rate, so
## the pool has to grow with it or the emitter runs out of slots and stops spawning.
##
## The lifetime is passed in rather than taken from the template: a static emitter is given a
## shorter one than the template asks for (see [code]SmokeSourceLibrary[/code]).
func get_particle_amount(max_particles:int, density:float, lifetime:float) -> int:
    return clampi(ceili(spawn_rate * density * lifetime), 0, max_particles)


## Terminal billboard size of a particle that lives a full [param lifetime]
func get_terminal_size(lifetime:float) -> float:
    return clampf(get_mean_size() + size_step * lifetime, size_limit_min, size_limit_max)


func get_mean_size() -> float:
    return (size_min + size_max) * 0.5
