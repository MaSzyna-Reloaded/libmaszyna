#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /// Scenery models operated by mouse while walking: a left click on a model calls what its owner
    /// handed in, Shift+click the other one - the original's click on a scenery model in free-fly
    /// mode (drivermouseinput.cpp:331-353, basic_cell::on_click(), scene.cpp:33-43 and 691-704),
    /// which is how a hand-thrown switch is set. The model under the cursor is outlined as a cab
    /// control is (CabinHUDMouseSystem); the original shows nothing there. A vehicle's model is
    /// picked the same way, and a click on it is announced with the vehicle's handle.
    ///
    /// A pickable is an E3DRenderingServer instance and the operations its owner hands in, or the
    /// vehicle it draws; the server knows nothing of what they do, and the pickable goes with its
    /// instance. Picking runs on mouse motion only and casts the cursor ray against the registered
    /// instances alone - a handful per scenery and the vehicles near the camera, not every model -
    /// of which only the ones built (streamed in) are tested.
    ///
    /// Unlike the original's pick buffer, nothing hides a model: one standing behind a building is
    /// picked through it.
    class SceneryHUDMouseServer : public Object {
            GDCLASS(SceneryHUDMouseServer, Object)

        public:
            static const char *pickable_hovered_signal;
            static const char *pickable_unhovered_signal;
            static const char *vehicle_pressed_signal;

            static SceneryHUDMouseServer *get_instance() {
                return Object::cast_to<SceneryHUDMouseServer>(
                        Engine::get_singleton()->get_singleton("SceneryHUDMouseServer"));
            }

        private:
            struct Pickable {
                    RID instance;
                    String caption;
                    String hints;
                    Callable pressed;
                    Callable shift_pressed;
                    RID vehicle;
            };

            HashMap<RID, Pickable> pickables;
            ObjectID camera;
            bool active = true;
            Ref<StandardMaterial3D> outline_material;
            Ref<StandardMaterial3D> vehicle_outline_material;
            RID hovered;

            void _set_hovered(const RID &p_pickable);
            void _on_instance_freed(const RID &p_instance);

        protected:
            static void _bind_methods();

        public:
            SceneryHUDMouseServer();

            void mouse_set_camera(uint64_t p_camera_id);
            /// Inactive, nothing is picked and the outline is taken off - the original picks
            /// scenery only in free-fly mode, never from the cab
            void mouse_set_active(bool p_active);

            /// A click on the instance's model calls `p_pressed`, with Shift `p_shift_pressed`.
            /// Several pickables of one instance are all operated by a click on it. `p_caption` and
            /// `p_hints` (the keys that do the same) are what the tooltip shows while it is hovered.
            RID pickable_create(
                    const RID &p_instance, const String &p_caption, const String &p_hints, const Callable &p_pressed,
                    const Callable &p_shift_pressed);
            /// A click on the instance's model, with or without Shift, emits `vehicle_pressed` with
            /// `p_vehicle` - a RailVehicleServer handle; `p_caption` is what the tooltip shows
            RID vehicle_pickable_create(const RID &p_instance, const String &p_caption, const RID &p_vehicle);
            /// Frees the pickable; one whose instance is freed goes with it
            void pickable_free(const RID &p_pickable);

            /// Feeds one input event; true when the event operated a pickable and is consumed
            bool mouse_input(const Ref<InputEvent> &p_event);

            RID pickable_get_hovered() const;
    };
} // namespace godot
