-- The station clock of Dworzec Olkusz: the simulation's time of day on a round face. Both faces of
-- the clock show one texture, so the face is drawn once for both.
local face = ...

local WHITE = {255, 255, 255}
local BLACK = {20, 20, 20}
local RED = {200, 0, 0}
local FRAME = {60, 60, 60}
-- The face's rim and the marks of the minutes and the hours, as shares of its radius
local RIM = 4
local MARK_FROM = 0.85
local HOUR_MARK_FROM = 0.75
local MARK_TO = 0.95
-- The hands: their length as a share of the radius, their width [px]
local HOUR_HAND = {0.5, 6}
local MINUTE_HAND = {0.8, 4}
local SECOND_HAND = {0.9, 1}
local DEGREES_PER_HOUR = 30
local DEGREES_PER_MINUTE = 6
-- How often the face is drawn again [s of the clock on the wall]
local REDRAW_SECONDS = 0.5

local width, height = reloaded.draw.size(face)
local cx, cy = width / 2, height / 2
local radius = width / 2 - RIM

-- A line from the middle, at the angle [deg] clockwise from twelve
local function ray(angle, from, to, colour, line_width)
    local a = math.rad(angle - 90)
    reloaded.draw.line(face, {
        cx + math.cos(a) * radius * from, cy + math.sin(a) * radius * from,
        cx + math.cos(a) * radius * to, cy + math.sin(a) * radius * to,
    }, colour, line_width)
end

local function draw()
    reloaded.draw.clear(face, FRAME)
    reloaded.draw.ellipse(face, RIM, RIM, width - 2 * RIM, height - 2 * RIM, WHITE)
    for minute = 0, 59 do
        local hour_mark = minute % 5 == 0
        ray(minute * DEGREES_PER_MINUTE, hour_mark and HOUR_MARK_FROM or MARK_FROM, MARK_TO, BLACK,
            hour_mark and 4 or 1)
    end
    local hours = reloaded.sim.time_of_day()
    local minutes = (hours % 1) * 60
    local seconds = (minutes % 1) * 60
    ray((hours % 12) * DEGREES_PER_HOUR, 0, HOUR_HAND[1], BLACK, HOUR_HAND[2])
    ray(minutes * DEGREES_PER_MINUTE, 0, MINUTE_HAND[1], BLACK, MINUTE_HAND[2])
    ray(seconds * DEGREES_PER_MINUTE, 0, SECOND_HAND[1], RED, SECOND_HAND[2])
end

draw()
reloaded.time.every(REDRAW_SECONDS, draw)
