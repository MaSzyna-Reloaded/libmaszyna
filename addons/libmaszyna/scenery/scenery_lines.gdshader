shader_type spatial;
// The scenery's "lines" nodes (MaszynaLinesGeometry.build_mesh()) drawn as the original draws them
// (opengl33renderer.cpp:4132-4163): a line some pixels wide, as glLineWidth() draws it, fading with
// the distance of its node's centre and seen only within the node's range. A line is a quad whose
// corners are pushed to the line's sides on the screen.
render_mode blend_mix, cull_disabled, depth_draw_opaque, specular_disabled;

// opengl33renderer.cpp:4142-4145: the fade and the width [px] of a node's lines
const float FADE_SCALE = 10.0;
const float FADE_RADIUS_SHARE = 0.5;
const float WIDTH_FADE_SHARE = 0.5;
const float WIDTH_RADIUS_SCALE = 0.001;
const float MIN_WIDTH = 1.0;
const float MAX_WIDTH = 8.0;
// The original's line shader takes its colour as sRGB (traction.frag)
const float GAMMA = 2.2;
// How far ahead of the camera a line going behind it is cut [m]
const float CUT_DEPTH = 0.01;
// A corner outside the screen: the node's quads are not drawn
const vec4 HIDDEN = vec4(2.0, 2.0, 2.0, 1.0);

varying float v_alpha;

void vertex() {
	float width = CUSTOM2.x;
	float radius = CUSTOM1.w;
	float distance = length((MODELVIEW_MATRIX * vec4(CUSTOM1.xyz, 1.0)).xyz);
	if (distance < CUSTOM2.y || (CUSTOM2.z > 0.0 && distance >= CUSTOM2.z)) {
		POSITION = HIDDEN;
	} else {
		// a negative width is a line that never fades
		float line_alpha = width > 0.0
				? FADE_SCALE * width / max(FADE_RADIUS_SHARE * radius + 1.0, distance - FADE_RADIUS_SHARE * radius)
				: 1.0;
		float line_width = clamp(WIDTH_FADE_SHARE * line_alpha + width * radius * WIDTH_RADIUS_SCALE, MIN_WIDTH, MAX_WIDTH);
		v_alpha = min(1.0, line_alpha);

		vec3 end = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
		vec3 other = (MODELVIEW_MATRIX * vec4(CUSTOM0.xyz, 1.0)).xyz;
		// the end behind the camera is brought in front of it, so the line keeps its direction
		if (end.z > -CUT_DEPTH && other.z <= -CUT_DEPTH) {
			end = mix(other, end, (-CUT_DEPTH - other.z) / (end.z - other.z));
		} else if (other.z > -CUT_DEPTH && end.z <= -CUT_DEPTH) {
			other = mix(end, other, (-CUT_DEPTH - end.z) / (other.z - end.z));
		}
		vec4 clip_end = PROJECTION_MATRIX * vec4(end, 1.0);
		vec4 clip_other = PROJECTION_MATRIX * vec4(other, 1.0);
		vec2 along = (clip_other.xy / clip_other.w - clip_end.xy / clip_end.w) * VIEWPORT_SIZE;
		vec2 direction = length(along) > 0.0 ? normalize(along) : vec2(1.0, 0.0);
		// half the width each side: a pixel is 2 / VIEWPORT_SIZE of the clip space
		clip_end.xy += vec2(-direction.y, direction.x) * CUSTOM0.w * line_width / VIEWPORT_SIZE * clip_end.w;
		POSITION = clip_end;
	}
}

void fragment() {
	ALBEDO = pow(COLOR.rgb, vec3(GAMMA));
	ALPHA = v_alpha;
	ROUGHNESS = 1.0;
	METALLIC = 0.0;
	// a line faces the camera
	NORMAL = vec3(0.0, 0.0, 1.0);
}
