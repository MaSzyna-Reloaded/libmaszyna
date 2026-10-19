// types/default.gdshader cut out, both faces drawn
shader_type spatial;
render_mode depth_draw_opaque, cull_disabled, diffuse_burley, specular_disabled;

#define MASZYNA_ALPHA_SCISSOR
#include "res://addons/libmaszyna/legacy/materials/types/default.gdshaderinc"
