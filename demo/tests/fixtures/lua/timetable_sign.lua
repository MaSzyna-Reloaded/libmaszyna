-- A `make:` texture drawn by Lua: its drawing, the vehicle the request's `$timetable=` names and the
-- request's other keys - the train read from the run of the vehicle's trainset
local drawing, vehicle, parameters = ...
assert(drawing ~= nil, "the drawing comes first")
assert(reloaded.vehicle.id(vehicle) == "loco", "the vehicle $timetable= names")
assert(parameters.kod_e == "IC" and parameters._id1 == "IC 1234", "the request's other keys")
assert(parameters["$timetable"] == nil, "no dataset key")
local run = reloaded.station.run(vehicle)
local timetable = reloaded.station.timetable(run)
local number = reloaded.station.train(timetable)
assert(number == "IC 1234", "the train its trainset runs")
assert(reloaded.station.name(reloaded.station.destination(timetable)) == "Olkusz", "its destination")
assert(reloaded.station.entry(timetable, 2).arrival == 8.5, "its timetable's entries, from 1")
reloaded.draw.clear(drawing, {255, 170, 0})
