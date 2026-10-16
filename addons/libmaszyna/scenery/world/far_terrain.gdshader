// A world's far terrain (MaszynaWorld.FAR_TERRAIN_DIR): the ground's colour of each vertex, the mean
// of its textures - sRGB, as the textures' pixels it was taken from - lit, matte
shader_type spatial;
render_mode cull_disabled, diffuse_burley, specular_disabled;

void fragment() {
	vec3 srgb = COLOR.rgb;
	// sRGB to linear, IEC 61966-2-1
	ALBEDO = mix(pow((srgb + vec3(0.055)) / 1.055, vec3(2.4)), srgb / 12.92, lessThan(srgb, vec3(0.04045)));
	ROUGHNESS = 1.0;
}
