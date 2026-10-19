// types/shadowlessnormalmap.gdshader blended, both faces drawn
shader_type spatial;
render_mode depth_draw_opaque, cull_disabled, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_ALPHA_BLEND
#include "res://addons/libmaszyna/legacy/materials/types/shadowlessnormalmap.gdshaderinc"
