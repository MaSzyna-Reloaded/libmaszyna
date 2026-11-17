// A model drawn a little farther from the camera than it lies (draw_behind_depth [m], set by the
// material factory on a low-poly interior's materials): a vehicle's low-poly interior, part of which
// lies in the body's own plane - the original draws it first and the body over it (GL_LEQUAL,
// opengl33renderer.cpp:3179-3192), so where they meet the body is seen, not the two in turn. Moved
// away from the player's camera (MAIN_CAM_INV_VIEW_MATRIX), never CAMERA_POSITION_WORLD: a double
// precision build keeps the camera's origin negated there (inv_view_matrix[3], split for precision -
// Godot's render_scene_data_rd.cpp), so the interior was moved towards the camera from some places
// and came out through the body, and in a shadow pass it is the shadow cascade's camera besides

uniform float draw_behind_depth = 0.0;

// vertex moved away from camera_position by depth [m], in the model's own space
vec3 maszyna_draw_behind(vec3 vertex, mat4 model_matrix, vec3 camera_position, float depth) {
	if (depth <= 0.0) {
		return vertex;
	}
	vec3 away = normalize((model_matrix * vec4(vertex, 1.0)).xyz - camera_position);
	return vertex + inverse(mat3(model_matrix)) * away * depth;
}
