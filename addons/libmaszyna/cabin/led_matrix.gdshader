// A picture shown as a LED board: every pixel of the drawing is one LED, lit by the glow of the LEDs
// around it. A LuaTextureNode's material: its drawing is the LEDs, its texture the board.
//
// Its style is the game data's LED screens' (led_util.render_post(), scripts/led_util.py:201) - each
// LED a square cell of the texture darkened by a grid - or round LEDs with dark gaps between them,
// the unlit ones faintly seen, as on a real board.
shader_type canvas_item;
render_mode blend_disabled;

// The LEDs as the game data's screens draw them, or round ones
uniform int style : hint_enum("Squares (MaSzyna)", "Round") = 0;
// Each LED gathers the light of its neighbours (render_post(bloom = True))
uniform bool bloom = true;
// The colour of a lit LED: the drawing's colours are multiplied by it - white keeps them, a drawing
// in white takes it whole
uniform vec3 led_color : source_color = vec3(1.0);
// A round LED's radius, of its cell's side
uniform float led_radius : hint_range(0.1, 0.5) = 0.4;
// How soft a round LED's edge is, of its cell's side
uniform float led_edge : hint_range(0.0, 0.2) = 0.06;
// The colour of an unlit round LED
uniform vec3 unlit_color : source_color = vec3(0.05, 0.03, 0.01);
// How much of the glow lights the gaps between round LEDs
uniform float gap_glow : hint_range(0.0, 1.0) = 0.25;

const int STYLE_SQUARES = 0;
// How far a LED gathers its neighbours' light [LEDs] (led_util.py:214, r)
const int GLOW_RADIUS = 6;
// The glow's falloff: a neighbour's light is divided by FALLOFF plus its squared distance
// (led_util.py:223, 0.08)
const float GLOW_FALLOFF = 0.08;
// Farther than this squared distance a neighbour gives no light (led_util.py:247, 20)
const float GLOW_CUTOFF = 20.0;
// How much darker each half of a cell beyond its first is - the grid (led_util.py:227, 0.15)
const float GRID_DARKENING = 0.15;
// The half of a cell a pixel lies in from: a cell is two pixels a side in the original
const float CELL_HALF = 0.5;

void fragment() {
	ivec2 leds = textureSize(TEXTURE, 0);
	vec2 position = UV * vec2(leds);
	ivec2 led = clamp(ivec2(floor(position)), ivec2(0), leds - 1);
	vec3 lit = texelFetch(TEXTURE, led, 0).rgb;
	vec3 light = lit;
	if (bloom) {
		// get_px_str(), led_util.py:235
		light = vec3(0.0);
		for (int x = max(led.x - GLOW_RADIUS, 0); x < min(led.x + GLOW_RADIUS + 1, leds.x); x++) {
			float across = GLOW_FALLOFF + float((x - led.x) * (x - led.x));
			if (across > GLOW_CUTOFF) {
				continue;
			}
			for (int y = max(led.y - GLOW_RADIUS, 0); y < min(led.y + GLOW_RADIUS + 1, leds.y); y++) {
				float distance = across + float((y - led.y) * (y - led.y));
				if (distance > GLOW_CUTOFF) {
					continue;
				}
				light += texelFetch(TEXTURE, ivec2(x, y), 0).rgb / distance;
			}
		}
		light *= GLOW_FALLOFF;
	}
	light *= led_color;
	if (style == STYLE_SQUARES) {
		vec2 half_cell = step(vec2(CELL_HALF), fract(position));
		COLOR = vec4(light * (1.0 - (half_cell.x + half_cell.y) * GRID_DARKENING), 1.0);
	} else {
		float from_centre = length(fract(position) - CELL_HALF);
		float on_led = 1.0 - smoothstep(led_radius - led_edge, led_radius, from_centre);
		vec3 gap = bloom ? light * gap_glow : vec3(0.0);
		COLOR = vec4(mix(gap, max(light, unlit_color), on_led), 1.0);
	}
}
