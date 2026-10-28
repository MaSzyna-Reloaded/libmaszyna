// The normalmap_specgloss material type alpha-blended (MaterialOptions.force_transparent), two-
// sided (MaterialOptions.cull_disabled): a variant in a file of its own
// (MaszynaMaterialFactory.SHADER_VARIANTS), so that the export's shader baker bakes it
shader_type spatial;
#define MASZYNA_ALPHA_BLEND
render_mode depth_draw_opaque, cull_disabled, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libmaszyna/materials/types/normalmap_specgloss.gdshaderinc"
