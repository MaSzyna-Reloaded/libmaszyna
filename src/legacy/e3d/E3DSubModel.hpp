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

        private:
            SubModelType submodel_type = SUBMODEL_GL_TRIANGLES;
            AnimationType animation = ANIMATION_NONE;
            float lights_on_threshold = 0.0;
            float visibility_light = 0.0;
            float visibility_range_begin = 0.0;
            float visibility_range_end = 0.0;
            Color diffuse_color = Color(1.0, 1.0, 1.0, 1.0);
            Color self_illumination = Color(1.0, 1.0, 1.0, 1.0);
            bool material_colored = false;
            bool dynamic_material = false;
            int dynamic_material_index = 0;
            bool material_transparent = false;
            /* A replaceable skin drawn in the translucent phase - its own bit, 1 << skin index,
             * instead of 0x20 (Model3d.cpp:421-441); the skin's texture decides how much of it is */
            bool skin_translucent = false;
            String material_name = "";
            Transform3D transform = Transform3D();
            Transform3D material_transform = Transform3D();
            /// An E3DMesh when made of the model's data, which keeps its arrays (E3DModelBuilder)
            Ref<Mesh> mesh;
            TypedArray<E3DSubModel> submodels;
            bool visible = true;
            bool skip_rendering = false;

        public:
            MAKE_GS_NR(SubModelType, submodel_type);
            MAKE_GS_NR(AnimationType, animation);
            MAKE_GS_NR(float, lights_on_threshold);
            MAKE_GS_NR(float, visibility_light);
            MAKE_GS_NR(float, visibility_range_begin);
            MAKE_GS_NR(float, visibility_range_end);
            MAKE_GS_NR(Color, diffuse_color);
            MAKE_GS_NR(Color, self_illumination);
            MAKE_GS_NR(bool, material_colored);
            MAKE_GS_NR(bool, dynamic_material);
            MAKE_GS_NR(int, dynamic_material_index);
            MAKE_GS_NR(bool, material_transparent);
            MAKE_GS_NR(bool, skin_translucent);
            MAKE_GS_NR(String, material_name);
            MAKE_GS_NR(Transform3D, transform);
            MAKE_GS_NR(Transform3D, material_transform);
            MAKE_GS_NR(Ref<Mesh>, mesh);
            MAKE_GS_NR(TypedArray<E3DSubModel>, submodels);
            MAKE_GS_NR(bool, visible);
            MAKE_GS_NR(bool, skip_rendering);
            /// A "<name>_on"/"<name>_xon" submodel: a control in its lit state, hidden by default
            /// only in a dynamic (vehicle) model (Model3d.cpp:275, 2221)
            bool dynamic_hidden = false;
            void set_dynamic_hidden(bool p_dynamic_hidden);
            bool get_dynamic_hidden() const;

        private:
            float light_range = 0.0;
            float light_attenuation = 1.0;
            float light_angle = 45.0;
            float near_attenuation_start = 0.0;
            float near_attenuation_end = 0.0;
            bool use_near_attenuation = false;
            int far_attenuation_decay = 0;
            float cos_hotspot_angle = 0.0;
            float cos_view_angle = 0.0;
            float light_energy = 0.0;

        public:
            MAKE_GS_NR(float, light_range);
            MAKE_GS_NR(float, light_attenuation);
            MAKE_GS_NR(float, light_angle);
            MAKE_GS_NR(float, near_attenuation_start);
            MAKE_GS_NR(float, near_attenuation_end);
            MAKE_GS_NR(bool, use_near_attenuation);
            MAKE_GS_NR(int, far_attenuation_decay);
            MAKE_GS_NR(float, cos_hotspot_angle);
            MAKE_GS_NR(float, cos_view_angle);
            MAKE_GS_NR(float, light_energy);

            void add_child(const Ref<E3DSubModel> &p_sub_model);
            void clear();
    };
} // namespace godot

VARIANT_ENUM_CAST(E3DSubModel::AnimationType)
VARIANT_ENUM_CAST(E3DSubModel::SubModelType)
