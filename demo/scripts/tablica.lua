-- The station board of Dworzec Olkusz: the next train's departure, scrolling from right to left.
-- Run by the board's LuaTextureNode, which hands it the drawing - the board's LEDs, drawn through
-- the LED matrix material into the board's texture.
local board = ...

-- whole pixels: a LED is lit or dark
local font = reloaded.draw.load_font("fonts/DejaVuSansMono.ttf", "none")
-- Set in the inspector of the board's node (Script Variables)
local TEXT = reloaded.export_var("text", "Fill the text")
local TEXT_SIZE = reloaded.export_var("text_size", 12)
-- White leaves the board's LED colour to its material (led_color)
local LIT = reloaded.export_var("color", {255, 255, 255})
-- How far the text moves each step [LEDs] and how often it steps [s]
local STEP = reloaded.export_var("step", 2)
local STEP_SECONDS = reloaded.export_var("step_seconds", 0.07)
local BLACK = {0, 0, 0}

local width, height = reloaded.draw.size(board)
local text_width, text_height = reloaded.draw.text_size(font, TEXT, TEXT_SIZE)
local top = math.floor((height - text_height) / 2)
local x = width

reloaded.time.every(STEP_SECONDS, function()
    reloaded.draw.clear(board, BLACK)
    reloaded.draw.text(board, x, top, TEXT, font, TEXT_SIZE, LIT)
    x = x - STEP
    if x < -text_width then
        x = width
    end
end)
