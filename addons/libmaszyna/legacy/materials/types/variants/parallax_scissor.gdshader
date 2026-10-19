// types/parallax.gdshader cut out
shader_type spatial;
render_mode cull_back, depth_draw_opaque, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_ALPHA_SCISSOR
#include "res://addons/libmaszyna/legacy/materials/types/parallax.gdshaderinc"
