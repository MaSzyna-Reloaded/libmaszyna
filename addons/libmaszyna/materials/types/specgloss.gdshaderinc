// The specgloss texture of the original's *_specgloss shaders (mat_normalmap_specgloss.frag:53-56):
// red is the specularity, scaled by param_specular (light_common.glsl:208); green scales
// param_glossiness, the Blinn-Phong exponent; blue is the metalness. The reflectivity is
// param_reflection times a mask each shader picks (the normal map's alpha, the reflmap's alpha).

uniform sampler2D specgloss_texture : filter_linear_mipmap_anisotropic, repeat_enable;
uniform float specular_strength : hint_range(0.0, 1.0) = 1.0;
uniform float reflection_strength : hint_range(0.0, 1.0) = 0.0;
uniform float glossiness = 10.0;

#include "res://addons/libmaszyna/materials/types/glossiness.gdshaderinc"

void evaluate_specgloss(vec4 specgloss_sample, float reflection_mask, out float specular, out float metallic, out float roughness) {
    float source_specularity = specgloss_sample.r * specular_strength;
    float source_reflection = reflection_strength * reflection_mask;
    specular = clamp(max(source_specularity, source_reflection), 0.0, 1.0);
    metallic = clamp(specgloss_sample.b, 0.0, 1.0);
    roughness = glossiness_to_roughness(specgloss_sample.g * abs(glossiness));
}
