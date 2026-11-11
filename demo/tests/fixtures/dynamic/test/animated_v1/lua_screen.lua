-- The screen of lua_screen.mmd: what MaszynaScreenLuaNode hands its script
local drawing, vehicle, cabin, parameters, update_time = ...
local width, height = reloaded.draw.size(drawing)
assert(width > 0 and height > 0, "the screen's drawing")
assert(parameters.kod == "ic", "the screen's parameters, in lower case")
assert(math.abs(update_time - 0.3) < 1e-9, "the screen's update interval [s]")
reloaded.draw.clear(drawing, {0, 0, 255})
