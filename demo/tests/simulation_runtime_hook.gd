extends GutHookScript

## The tests drive the simulation themselves: no SimulationRuntime is placed, so no frame moves
## SimulationServer's clock - a test steps it tick by tick (MaszynaGutTest.step(),
## wait_simulated_until()), and its result does not depend on how fast the machine renders frames.
## The simulation is unpaused and at speed 1 for every test.


func run() -> void:
    # no ResourceCache read or written: a test builds everything from its fixtures, never from what
    # a previous run compiled (a stale scenery cache read as a regression, FINDINGS.md 2026-10-12)
    ProjectSettings.set_setting("maszyna/cache/enabled", false)
    # loaded at registration, on the main thread: the headless dummy renderer takes no resource made
    # on a worker (ResourceLazyLoader); a test of lazy loading turns it on itself, with loaders of its
    # own that make none
    ResourceLazyLoader.lazy_loading = false
    SimulationServer.simulation_unpause()
    SimulationServer.simulation_reset_speed()
