// The shadowlessnormalmap material type with its specgloss texture bound: a variant in a file of
// its own (MaszynaMaterialFactory.SHADER_VARIANTS), so that the export's shader baker bakes it
shader_type spatial;
#define MASZYNA_SPECGLOSS
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libmaszyna/materials/types/shadowlessnormalmap.gdshaderinc"
