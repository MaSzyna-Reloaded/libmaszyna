// NOTE: Shader automatically converted from Godot Engine 4.6.2.stable's StandardMaterial3D.

shader_type spatial;
// receives shadows, unlike the original (mat_shadowlessnormalmap.frag passes shadowtone 1.0) -
// operator's decision: unshadowed grass glows inside the shadow of a building
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_disabled;

#ifdef MASZYNA_SPECGLOSS
// absolute: the variants MaszynaMaterialFactory builds from this code have no path of their own
#include "res://addons/libmaszyna/legacy/materials/types/specgloss.gdshaderinc"
#endif

uniform vec4 albedo : source_color;
uniform sampler2D texture_albedo : source_color, filter_linear_mipmap, repeat_enable;
uniform int transparency = 0;
uniform float alpha_scissor_threshold : hint_range(0.0, 1.0) = 0.5;
uniform ivec2 albedo_texture_size;
uniform float point_size : hint_range(0.1, 128.0, 0.1);

uniform bool emission_enabled = false;
uniform vec4 emission_color : source_color = vec4(1.0);
uniform float emission_energy : hint_range(0.0, 16.0);


uniform vec3 uv1_scale;
uniform vec3 uv1_offset;
uniform vec3 uv2_scale;
uniform vec3 uv2_offset;

void vertex() {
	UV = UV * uv1_scale.xy + uv1_offset.xy;
}

void fragment() {
	vec2 base_uv = UV;

	vec4 albedo_tex = texture(texture_albedo, base_uv);
#ifdef MASZYNA_ALPHA_BLEND
	ALPHA = albedo_tex.a;
#endif
	if (transparency == 2 && albedo_tex.a < alpha_scissor_threshold) {
		discard;
	}
	ALBEDO = albedo.rgb * albedo_tex.rgb;

	// vegetation (grass, trees, crops): no specular, not even the sky reflection's grazing Fresnel -
	// operator's decision; the original lights it with texture luminance (mat_shadowlessnormalmap.frag)
	SPECULAR = 0.0;
	METALLIC = 0.0;
	ROUGHNESS = 1.0;

    if(emission_enabled) {
        EMISSION = emission_color.rgb * albedo.rgb * albedo_tex.rgb * emission_energy;
    }

#ifdef MASZYNA_SPECGLOSS
	// reflection mask: the original takes the normal map's alpha, which this port does not bind
	float specgloss_specular;
	float specgloss_metallic;
	float specgloss_roughness;
	evaluate_specgloss(
			texture(specgloss_texture, base_uv), 1.0, specgloss_specular, specgloss_metallic, specgloss_roughness);
	SPECULAR = specgloss_specular;
	METALLIC = specgloss_metallic;
	ROUGHNESS = specgloss_roughness;
#endif
}
