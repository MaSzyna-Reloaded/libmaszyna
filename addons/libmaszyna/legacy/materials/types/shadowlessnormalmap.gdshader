// NOTE: Shader automatically converted from Godot Engine 4.6.2.stable's StandardMaterial3D.

shader_type spatial;
// receives shadows, unlike the original (mat_shadowlessnormalmap.frag passes shadowtone 1.0) -
// operator's decision: unshadowed grass glows inside the shadow of a building
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_disabled;

#include "res://addons/libmaszyna/legacy/materials/types/shadowlessnormalmap.gdshaderinc"
