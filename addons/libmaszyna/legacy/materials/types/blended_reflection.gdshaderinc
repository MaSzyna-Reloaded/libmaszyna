// A blended surface of a type without a specular of its own (default, normalmap, detail_normalmap):
// it mirrors the sky as the original's does - the environment's reflection weighted by the Fresnel
// term and the material's reflectivity (light_common.glsl, apply_lights(): env_reflection) - and
// takes no highlight from the lights: in a pane as smooth as glass the sun's burns out the screen.

// the material's "reflection:", which the normal map's alpha scales (mat_normalmap.frag:49)
uniform float blended_reflection = 0.0;
// a dielectric's reflectance at normal incidence, the original's F0 of 0.04 (light_common.glsl) -
// Godot's SPECULAR of 0.5
const float BLENDED_SPECULAR = 0.5;
// a pane of glass is smooth: the sky is mirrored in it sharp
const float BLENDED_ROUGHNESS = 0.05;

float blended_specular(float normal_map_alpha) {
	return BLENDED_SPECULAR * clamp(blended_reflection * normal_map_alpha, 0.0, 1.0);
}

// the lights' diffuse part alone (Lambert, as Godot's own light() example)
void light() {
	DIFFUSE_LIGHT += clamp(dot(NORMAL, LIGHT), 0.0, 1.0) * ATTENUATION * LIGHT_COLOR / PI;
}
