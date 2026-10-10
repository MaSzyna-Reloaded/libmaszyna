// normalmap.gdshader + a tiled detail normal map, ported from the original mat_detail_normalmap.frag.

shader_type spatial;
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_disabled;

#ifdef MASZYNA_SPECGLOSS
// absolute: the variants MaszynaMaterialFactory builds from this code have no path of their own
#include "res://addons/libmaszyna/legacy/materials/types/specgloss.gdshaderinc"
#endif

#ifdef MASZYNA_ALPHA_BLEND
#ifndef MASZYNA_SPECGLOSS
// absolute: the variants MaszynaMaterialFactory builds from this code have no path of their own
#include "res://addons/libmaszyna/legacy/materials/types/blended_reflection.gdshaderinc"
#endif
#endif

uniform vec4 albedo : source_color;
uniform sampler2D texture_albedo : source_color, filter_linear_mipmap, repeat_enable;
uniform int transparency = 0;
uniform float alpha_scissor_threshold : hint_range(0.0, 1.0) = 0.5;
uniform ivec2 albedo_texture_size;
uniform float point_size : hint_range(0.1, 128.0, 0.1);

uniform float roughness : hint_range(0.0, 1.0);
uniform sampler2D texture_metallic : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform vec4 metallic_texture_channel;
uniform sampler2D texture_roughness : hint_roughness_r, filter_linear_mipmap, repeat_enable;

uniform float specular : hint_range(0.0, 1.0, 0.01);
uniform float metallic : hint_range(0.0, 1.0, 0.01);

uniform sampler2D texture_normal : hint_roughness_normal, filter_linear_mipmap, repeat_enable;
uniform float normal_scale : hint_range(-16.0, 16.0);

uniform sampler2D texture_detail_normal : hint_normal, filter_linear_mipmap, repeat_enable;
uniform float detail_scale = 1.0;
uniform float detail_height_scale = 1.0;

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

	float metallic_tex = dot(texture(texture_metallic, base_uv), metallic_texture_channel);
	METALLIC = metallic_tex * metallic;
	SPECULAR = specular;

	vec4 roughness_texture_channel = vec4(1.0, 0.0, 0.0, 0.0);
	float roughness_tex = dot(texture(texture_roughness, base_uv), roughness_texture_channel);
	ROUGHNESS = roughness_tex * roughness;

	// Normal Map: Enabled
	// mat_detail_normalmap.frag: normalize(vec3(normal.xy + detail.xy * detail_height_scale, normal.z)),
	// with the detail map sampled at f_coord * detail_scale.
	vec2 normal_xy = texture(texture_normal, base_uv).rg * 2.0 - 1.0;
	float normal_z = sqrt(1.0 - clamp(dot(normal_xy, normal_xy), 0.0, 1.0));
	vec2 detail_xy = (texture(texture_detail_normal, base_uv * detail_scale).rg * 2.0 - 1.0) * detail_height_scale;
	// normal_scale is this wrapper's strength of the base normal map and has no counterpart in the
	// original, so it must not reach the detail map: tiled over hundreds of metres (grass.mat:
	// detail_scale 0.00125) and amplified, it tilts flat ground far enough to leave it unlit.
	vec3 combined_normal = normalize(vec3(normal_xy * normal_scale + detail_xy, normal_z));
	NORMAL_MAP = vec3(combined_normal.xy * 0.5 + 0.5, 1.0);
	NORMAL_MAP_DEPTH = 1.0;

    if(emission_enabled) {
        EMISSION = emission_color.rgb * albedo.rgb * albedo_tex.rgb * emission_energy;
    }

#ifdef MASZYNA_SPECGLOSS
	// reflection mask: the normal map's alpha; the original blends by reflblend (mat_detail_normalmap_specgloss.frag:68)
	float specgloss_specular;
	float specgloss_metallic;
	float specgloss_roughness;
	evaluate_specgloss(
			texture(specgloss_texture, base_uv), texture(texture_normal, base_uv).a, specgloss_specular, specgloss_metallic, specgloss_roughness);
	SPECULAR = specgloss_specular;
	METALLIC = specgloss_metallic;
	ROUGHNESS = specgloss_roughness;
#endif

#ifdef MASZYNA_ALPHA_BLEND
#ifndef MASZYNA_SPECGLOSS
	SPECULAR = blended_specular(texture(texture_normal, base_uv).a);
	ROUGHNESS = BLENDED_ROUGHNESS;
#endif
#endif
}
