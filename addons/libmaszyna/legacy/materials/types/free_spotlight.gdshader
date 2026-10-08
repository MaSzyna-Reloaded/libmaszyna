// A free spotlight (TP_FREESPOTLIGHT) seen from afar: a round point of a constant size on the
// screen, the way the original draws it (opengl33renderer.cpp:4441-4500, freespot.vert/.frag), so
// a signal stays visible long after its lens is smaller than a pixel. The quad is expanded in clip
// space around the light's centre; the light points along the submodel's -Z. The glare is the next
// pass (free_spotlight_glare.gdshader).

shader_type spatial;
render_mode unshaded, blend_mix, depth_draw_never, cull_disabled, shadows_disabled, fog_disabled;

// The original's Global.Overcast (0-1 clouds, above 1 precipitation) and the scene's fog range,
// m_fogrange (opengl33renderer.cpp:5009); set by MaszynaEnvironmentNode
global uniform float maszyna_overcast;
global uniform float maszyna_fog_range;

// The submodel's own colour (or the scenery node's lightcolors), its cone and its range
instance uniform vec3 light_color : source_color = vec3(1.0);
instance uniform float cos_hotspot_angle = 1.0;
instance uniform float cos_falloff_angle = 0.0;
instance uniform float max_distance = 15000.0;

#include "res://addons/libmaszyna/legacy/materials/types/free_spotlight.gdshaderinc"

// pointsize = max(3, 5 * distancefactor * anglefactor) in pixels (opengl33renderer.cpp:4451)
const float POINT_SIZE_MIN = 3.0;
const float POINT_SIZE_SCALE = 5.0;
// how many times pointsize the point is drawn; the original's 4 (:4488) makes blobs of the lights -
// maszyna/scenery/railway_lights_point_size_multiplier, set by MaterialManager
uniform float point_size_multiplier = 2.0;
// distancefactor never drops below this (opengl33renderer.cpp:4450)
const float DISTANCE_FACTOR_MIN = 0.5;
// freespot.frag:20 discards outside half the point's radius
const float DISC_RADIUS = 0.5;
// Precipitation dims the far points above this overcast (opengl33renderer.cpp:4453-4457)
const float PRECIPITATION_DIMMING_OVERCAST = 0.75;
const float PRECIPITATION_DIMMING_MIN = 0.25;
const float PRECIPITATION_DIMMING_SLOPE = 0.75;
const float PRECIPITATION_DIMMING_OFFSET = 0.5;
// The fog halo: drawn above this overcast at half the alpha, its size scaled by the fog
// (opengl33renderer.cpp:4478-4484)
const float FOG_HALO_OVERCAST = 1.0;
const float FOG_HALO_ALPHA = 0.5;
const float FOG_HALO_SCALE_NEAR = 1.5;
const float FOG_HALO_FOG_END = 2000.0;
// The lights see through twice the scene's fog (opengl33renderer.cpp:4471). The original also never
// lets it drop below 500 m, which leaves a signal bright far into a 10 m fog - left out.
const float LIGHT_FOG_RANGE_SCALE = 2.0;

varying float v_light_level;
varying float v_point_share;
varying float v_halo_share;
varying float v_fog_visibility;

void vertex() {
	float square_distance = free_spotlight_square_distance(MODELVIEW_MATRIX);
	vec4 center_view = free_spotlight_center(MODELVIEW_MATRIX, square_distance, max_distance);
	float angle_factor = free_spotlight_angle_factor(MODELVIEW_MATRIX, center_view, cos_hotspot_angle, cos_falloff_angle);
	float square_max_distance = max_distance * max_distance;
	float distance_factor = max(DISTANCE_FACTOR_MIN, (square_max_distance - square_distance) / square_max_distance);
	float point_size = max(POINT_SIZE_MIN, POINT_SIZE_SCALE * distance_factor * angle_factor);

	float light_level = free_spotlight_in_range(square_distance, max_distance) ? angle_factor : 0.0;
	if (maszyna_overcast > PRECIPITATION_DIMMING_OVERCAST) {
		float precipitation = mix(
				mix(1.0, PRECIPITATION_DIMMING_MIN, clamp(maszyna_overcast * PRECIPITATION_DIMMING_SLOPE - PRECIPITATION_DIMMING_OFFSET, 0.0, 1.0)), 1.0, distance_factor);
		light_level *= precipitation;
	}
	v_light_level = min(1.0, light_level);

	// the original's fFogEnd, of which m_fogrange is the part the overcast leaves
	float fog_end = maszyna_fog_range * max(1.0, maszyna_overcast * 2.0);
	float halo_scale = maszyna_overcast > FOG_HALO_OVERCAST
			? mix(FOG_HALO_SCALE_NEAR, 1.0, clamp(fog_end / FOG_HALO_FOG_END, 0.0, 1.0)) * max(1.0, maszyna_overcast)
			: 0.0;
	float quad_size = point_size * point_size_multiplier * max(1.0, halo_scale);
	v_point_share = 1.0 / max(1.0, halo_scale);
	v_halo_share = halo_scale > 0.0 ? halo_scale / max(1.0, halo_scale) : 0.0;

	// apply_fog.glsl with the lights' own fog density, 1 / (m_fogrange * 2)
	float fog_density = 1.0 / (maszyna_fog_range * LIGHT_FOG_RANGE_SCALE);
	v_fog_visibility = exp(-fog_density * fog_density * square_distance);

	vec4 clip = PROJECTION_MATRIX * center_view;
	// VERTEX is a corner of the unit quad (-1..1); half the quad's size in pixels each way
	clip.xy += VERTEX.xy * quad_size / VIEWPORT_SIZE * clip.w;
	POSITION = clip;
	UV = VERTEX.xy;
}

void fragment() {
	if (v_light_level <= 0.0) {
		discard;
	}
	// freespot.frag: alpha fading to the edge of a disc half the point's radius
	float point_radius = length(UV) / v_point_share;
	float point_alpha = point_radius > DISC_RADIUS ? 0.0 : mix(v_light_level, 0.0, point_radius / DISC_RADIUS);
	float halo_alpha = 0.0;
	if (v_halo_share > 0.0) {
		float halo_radius = length(UV) / v_halo_share;
		halo_alpha = halo_radius > DISC_RADIUS ? 0.0 : mix(v_light_level * FOG_HALO_ALPHA, 0.0, halo_radius / DISC_RADIUS);
	}
	// the point is drawn over its halo
	float alpha = point_alpha + halo_alpha * (1.0 - point_alpha);
	if (alpha <= 0.0) {
		discard;
	}
	ALBEDO = light_color;
	// the original mixes the colour towards the fog's; over the fogged scene behind it fading the
	// point out is the same
	ALPHA = alpha * v_fog_visibility;
}
