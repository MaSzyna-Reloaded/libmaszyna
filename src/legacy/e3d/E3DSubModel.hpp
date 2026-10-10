#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    class E3DSubModel : public Resource {
            GDCLASS(E3DSubModel, Resource)
        public:
            ~E3DSubModel() override;

        protected:
            static void _bind_methods();

        public:
            enum SubModelType {
                SUBMODEL_GL_POINTS,
                SUBMODEL_GL_LINES,
                SUBMODEL_GL_LINE_STRIP,
                SUBMODEL_GL_LINE_LOOP,
                SUBMODEL_GL_TRIANGLES,
                SUBMODEL_GL_TRIANGLE_STRIP,
                SUBMODEL_GL_TRIANGLE_FAN,
                SUBMODEL_GL_QUADS,
                SUBMODEL_GL_QUAD_STRIP,
                SUBMODEL_GL_POLYGON,
                SUBMODEL_TRANSFORM = 256,
                SUBMODEL_FREE_SPOTLIGHT,
                SUBMODEL_STARS
            };

            enum AnimationType {
                ANIMATION_NONE = 0,
                ANIMATION_ROTATE_VEC,
                ANIMATION_ROTATE_XYZ,
                ANIMATION_MOVE,
                ANIMATION_JUMP_SECONDS,
                ANIMATION_JUMP_MINUTES,
                ANIMATION_JUMP_HOURS,
                ANIMATION_JUMP_HOURS24,
                ANIMATION_SECONDS,
                ANIMATION_MINUTES,
                ANIMATION_HOURS,
                ANIMATION_HOURS24,
                ANIMATION_BILLBOARD,
                ANIMATION_WIND,
                ANIMATION_SKY,
                ANIMATION_DIGITAL,
                ANIMATION_DIGICLK,
                ANIMATION_UNDEFINED,
                ANIMATION_IK = 256,
                ANIMATION_IK1,
                ANIMATION_IK2,
                ANIMATION_UNKNOWN = -1
            };

            MAKE_MEMBER_GS_NR(SubModelType, submodel_type, SUBMODEL_GL_TRIANGLES)
            MAKE_MEMBER_GS_NR(AnimationType, animation, ANIMATION_NONE)
            MAKE_MEMBER_GS_NR(float, lights_on_threshold, 0.0)
            MAKE_MEMBER_GS_NR(float, visibility_light, 0.0)
            MAKE_MEMBER_GS_NR(float, visibility_range_begin, 0.0)
            MAKE_MEMBER_GS_NR(float, visibility_range_end, 0.0)
            MAKE_MEMBER_GS_NR(Color, diffuse_color, Color(1.0, 1.0, 1.0, 1.0))
            MAKE_MEMBER_GS_NR(Color, self_illumination, Color(1.0, 1.0, 1.0, 1.0))
            MAKE_MEMBER_GS_NR(bool, material_colored, false)
            MAKE_MEMBER_GS_NR(bool, dynamic_material, false)
            MAKE_MEMBER_GS_NR(int, dynamic_material_index, 0)
            MAKE_MEMBER_GS_NR(bool, material_transparent, false)
            /* A replaceable skin drawn in the translucent phase - its own bit, 1 << skin index,
             * instead of 0x20 (Model3d.cpp:421-441); the skin's texture decides how much of it is */
            MAKE_MEMBER_GS_NR(bool, skin_translucent, false)
            MAKE_MEMBER_GS_NR(String, material_name, "")
            MAKE_MEMBER_GS_NR(Transform3D, transform, Transform3D())
            MAKE_MEMBER_GS_NR(Transform3D, material_transform, Transform3D())
            MAKE_MEMBER_GS_NR_NO_DEF(Ref<ArrayMesh>, mesh)
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<E3DSubModel>, submodels)
            MAKE_MEMBER_GS_NR(bool, visible, true)
            MAKE_MEMBER_GS_NR(bool, skip_rendering, false)
            /// A "<name>_on"/"<name>_xon" submodel: a control in its lit state, hidden by default
            /// only in a dynamic (vehicle) model (Model3d.cpp:275, 2221)
            bool dynamic_hidden = false;
            void set_dynamic_hidden(bool p_dynamic_hidden);
            bool get_dynamic_hidden() const;

            MAKE_MEMBER_GS_NR(float, light_range, 0.0)
            MAKE_MEMBER_GS_NR(float, light_attenuation, 1.0)
            MAKE_MEMBER_GS_NR(float, light_angle, 45.0)
            MAKE_MEMBER_GS_NR(float, near_attenuation_start, 0.0)
            MAKE_MEMBER_GS_NR(float, near_attenuation_end, 0.0)
            MAKE_MEMBER_GS_NR(bool, use_near_attenuation, false)
            MAKE_MEMBER_GS_NR(int, far_attenuation_decay, 0)
            MAKE_MEMBER_GS_NR(float, cos_hotspot_angle, 0.0)
            MAKE_MEMBER_GS_NR(float, cos_view_angle, 0.0)
            MAKE_MEMBER_GS_NR(float, light_energy, 0.0)

            void add_child(const Ref<E3DSubModel> &p_sub_model);
            void clear();
    };
} // namespace godot

VARIANT_ENUM_CAST(E3DSubModel::AnimationType)
VARIANT_ENUM_CAST(E3DSubModel::SubModelType)
