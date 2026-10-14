#pragma once

#include "tracks/TrackServer.hpp"
#include "vehicles/rail/RailVehicleAppearance.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* A rail vehicle in the scene: the node it is drawn at, and whatever of it rides on that node
     * - a cab, sounds. The vehicle itself is its handle (get_rid()); this node takes it
     * (set_vehicle()) and hands the servers what the scene set: RailVehicleServer places it on its
     * start track, RailVehicleRenderingServer draws and moves it. The node does no work of its
     * own per frame.
     *
     * A vehicle assembled by hand names its parts by path - its VehiclePhysicsNode, its exterior
     * and low-poly models (E3DModelInstance) and the submodels of the exterior that move; one
     * built from data hands the servers its handle and appearance itself, and names nothing. */
    class RailVehicle3D : public Node3D {
            GDCLASS(RailVehicle3D, Node3D)

        public:
            static const char *vehicle_changed_signal;

        private:
            NodePath controller_path;
            Ref<RailVehicleAppearance> appearance;
            NodePath model_instance_path;
            NodePath low_poly_cabin_path;
            NodePath front_bogie_path;
            NodePath rear_bogie_path;
            TypedArray<NodePath> front_rolling_wheel_paths;
            TypedArray<NodePath> powered_wheel_paths;
            TypedArray<NodePath> rear_rolling_wheel_paths;
            TypedArray<NodePath> pantograph_front_arm_paths;
            TypedArray<NodePath> pantograph_rear_arm_paths;
            TypedArray<NodePath> wiper_arm_paths;
            TypedArray<NodePath> mirror_paths;
            String start_track_name;
            double start_track_offset = 0.0;
            TrackServer::Direction start_direction = TrackServer::DIRECTION_NORMAL;
            Ref<Material> head_display_material;

            /* The vehicle this node draws - the handle its VehiclePhysicsNode built, or the one it
             * was given */
            RID rid;
            /* The start track is not there yet: whichever comes last, the vehicle or the track,
             * places it */
            bool start_track_pending = false;
            /* "Edit FIZ": the vehicle's models stand as nodes the editor shows - not a property,
             * a view of the vehicle that the scene does not keep */
            bool editable_in_editor = false;

            void _on_physics_node_vehicle_changed();
            /* A model of a vehicle assembled by hand is built after this node enters the tree, and
             * built again whenever it reloads: the servers are handed it every time */
            void _on_model_instance_created(const RID &p_instance);
            /* The models the scene names */
            Vector<Node *> _model_nodes() const;
            void _connect_scene_parts(bool p_connected);
            void _on_tracks_changed();
            void _place_on_start_track();
            /* The models and the parts the scene names, handed to RailVehicleRenderingServer */
            void _hand_over_assembly();
            String _submodel_name(const NodePath &p_path) const;
            PackedStringArray _submodel_names(const TypedArray<NodePath> &p_paths) const;
            RID _model_instance(const NodePath &p_path) const;

        protected:
            static void _bind_methods();
            void _notification(int p_what); // NOLINT(bugprone-derived-method-shadowing-base-method)

        public:
            /* Takes the vehicle p_vehicle: the servers are handed it with what the scene set, and
             * whoever follows this node hears vehicle_changed */
            void set_vehicle(const RID &p_vehicle);
            /* This vehicle's handle - the key anything keeping per-vehicle state of its own uses */
            RID get_rid() const;
            /* The controller the vehicle runs on, null before it has one */
            Ref<RailVehicleController> get_controller() const;
            /* The vehicle's models shown in the editor's Scene dock as nodes, and edited there
             * (RailVehicleRenderingServer::vehicle_set_editable()) - every vehicle this node
             * draws from now on */
            void set_editable_in_editor(bool p_editable);
            bool is_editable_in_editor() const;

            void set_controller_path(const NodePath &p_value);
            NodePath get_controller_path() const;
            void set_appearance(const Ref<RailVehicleAppearance> &p_value);
            Ref<RailVehicleAppearance> get_appearance() const;
            void set_model_instance_path(const NodePath &p_value);
            NodePath get_model_instance_path() const;
            void set_low_poly_cabin_path(const NodePath &p_value);
            NodePath get_low_poly_cabin_path() const;
            void set_front_bogie_path(const NodePath &p_value);
            NodePath get_front_bogie_path() const;
            void set_rear_bogie_path(const NodePath &p_value);
            NodePath get_rear_bogie_path() const;
            void set_front_rolling_wheel_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_front_rolling_wheel_paths() const;
            void set_powered_wheel_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_powered_wheel_paths() const;
            void set_rear_rolling_wheel_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_rear_rolling_wheel_paths() const;
            void set_pantograph_front_arm_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_pantograph_front_arm_paths() const;
            void set_pantograph_rear_arm_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_pantograph_rear_arm_paths() const;
            void set_wiper_arm_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_wiper_arm_paths() const;
            void set_mirror_paths(const TypedArray<NodePath> &p_value);
            TypedArray<NodePath> get_mirror_paths() const;
            void set_start_track_name(const String &p_value);
            String get_start_track_name() const;
            void set_start_track_offset(double p_value);
            double get_start_track_offset() const;
            void set_start_direction(TrackServer::Direction p_value);
            TrackServer::Direction get_start_direction() const;
            void set_head_display_material(const Ref<Material> &p_value);
            Ref<Material> get_head_display_material() const;
    };
} // namespace godot
