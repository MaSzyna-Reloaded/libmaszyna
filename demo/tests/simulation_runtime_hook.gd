extends GutHookScript

## The tests drive the simulation themselves: no SimulationRuntime is placed, so no frame moves
## SimulationServer's clock - a test steps it tick by tick (MaszynaGutTest.step(),
## wait_simulated_until()), and its result does not depend on how fast the machine renders frames.
## The simulation is unpaused and at speed 1 for every test.


func run() -> void:
    SimulationServer.simulation_unpause()
    SimulationServer.simulation_reset_speed()
