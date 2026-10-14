#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    class Camera3D;
    class MeshInstance3D;

    /// Cab controls operated by mouse: the control under the cursor is outlined and captioned, a
    /// left click presses it, and dragging a lever moves it along its own movement, step by step or continuously - the
    /// original's drivermouse_input (drivermouseinput.cpp) and its tooltip (drivermode.cpp:352).
    ///
    /// A control is its mesh and the operations its owner hands in; the system knows nothing of
    /// what they do. Picking casts the cursor ray against the triangles of each control mesh, in
    /// the mesh's space - so a rotating lever is always hit where it is drawn - and the nearest
    /// hit wins unless an occluder (the cab's own geometry: the desk, the walls) is nearer, as the
    /// original's pick buffer lets the desk hide a shaft running under it. It runs on mouse
    /// motion only, over the one cab that exists (a cabin is instanced on entering it).
    ///
    /// Alternative, if triangle tests ever cost too much or alpha-tested geometry must be honoured:
    /// the original's own way - render the cab into a small pick buffer, every submodel in its own
    /// flat colour, and read the one pixel under the cursor (opengl33renderer.cpp:1188-1214,
    /// 3494-3507, Update_Pick_Control :4568). Exact to the pixel, at the price of a second render
    /// of the cab (a SubViewport with a pick shader) on every pick.
    class CabinHUDMouseSystem : public Object {
            GDCLASS(CabinHUDMouseSystem, Object)

        public:
            static const char *control_hovered_signal;
            static const char *control_unhovered_signal;
            static const char *control_state_changed_signal;

            /// Mouse travel, in pixels along the control's movement, that moves a stepped control by
            /// one step
            static constexpr int DRAG_STEP_PIXELS = 28;

            static CabinHUDMouseSystem *get_instance() {
                return Object::cast_to<CabinHUDMouseSystem>(
                        Engine::get_singleton()->get_singleton("CabinHUDMouseSystem"));
            }

        private:
            /// A mesh the cursor ray can hit: its triangles, taken once, in its own space
            struct Part {
                    ObjectID mesh;
                    PackedVector3Array faces;
            };

            /// What the cursor ray can hit of one control or occluder, its own mesh first. A control
            /// is its mesh and every mesh under it (EP07's and SM42's brake valves: the handle
            /// raczkaKranu under zasadniczy), but a control with another control under it is a
            /// panel and its own mesh only (E186's universal1 with op1/op2, and op12 under it that
            /// is nobody's - the original picks a control's exact submodel, Train.cpp:64)
            struct Pickable {
                    Vector<Part> parts;
            };

            struct Control {
                    Pickable pickable;
                    String caption;
                    String hints;
                    /// What the control shows now, as its owner words it
                    String state;
                    Callable pressed;
                    Callable released;
                    Callable increase;
                    Callable decrease;
                    /// Called with the travel of a drag, in pixels along the control's movement,
                    /// in place of the steps - for a control that moves continuously
                    Callable drag;
                    /// One increase of the control: the mesh turned by `step_rotation` in its own
                    /// space and shifted by `step_offset` in its parent's
                    Basis step_rotation;
                    Vector3 step_offset;
                    /// Outlined heavier and tinted - see SMALL_CONTROL_SIZE
                    bool small = false;
                    /// Of a control that turns: the point of it farthest from the axis, where the
                    /// hand holds it - on its handle (the control's own mesh's space)
                    Vector3 grip;
                    /// Fixed signs of an increase per mouse axis (x: right, y: down), in place of
                    /// the ones the grip gives; zero to follow the grip
                    Vector2 drag_signs;
            };

            HashMap<RID, Control> controls;
            HashMap<RID, Pickable> occluders;
            ObjectID camera;
            Ref<StandardMaterial3D> small_outline_material;
            Ref<StandardMaterial3D> large_outline_material;

            RID hovered;
            /// Where the cursor ray met the hovered control, in world space
            Vector3 hovered_point;
            /// The control the left button went down on, until it goes up
            RID held;
            bool dragging = false;
            double drag_travel = 0.0;
            /// The one mouse axis a drag works along, chosen by its first movement and kept until
            /// the button is let go
            enum DragAxis {
                DRAG_AXIS_NONE,
                DRAG_AXIS_HORIZONTAL,
                DRAG_AXIS_VERTICAL,
            };
            DragAxis drag_axis = DRAG_AXIS_NONE;
            /// The movement since the button went down, until it is enough to choose the axis
            Vector2 drag_gesture;
            Vector2 drag_cursor_position;
            /// The sign of an increase on each mouse axis (x: right, y: down; 0 - the axis does
            /// nothing) - the drag follows the control as the hand would
            Vector2 drag_signs;

            static Pickable _pickable(const PackedInt64Array &p_mesh_instance_ids);
            /// Of a control that turns by `p_step_rotation`: the point of it farthest from the axis,
            /// in its own mesh's space; zero for one that does not turn
            static Vector3 _grip(const Pickable &p_pickable, const Basis &p_step_rotation);
            /// The nearest hit of the segment on the pickable closer than `r_distance`: updates
            /// `r_distance` and `r_point` (world space) and returns true
            static bool
            _hit(const Pickable &p_pickable, const Vector3 &p_from, const Vector3 &p_to, double &p_r_distance,
                 Vector3 &p_r_point);
            void _set_outline(const RID &p_control, bool p_outlined);
            /// The control under the cursor, or failing that the one whose middle lies within
            /// PICK_TOLERANCE_PIXELS of it and is not hidden there
            RID _pick(const Vector2 &p_position, Vector3 &p_r_point) const;
            /// The control the ray through `p_position` meets first, unless an occluder is nearer
            RID _pick_exact(const Camera3D *p_view, const Vector2 &p_position, Vector3 &p_r_point) const;
            Vector2 _increase_signs(const Control &p_control) const;
            void _set_hovered(const RID &p_control);
            void _end_hold();

        protected:
            static void _bind_methods();

        public:
            CabinHUDMouseSystem();

            void mouse_set_camera(uint64_t p_camera_id);

            /// A valid `p_increase` makes the control draggable in steps, a valid `p_drag`
            /// continuously; `p_step_rotation` (the mesh's own space) and `p_step_offset` (its
            /// parent's) say how an increase moves it; `p_drag_signs`, when not zero, fixes which
            /// way an increase goes on each mouse axis; `p_caption` empty shows no caption
            RID control_create(
                    uint64_t p_mesh_instance_id, const String &p_caption, const String &p_hints,
                    const Callable &p_pressed, const Callable &p_released, const Callable &p_increase,
                    const Callable &p_decrease, const Basis &p_step_rotation, const Vector3 &p_step_offset,
                    const Callable &p_drag, const Vector2 &p_drag_signs);
            void control_free(const RID &p_control);
            /// What the control shows now (a position, a level), shown under its caption and
            /// updated while it is hovered
            void control_set_state(const RID &p_control, const String &p_state);

            /// A mesh that hides the controls behind it and is not operated itself
            RID occluder_create(uint64_t p_mesh_instance_id);
            void occluder_free(const RID &p_occluder);

            /// Feeds one input event; true when the event operated a control and is consumed
            bool mouse_input(const Ref<InputEvent> &p_event);

            RID control_get_hovered() const;
    };
} // namespace godot
