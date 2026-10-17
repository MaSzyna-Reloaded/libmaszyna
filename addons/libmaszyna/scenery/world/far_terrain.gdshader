// A world's far terrain (MaszynaWorld.FAR_TERRAIN_DIR): its block's maps over the coarse mesh - the
// ground's colour by its textures and the ground's normal of the 1 m heights (the world's X, Y up, Z),
// lit, matte
shader_type spatial;
render_mode cull_disabled, diffuse_burley, specular_disabled;

uniform sampler2D albedo_map : source_color, filter_linear_mipmap_anisotropic, repeat_disable;
uniform sampler2D normal_map : filter_linear_mipmap_anisotropic, repeat_disable;

void fragment() {
	vec3 ground = normalize(texture(normal_map, UV).xyz * 2.0 - 1.0);
	NORMAL = normalize((VIEW_MATRIX * vec4(ground, 0.0)).xyz);
	ALBEDO = texture(albedo_map, UV).rgb;
	ROUGHNESS = 1.0;
}
