// Head torch near-field glow: Godot's spot light barely lights the cone right in front of the
// projector, so this full-screen pass brightens what lies inside the torch cone, reconstructed
// from the depth buffer - the cone and the distance falloff follow the Headlamp spot light.
// The pass only adds its light: the screen texture is copied before translucent geometry is drawn,
// so a pass writing it back over the whole screen erased every translucent emission (signal lights,
// cab displays) whenever the torch was on.
shader_type spatial;
render_mode unshaded, blend_add, depth_test_disabled, depth_draw_never, cull_disabled, fog_disabled, shadows_disabled;

uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform sampler2D depth_texture : hint_depth_texture, filter_nearest;
// view space normals (Forward+) - the glow is shaded by the angle of incidence like real light
uniform sampler2D normal_roughness_texture : hint_normal_roughness_texture, filter_nearest;

// torch position and axis in view (camera) space, set by MaszynaPlayer from the Headlamp node
uniform vec3 light_position = vec3(0.0, 0.16, -0.05);
uniform vec3 light_direction = vec3(0.0, -0.26, -0.97);
uniform vec3 light_color : source_color = vec3(1.0, 0.965, 0.93);
// cone edge: full brightness inside the inner angle, none outside the outer one (degrees)
uniform float cone_inner_angle = 8.0;
uniform float cone_outer_angle = 30.0;
// distance where the glow fades out completely (m)
uniform float glow_range = 6.0;
// inverse square falloff (1 at 1 m) is softened below this distance, so it stays finite up close (m)
uniform float near_distance = 0.3;
// multiplies what is already lit - the full value in a dark frame, less the brighter the frame is
uniform float boost = 1.5;
// average frame luminance at which the boost is halved - like the eye adapting to the light around
uniform float adaptation_luminance = 0.006;
// overall glow strength, scales both the boost and the lift
uniform float light_energy = 1.0;
// small light added on top, lets unlit (black) surfaces show up - keep it low, a large one flattens
// the image into a milky haze (the real albedo is not available in screen space)
uniform float lift = 0.01;

void vertex() {
	// full-screen quad (QuadMesh 2x2) at the reverse-z near plane
	POSITION = vec4(VERTEX.xy, 1.0, 1.0);
}

void fragment() {
	float depth = texture(depth_texture, SCREEN_UV).r;
	vec4 view = INV_PROJECTION_MATRIX * vec4(SCREEN_UV * 2.0 - 1.0, depth, 1.0);
	vec3 surface = view.xyz / view.w;

	vec3 to_surface = surface - light_position;
	float distance = length(to_surface);
	float cone = smoothstep(
			cos(radians(cone_outer_angle)), cos(radians(cone_inner_angle)),
			dot(to_surface / max(distance, 0.0001), normalize(light_direction)));
	// inverse square like a real torch, normalized to 1 at 1 m, faded out smoothly towards the range
	float near_squared = near_distance * near_distance;
	float falloff = (1.0 + near_squared) / (distance * distance + near_squared)
			* pow(clamp(1.0 - distance / glow_range, 0.0, 1.0), 2.0);
	vec3 normal = normalize(texture(normal_roughness_texture, SCREEN_UV).xyz * 2.0 - 1.0);
	float incidence = max(dot(normal, -to_surface / max(distance, 0.0001)), 0.0);
	float glow = light_energy * cone * falloff * incidence;

	// the smallest mip level is the average of the whole frame
	vec3 average = textureLod(screen_texture, vec2(0.5), 16.0).rgb;
	float adapted_boost = boost / (1.0 + dot(average, vec3(0.2126, 0.7152, 0.0722)) / adaptation_luminance);

	vec3 screen = texture(screen_texture, SCREEN_UV).rgb;
	ALBEDO = screen * adapted_boost * glow * light_color + lift * glow * light_color;
}
