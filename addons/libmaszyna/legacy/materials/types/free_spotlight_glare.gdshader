// The glare of a free spotlight at night, in fog or in rain: a 2.5 m billboard added on top without
// a depth test (opengl33renderer.cpp:4646-4700, billboard.frag). Departures from the original, whose
// hard edges made the glare read as an overlay:
// - the original turns it towards the camera only around the vertical, which flattens it seen from
//   above; here it always faces the camera
// - the original draws it only if its point passed the depth test (a conditional render on the
//   point's occlusion query), all or nothing; here the scene's depth is sampled at the light's
//   centre and on a ring around it, and the glare fades with the share of samples that see it
// - the cone fades it over its own band, from glare_fade_start of the falloff angle out to the
//   falloff, unless the model's hotspot starts it earlier: vehicle lamps declare a 1 degree band
//   (sm42 fspot: 21.5-22.5), which made the glare blink on and off
// - glare_intensity scales the whole
// - the original's glare is the same star whatever the angle; here its rays turn as the camera goes
//   round the light (glare_rotation_ratio) and it shrinks towards the cone's edge
//   (glare_edge_size), so it does not read as a sticker
// - from afar it keeps at least glare_min_screen_size of the screen's height, where the original's
//   2.5 m would shrink to a speck

shader_type spatial;
render_mode unshaded, blend_add, depth_draw_never, depth_test_disabled, cull_disabled, shadows_disabled, fog_disabled;

// The original's Global.fLuminance and Global.Overcast; set by MaszynaEnvironmentNode
global uniform float maszyna_light_level;
global uniform float maszyna_overcast;

instance uniform vec3 light_color : source_color = vec3(1.0);
instance uniform float cos_hotspot_angle = 1.0;
instance uniform float cos_falloff_angle = 0.0;
instance uniform float max_distance = 15000.0;
// fLight: the glare shows below this light level
instance uniform float lights_on_threshold = 0.0;

// fx/lightglare, bound by MaterialManager
uniform sampler2D glare_texture : source_color, filter_linear_mipmap;
uniform sampler2D depth_texture : hint_depth_texture, filter_nearest;
// maszyna/scenery/railway_lights_glare_intensity, set by MaterialManager
uniform float glare_intensity = 1.0;
// maszyna/scenery/railway_lights_glare_fade_start, set by MaterialManager
uniform float glare_fade_start = 1.0;
// maszyna/scenery/railway_lights_glare_rotation_ratio, set by MaterialManager
uniform float glare_rotation_ratio = 0.0;
// maszyna/scenery/railway_lights_glare_edge_size, set by MaterialManager
uniform float glare_edge_size = 1.0;
// maszyna/scenery/railway_lights_glare_min_screen_size, set by MaterialManager
uniform float glare_min_screen_size = 0.0;

#include "res://addons/libmaszyna/legacy/materials/types/free_spotlight.gdshaderinc"

// half of the billboard's 2.5 m (opengl33renderer.cpp:110)
const float GLARE_HALF_SIZE = 1.25;
// the glare fades out as the light level rises to this (opengl33renderer.cpp:4390)
const float GLARE_LUMINANCE = 0.6;
// above this overcast it rains or snows, and the glare shows whatever the light level
const float GLARE_OVERCAST = 1.0;
// the depth samples on the ring around the light's centre, and the ring's radius on the screen
const int OCCLUSION_RING_TAPS = 8;
const float OCCLUSION_RADIUS_PIXELS = 4.0;

varying float v_glare;
varying vec4 v_center_clip;

void vertex() {
	float square_distance = free_spotlight_square_distance(MODELVIEW_MATRIX);
	vec4 center_view = free_spotlight_center(MODELVIEW_MATRIX, square_distance, max_distance);
	float view_angle = acos(clamp(free_spotlight_cos_view(MODELVIEW_MATRIX, center_view), -1.0, 1.0));
	float falloff_angle = acos(clamp(cos_falloff_angle, -1.0, 1.0));
	float fade_start_angle = min(acos(clamp(cos_hotspot_angle, -1.0, 1.0)), falloff_angle * glare_fade_start);
	float angle_factor = 1.0 - smoothstep(fade_start_angle, falloff_angle, view_angle);
	v_glare = 0.0;
	if (free_spotlight_in_range(square_distance, max_distance)
			&& (maszyna_light_level < lights_on_threshold || maszyna_overcast > GLARE_OVERCAST)) {
		v_glare = clamp(max(GLARE_LUMINANCE - maszyna_light_level, maszyna_overcast - GLARE_OVERCAST), 0.0, 1.0)
				* angle_factor;
	}
	// the camera's angles off the light's axis (-Z), sideways and up, in the light's own frame
	vec3 to_camera = normalize(-center_view.xyz);
	float forward = dot(to_camera, -normalize(MODELVIEW_MATRIX[2].xyz));
	float side_angle = atan(dot(to_camera, normalize(MODELVIEW_MATRIX[0].xyz)), forward);
	float up_angle = atan(dot(to_camera, normalize(MODELVIEW_MATRIX[1].xyz)), forward);
	float rotation = glare_rotation_ratio * (side_angle + up_angle);
	mat2 turn = mat2(vec2(cos(rotation), sin(rotation)), vec2(-sin(rotation), cos(rotation)));
	// the half size at which the whole glare spans glare_min_screen_size of the screen's height
	float min_half_size = glare_min_screen_size * -center_view.z / PROJECTION_MATRIX[1][1];
	float half_size = max(GLARE_HALF_SIZE, min_half_size) * mix(glare_edge_size, 1.0, angle_factor);
	vec3 corner = center_view.xyz + vec3(turn * VERTEX.xy, 0.0) * half_size;
	POSITION = PROJECTION_MATRIX * vec4(corner, 1.0);
	v_center_clip = PROJECTION_MATRIX * center_view;
	UV = vec2(0.5 - VERTEX.x * 0.5, 0.5 + VERTEX.y * 0.5);
}

void fragment() {
	if (v_glare <= 0.0) {
		discard;
	}
	if (v_center_clip.w <= 0.0) {
		discard;
	}
	vec3 center_ndc = v_center_clip.xyz / v_center_clip.w;
	vec2 center_uv = center_ndc.xy * 0.5 + 0.5;
	vec2 radius_uv = OCCLUSION_RADIUS_PIXELS / VIEWPORT_SIZE;
	float visible_taps = 0.0;
	for (int tap = 0; tap <= OCCLUSION_RING_TAPS; tap++) {
		// tap 0 is the centre, the rest lie on the ring
		float angle = TAU * float(tap) / float(OCCLUSION_RING_TAPS);
		vec2 tap_uv = center_uv + (tap == 0 ? vec2(0.0) : vec2(cos(angle), sin(angle)) * radius_uv);
		// a tap off the screen sees nothing; reversed depth: the scene is in front of the light
		// when its depth is greater
		if (all(greaterThanEqual(tap_uv, vec2(0.0))) && all(lessThanEqual(tap_uv, vec2(1.0)))
				&& texture(depth_texture, tap_uv).r <= center_ndc.z) {
			visible_taps += 1.0;
		}
	}
	if (visible_taps <= 0.0) {
		discard;
	}
	vec4 glare = texture(glare_texture, UV);
	ALBEDO = glare.rgb * light_color;
	ALPHA = glare.a * v_glare * glare_intensity * visible_taps / float(OCCLUSION_RING_TAPS + 1);
}
