// types/detail_normalmap.gdshader both faces drawn, with the specgloss texture
shader_type spatial;
render_mode depth_draw_opaque, cull_disabled, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_SPECGLOSS
#include "res://addons/libmaszyna/legacy/materials/types/detail_normalmap.gdshaderinc"
