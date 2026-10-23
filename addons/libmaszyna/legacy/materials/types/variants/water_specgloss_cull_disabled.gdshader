// types/water_specgloss.gdshader both faces drawn
shader_type spatial;
render_mode blend_mix, depth_prepass_alpha, cull_disabled, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libmaszyna/legacy/materials/types/water_specgloss.gdshaderinc"
