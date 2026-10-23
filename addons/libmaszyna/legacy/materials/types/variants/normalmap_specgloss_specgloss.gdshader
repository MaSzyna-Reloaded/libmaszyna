// types/normalmap_specgloss.gdshader with the specgloss texture
shader_type spatial;
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_SPECGLOSS
#include "res://addons/libmaszyna/legacy/materials/types/normalmap_specgloss.gdshaderinc"
