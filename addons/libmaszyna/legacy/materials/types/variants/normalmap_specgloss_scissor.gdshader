// types/normalmap_specgloss.gdshader cut out
shader_type spatial;
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_ALPHA_SCISSOR
#include "res://addons/libmaszyna/legacy/materials/types/normalmap_specgloss.gdshaderinc"
