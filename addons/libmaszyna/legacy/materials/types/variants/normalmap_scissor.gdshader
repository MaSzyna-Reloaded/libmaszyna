// types/normalmap.gdshader cut out
shader_type spatial;
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_disabled;

#define MASZYNA_ALPHA_SCISSOR
#include "res://addons/libmaszyna/legacy/materials/types/normalmap.gdshaderinc"
