#include "PlanarMirror3D.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    namespace {
        /* The reflection is rendered from the eye mirrored across the glass, framing the glass
         * alone; a point of the glass shows what that camera sees through it. The reflection is lit
         * and fogged already, and linear - the scene's tonemapping is applied to it once, here. */
        const char *const MIRROR_SHADER = R"(
shader_type spatial;
render_mode unshaded, cull_disabled, shadows_disabled, fog_disabled;

// a silvered glass reflects about 90% of the light
const float REFLECTANCE = 0.9;
uniform sampler2D reflection : filter_linear, repeat_disable;
uniform mat4 mirror_view_projection;
// seen from behind, the glass keeps its own look
uniform bool reflecting = false;
varying vec3 world_position;

void vertex() {
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	if (!reflecting) {
		discard;
	}
	vec4 clip = mirror_view_projection * vec4(world_position, 1.0);
	vec2 ndc = clip.xy / clip.w;
	// normalized device coordinates run up, a texture down
	vec3 reflected = texture(reflection, vec2(ndc.x, -ndc.y) * 0.5 + 0.5).rgb;
	// a value that is not a colour must not reach the scene's glow
	if (any(isnan(reflected)) || any(isinf(reflected))) {
		reflected = vec3(0.0);
	}
	ALBEDO = reflected * REFLECTANCE;
}
)";
        // the mirror's meshes may drop to coarser LODs this many pixels sooner than the scene's
        const real_t MESH_LOD_THRESHOLD = 2.0;
        // past this the view is too steep for world up to orient the camera
        const real_t MAX_UP_ALIGNMENT = 0.99;
        // the field of view Camera3D accepts [deg] (Camera3D::set_fov(), camera_3d.cpp:738)
        const double MIN_CAMERA_FOV = 1.0;
        const double MAX_CAMERA_FOV = 179.0;
        // the reflection's size in pixels, whatever the glass's size on the screen
        const int32_t MIN_TEXTURE_SIZE = 256;
        const int32_t MAX_TEXTURE_SIZE = 1024;
    } // namespace

    void PlanarMirror3D::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &PlanarMirror3D::set_max_distance);
        ClassDB::bind_method(D_METHOD("get_max_distance"), &PlanarMirror3D::get_max_distance);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0,1000,0.1,or_greater,suffix:m"),
                "set_max_distance", "get_max_distance");
        ClassDB::bind_method(D_METHOD("set_resolution_scale", "value"), &PlanarMirror3D::set_resolution_scale);
        ClassDB::bind_method(D_METHOD("get_resolution_scale"), &PlanarMirror3D::get_resolution_scale);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "resolution_scale", PROPERTY_HINT_RANGE, "0.05,1,0.05"),
                "set_resolution_scale", "get_resolution_scale");
    }

    void PlanarMirror3D::_notification(const int p_what) {
        // The plane is the glass's faces weighted by their area; the camera and its target are made once.
        if (p_what == NOTIFICATION_ENTER_TREE) {
            glass = Object::cast_to<MeshInstance3D>(get_parent());
            if (glass == nullptr || glass->get_mesh().is_null()) {
                glass = nullptr;
                return;
            }
            const PackedVector3Array faces = glass->get_mesh()->get_faces();
            Vector3 area_normal;
            Vector3 weighted_middle;
            real_t area = 0.0;
            for (int64_t index = 0; index + 2 < faces.size(); index += 3) {
                Vector3 cross = (faces[index + 1] - faces[index]).cross(faces[index + 2] - faces[index]);
                const real_t face_area = cross.length();
                // a glass drawn from both sides has its faces back to back - they count the same way
                if (cross.dot(area_normal) < 0.0) {
                    cross = -cross;
                }
                area_normal += cross;
                weighted_middle += (faces[index] + faces[index + 1] + faces[index + 2]) * face_area;
                area += face_area;
            }
            if (area_normal.is_zero_approx() || Math::is_zero_approx(area)) {
                glass = nullptr;
                return;
            }
            plane_normal = area_normal.normalized();
            // the middle of a face is the sum of its corners over three
            plane_point = weighted_middle / (area * 3.0);
            // the glass reflects on the side away from its housing, the mesh it is part of
            if (const MeshInstance3D *housing = Object::cast_to<MeshInstance3D>(glass->get_parent());
                housing != nullptr && housing->get_mesh().is_valid()) {
                const Vector3 housing_middle =
                        glass->get_transform().affine_inverse().xform(housing->get_mesh()->get_aabb().get_center());
                if (plane_normal.dot(housing_middle - plane_point) > 0.0) {
                    plane_normal = -plane_normal;
                }
                facing_viewer = false;
            }
            glass_radius = 0.0;
            for (const Vector3 &vertex: faces) {
                glass_radius = MAX(glass_radius, vertex.distance_to(plane_point));
            }

            if (viewport == nullptr) {
                viewport = memnew(SubViewport);
                viewport->set_update_mode(SubViewport::UPDATE_DISABLED);
                viewport->set_use_hdr_2d(true);
                viewport->set_positional_shadow_atlas_size(0);
                viewport->set_mesh_lod_threshold(static_cast<float>(MESH_LOD_THRESHOLD));
                viewport->set_msaa_3d(Viewport::MSAA_DISABLED);
                viewport->set_screen_space_aa(Viewport::SCREEN_SPACE_AA_DISABLED);
                viewport->set_use_taa(false);
                add_child(viewport, false, INTERNAL_MODE_BACK);
                camera = memnew(Camera3D);
                viewport->add_child(camera);
                camera->set_current(true);
                Ref<Shader> shader;
                shader.instantiate();
                shader->set_code(MIRROR_SHADER);
                material.instantiate();
                material->set_shader(shader);
                material->set_shader_parameter("reflection", viewport->get_texture());
            }
            // drawn over the glass: its own material belongs to the model it is part of
            glass_material = glass->get_material_overlay();
            glass->set_material_overlay(material);
            set_process_internal(true);
            return;
        }

        if (p_what == NOTIFICATION_EXIT_TREE) {
            set_process_internal(false);
            _set_rendering(false);
            if (glass != nullptr) {
                glass->set_material_overlay(glass_material);
            }
            glass = nullptr;
            glass_material.unref();
            return;
        }

        if (p_what != NOTIFICATION_INTERNAL_PROCESS) {
            return;
        }
        Viewport *seen_in = get_viewport();
        Camera3D *eye = seen_in != nullptr ? seen_in->get_camera_3d() : nullptr;
        if (eye == nullptr || !glass->is_visible_in_tree()) {
            _set_rendering(false);
            return;
        }
        const Transform3D glass_transform = glass->get_global_transform();
        const Vector3 point = glass_transform.xform(plane_point);
        const Vector3 eye_position = eye->get_global_position();
        if (eye_position.distance_to(point) > max_distance || !eye->is_position_in_frustum(point)) {
            _set_rendering(false);
            return;
        }
        Vector3 normal = glass_transform.basis.inverse().transposed().xform(plane_normal).normalized();
        if (normal.dot(eye_position - point) < 0.0) {
            // without a housing to tell, the side the eye looks at reflects
            if (!facing_viewer) {
                _set_rendering(false);
                return;
            }
            normal = -normal;
        }

        const Vector3 glass_scale = glass_transform.basis.get_scale().abs();
        const real_t radius = glass_radius * MAX(glass_scale.x, MAX(glass_scale.y, glass_scale.z));
        // the eye mirrored across the glass looks at the glass, with the glass just filling its view
        const Vector3 mirrored_origin = eye_position - normal * (2.0 * normal.dot(eye_position - point));
        const Vector3 view = point - mirrored_origin;
        const real_t glass_distance = view.length();
        const Vector3 up =
                Math::abs(view.normalized().y) > MAX_UP_ALIGNMENT ? Vector3(0.0, 0.0, 1.0) : Vector3(0.0, 1.0, 0.0);
        // the angle the glass fills seen from the mirrored eye: a glass a speck in the distance or
        // one the eye is at has no view a camera can take
        const double fov = Math::rad_to_deg(2.0 * Math::atan(radius / glass_distance));
        if (fov < MIN_CAMERA_FOV || fov > MAX_CAMERA_FOV) {
            _set_rendering(false);
            return;
        }
        camera->set_global_transform(Transform3D(Basis::looking_at(view, up), mirrored_origin));
        camera->set_keep_aspect_mode(Camera3D::KEEP_HEIGHT);
        camera->set_fov(fov);
        // whatever is nearer than the glass stands behind it
        camera->set_near(MAX(eye->get_near(), glass_distance - radius));
        camera->set_far(eye->get_far());
        camera->set_cull_mask(eye->get_cull_mask());

        // as many pixels as the glass covers on the screen
        const real_t screen_height = seen_in->get_visible_rect().size.y;
        const real_t eye_distance = eye_position.distance_to(point);
        const real_t glass_pixels =
                screen_height * radius / (eye_distance * Math::tan(Math::deg_to_rad(eye->get_fov()) * 0.5));
        // in powers of two: a size that follows every move reallocates the texture every frame, and
        // the glass shows whatever the new one holds before it is rendered into
        const int32_t side =
                CLAMP(static_cast<int32_t>(
                              Math::next_power_of_2(static_cast<uint32_t>(MAX(glass_pixels * resolution_scale, 1.0)))),
                      MIN_TEXTURE_SIZE, MAX_TEXTURE_SIZE);
        if (viewport->get_size() != Vector2i(side, side)) {
            viewport->set_size(Vector2i(side, side));
        }
        material->set_shader_parameter(
                "mirror_view_projection", Projection(camera->get_camera_projection()) *
                                                  Projection(camera->get_global_transform().affine_inverse()));
        if (viewport->get_update_mode() == SubViewport::UPDATE_ALWAYS) {
            return;
        }
        // starts rendering: the scene's environment as it is now, without what the mirror need not pay for
        Ref<Environment> source = eye->get_environment();
        if (source.is_null() && eye->get_world_3d().is_valid()) {
            source = eye->get_world_3d()->get_environment();
        }
        environment = source.is_valid() ? Ref<Environment>(source->duplicate()) : Ref<Environment>();
        if (environment.is_valid()) {
            environment->set_sdfgi_enabled(false);
            environment->set_ssr_enabled(false);
            environment->set_ssao_enabled(false);
            environment->set_ssil_enabled(false);
            environment->set_glow_enabled(false);
            environment->set_volumetric_fog_enabled(false);
            environment->set_adjustment_enabled(false);
            // linear, the glass is tonemapped with the scene
            environment->set_tonemapper(Environment::TONE_MAPPER_LINEAR);
            environment->set_tonemap_exposure(1.0);
            environment->set_tonemap_white(1.0);
        }
        camera->set_environment(environment);
        _set_rendering(true);
    }

    void PlanarMirror3D::_set_rendering(const bool p_rendering) {
        if (viewport != nullptr) {
            viewport->set_update_mode(p_rendering ? SubViewport::UPDATE_ALWAYS : SubViewport::UPDATE_DISABLED);
            material->set_shader_parameter("reflecting", p_rendering);
        }
    }

    void PlanarMirror3D::set_max_distance(const double p_value) {
        max_distance = p_value;
    }

    double PlanarMirror3D::get_max_distance() const {
        return max_distance;
    }

    void PlanarMirror3D::set_resolution_scale(const double p_value) {
        resolution_scale = p_value;
    }

    double PlanarMirror3D::get_resolution_scale() const {
        return resolution_scale;
    }
} // namespace godot
