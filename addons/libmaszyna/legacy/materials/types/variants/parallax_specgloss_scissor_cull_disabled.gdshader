// types/parallax_specgloss.gdshader cut out, both faces drawn
shader_type spatial;
render_mode cull_disabled, depth_draw_opaque, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_ALPHA_SCISSOR
#include "res://addons/libmaszyna/legacy/materials/types/parallax_specgloss.gdshaderinc"
