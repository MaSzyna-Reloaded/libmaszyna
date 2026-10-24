extends MaszynaGutTest

## The weather in the AI driver: rain and snow take the rails' grip (Global.FrictionWeatherFactor,
## simulationenvironment.cpp:198-201), so it brakes the earlier (braking_distance_multiplier(),
## Driver.cpp:1733)

## A target fast enough for no stretching of its own, and the vehicle's speed [km/h]
const FAST_TARGET:float = 100.0
const VEHICLE_SPEED:float = 80.0
const TOLERANCE:float = 0.0001


func after_each() -> void:
    SimulationServer.weather = SimulationServer.WEATHER_CLEAR


func test_rain_and_snow_lengthen_the_braking_distance() -> void:
    var braking:MaszynaDriverBraking = MaszynaDriverBraking.new()
    var trainset:MaszynaDriverTrainset = MaszynaDriverTrainset.new()
    SimulationServer.weather = SimulationServer.WEATHER_CLEAR
    var clear:float = braking.distance_multiplier(FAST_TARGET, VEHICLE_SPEED, trainset, RailVehicleServer.TRAINSET_TYPE_PASSENGER)
    for weather:SimulationServer.Weather in [SimulationServer.WEATHER_RAIN, SimulationServer.WEATHER_SNOW]:
        SimulationServer.weather = weather
        assert_almost_eq(braking.distance_multiplier(FAST_TARGET, VEHICLE_SPEED, trainset, RailVehicleServer.TRAINSET_TYPE_PASSENGER),
                clear / SimulationServer.simulation_get_weather_friction_factor(), TOLERANCE, "weather %d" % weather)
