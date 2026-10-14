#include "RailVehicle3D.hpp"
#include "vehicles/base/VehiclePhysicsNode.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleRenderingServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *RailVehicle3D::vehicle_changed_signal = "vehicle_changed";

    void RailVehicle3D::_bind_methods() {
#define BIND_RAIL_NODE_PATH(name, valid_types)                                                                         \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &RailVehicle3D::set_##name);                                 \
    ClassDB::bind_method(D_METHOD("get_" #name), &RailVehicle3D::get_##name);                                          \
    ADD_PROPERTY(                                                                                                      \
            PropertyInfo(Variant::NODE_PATH, #name, PROPERTY_HINT_NODE_PATH_VALID_TYPES, valid_types), "set_" #name,   \
            "get_" #name)
#define BIND_RAIL_NODE_PATH_ARRAY(name)                                                                                \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &RailVehicle3D::set_##name);                                 \
    ClassDB::bind_method(D_METHOD("get_" #name), &RailVehicle3D::get_##name);                                          \
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, #name, PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_" #name, "get_" #name)

        BIND_RAIL_NODE_PATH(controller_path, "VehiclePhysicsNode");
        ClassDB::bind_method(D_METHOD("set_appearance", "value"), &RailVehicle3D::set_appearance);
        ClassDB::bind_method(D_METHOD("get_appearance"), &RailVehicle3D::get_appearance);
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "appearance", PROPERTY_HINT_RESOURCE_TYPE, "RailVehicleAppearance"),
                "set_appearance", "get_appearance");
        BIND_RAIL_NODE_PATH(model_instance_path, "E3DModelInstance");
        BIND_RAIL_NODE_PATH(low_poly_cabin_path, "E3DModelInstance");
        BIND_RAIL_NODE_PATH(front_bogie_path, "Node3D");
        BIND_RAIL_NODE_PATH(rear_bogie_path, "Node3D");
        BIND_RAIL_NODE_PATH_ARRAY(front_rolling_wheel_paths);
        BIND_RAIL_NODE_PATH_ARRAY(powered_wheel_paths);
        BIND_RAIL_NODE_PATH_ARRAY(rear_rolling_wheel_paths);
        BIND_RAIL_NODE_PATH_ARRAY(pantograph_front_arm_paths);
        BIND_RAIL_NODE_PATH_ARRAY(pantograph_rear_arm_paths);
        BIND_RAIL_NODE_PATH_ARRAY(wiper_arm_paths);
        BIND_RAIL_NODE_PATH_ARRAY(mirror_paths);
        ClassDB::bind_method(D_METHOD("set_start_track_name", "value"), &RailVehicle3D::set_start_track_name);
        ClassDB::bind_method(D_METHOD("get_start_track_name"), &RailVehicle3D::get_start_track_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "start_track_name"), "set_start_track_name", "get_start_track_name");
        ClassDB::bind_method(D_METHOD("set_start_track_offset", "value"), &RailVehicle3D::set_start_track_offset);
        ClassDB::bind_method(D_METHOD("get_start_track_offset"), &RailVehicle3D::get_start_track_offset);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "start_track_offset"), "set_start_track_offset", "get_start_track_offset");
        ClassDB::bind_method(D_METHOD("set_start_direction", "value"), &RailVehicle3D::set_start_direction);
        ClassDB::bind_method(D_METHOD("get_start_direction"), &RailVehicle3D::get_start_direction);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "start_direction", PROPERTY_HINT_ENUM, "Normal,Reversed",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackServer.Direction"),
                "set_start_direction", "get_start_direction");
        ClassDB::bind_method(D_METHOD("set_head_display_material", "value"), &RailVehicle3D::set_head_display_material);
        ClassDB::bind_method(D_METHOD("get_head_display_material"), &RailVehicle3D::get_head_display_material);
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "head_display_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
                "set_head_display_material", "get_head_display_material");

        ClassDB::bind_method(D_METHOD("set_vehicle", "vehicle"), &RailVehicle3D::set_vehicle);
        ClassDB::bind_method(D_METHOD("set_editable_in_editor", "editable"), &RailVehicle3D::set_editable_in_editor);
        ClassDB::bind_method(D_METHOD("is_editable_in_editor"), &RailVehicle3D::is_editable_in_editor);
        ClassDB::bind_method(D_METHOD("get_rid"), &RailVehicle3D::get_rid);
        ClassDB::bind_method(D_METHOD("get_controller"), &RailVehicle3D::get_controller);
#undef BIND_RAIL_NODE_PATH_ARRAY
#undef BIND_RAIL_NODE_PATH
        /* The vehicle this node draws has changed - it has one now, or a different one. Whoever
         * needs the vehicle reacts to this instead of looking for it again later. */
        ADD_SIGNAL(MethodInfo(vehicle_changed_signal));
    }

    /* The node's own work happens as it enters and leaves the tree, never per frame: a subclass
     * written in a script has its own _enter_tree() and _exit_tree() beside these. */
    void RailVehicle3D::_notification(const int p_what) {
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE: {
                // moved in the editor (its gizmo), the vehicle on no track goes along
                set_notify_transform(Engine::get_singleton()->is_editor_hint());
                if (TrackServer *tracks = TrackServer::get_instance(); tracks != nullptr) {
                    tracks->connect(
                            TrackServer::tracks_changed_signal, callable_mp(this, &RailVehicle3D::_on_tracks_changed));
                }
                _connect_scene_parts(true);
            } break;
            case NOTIFICATION_EXIT_TREE: {
                if (TrackServer *tracks = TrackServer::get_instance(); tracks != nullptr) {
                    tracks->disconnect(
                            TrackServer::tracks_changed_signal, callable_mp(this, &RailVehicle3D::_on_tracks_changed));
                }
                _connect_scene_parts(false);
            } break;
            // the editor takes a scene out of the tree when another one's tab is shown, and all of
            // them draw into one world - the vehicle is drawn only while its node is in it
            case NOTIFICATION_ENTER_WORLD:
            case NOTIFICATION_EXIT_WORLD: {
                RailVehicleRenderingServer *drawing = RailVehicleRenderingServer::get_instance();
                if (drawing != nullptr && drawing->vehicle_is_attached(rid)) {
                    drawing->vehicle_set_scenario(
                            rid, p_what == NOTIFICATION_ENTER_WORLD ? get_world_3d()->get_scenario() : RID());
                }
            } break;
            // a vehicle on no track stands where its node does; on a track the node rides on the
            // vehicle instead - and the vehicle moving the node lands here with its own transform
            case NOTIFICATION_TRANSFORM_CHANGED: {
                const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
                RailVehicleRenderingServer *drawing = RailVehicleRenderingServer::get_instance();
                if (rail_vehicles == nullptr || drawing == nullptr || !drawing->vehicle_is_attached(rid) ||
                    RID(rail_vehicles->vehicle_get_track_position(rid).get("track_rid", RID())).is_valid() ||
                    drawing->vehicle_get_transform(rid).is_equal_approx(get_global_transform())) {
                    break;
                }
                drawing->vehicle_set_transform(rid, get_global_transform());
            } break;
            default:
                break;
        }
    }

    void RailVehicle3D::_on_physics_node_vehicle_changed() {
        const VehiclePhysicsNode *physics = Object::cast_to<VehiclePhysicsNode>(get_node_or_null(controller_path));
        if (physics != nullptr) {
            set_vehicle(physics->get_vehicle_rid());
        }
    }

    /* A vehicle assembled by hand is its VehiclePhysicsNode's - taken whenever that one builds it,
     * which is on entering the tree, right after this node - and its models are built after this
     * node enters the tree and again whenever they reload: the servers are handed them every time.
     * Connected while this node is in the tree, to the parts its paths name now. */
    void RailVehicle3D::_connect_scene_parts(const bool p_connected) {
        VehiclePhysicsNode *physics = controller_path.is_empty()
                                              ? nullptr
                                              : Object::cast_to<VehiclePhysicsNode>(get_node_or_null(controller_path));
        const Callable on_vehicle_changed = callable_mp(this, &RailVehicle3D::_on_physics_node_vehicle_changed);
        const Callable on_model_created = callable_mp(this, &RailVehicle3D::_on_model_instance_created);
        if (!p_connected) {
            if (physics != nullptr &&
                physics->is_connected(VehiclePhysicsNode::vehicle_changed_signal, on_vehicle_changed)) {
                physics->disconnect(VehiclePhysicsNode::vehicle_changed_signal, on_vehicle_changed);
            }
            // E3DModelInstance is a GDScript node, unknown at build time: its signal by name
            for (Node *model: _model_nodes()) {
                model->disconnect("e3d_instance_created", on_model_created);
            }
            return;
        }
        for (Node *model: _model_nodes()) {
            model->connect("e3d_instance_created", on_model_created);
        }
        if (physics != nullptr) {
            physics->connect(VehiclePhysicsNode::vehicle_changed_signal, on_vehicle_changed);
            if (physics->get_vehicle_rid().is_valid()) {
                set_vehicle(physics->get_vehicle_rid());
            }
        } else if (rid.is_valid()) {
            _hand_over_assembly();
        }
    }

    Vector<Node *> RailVehicle3D::_model_nodes() const {
        Vector<Node *> models;
        for (const NodePath &path: {model_instance_path, low_poly_cabin_path}) {
            if (Node *model = path.is_empty() ? nullptr : get_node_or_null(path); model != nullptr) {
                models.push_back(model);
            }
        }
        return models;
    }

    void RailVehicle3D::_on_model_instance_created(const RID &p_instance) {
        if (rid.is_valid()) {
            _hand_over_assembly();
        }
    }

    void RailVehicle3D::set_vehicle(const RID &p_vehicle) {
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        RailVehicleRenderingServer *drawing = RailVehicleRenderingServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        ERR_FAIL_NULL(drawing);
        rid = p_vehicle;
        if (rid.is_valid()) {
            // the vehicle and its handle are the VehiclePhysicsNode's; this node makes it a rail
            // one - a place on the route, stepped there - and draws it
            rail_vehicles->vehicle_attach(rid);
            // drawn in this node's world - unless whoever built the vehicle from data draws it
            // already - and the node rides on the vehicle wherever it is placed
            if (!drawing->vehicle_is_attached(rid)) {
                drawing->vehicle_attach(rid, get_instance_id());
            }
            // it stands where this node does until it is placed on a track
            drawing->vehicle_set_transform(rid, get_global_transform());
            drawing->vehicle_mount_node(rid, get_instance_id());
            _hand_over_assembly();
            if (head_display_material.is_valid()) {
                drawing->vehicle_set_head_display_material(rid, head_display_material);
            }
            drawing->vehicle_set_editable(rid, editable_in_editor);
            start_track_pending = !start_track_name.is_empty();
            _place_on_start_track();
        }
        emit_signal(vehicle_changed_signal);
    }

    /* Only what the scene set is handed over; a vehicle built from data names nothing here and its
     * builder hands the servers its appearance itself. */
    void RailVehicle3D::_hand_over_assembly() {
        RailVehicleRenderingServer *drawing = RailVehicleRenderingServer::get_instance();
        const RID model = _model_instance(model_instance_path);
        if (model.is_valid()) {
            drawing->vehicle_set_models(rid, model, _model_instance(low_poly_cabin_path));
        }
        if (appearance.is_null() && !model.is_valid()) {
            return;
        }
        // the appearance the scene gave, with the parts its paths name
        Ref<RailVehicleAppearance> assembled = appearance.is_valid()
                                                       ? Ref<RailVehicleAppearance>(appearance->duplicate())
                                                       : Ref<RailVehicleAppearance>();
        if (assembled.is_null()) {
            assembled.instantiate();
        }
        if (model.is_valid()) {
            assembled->set_front_bogie(_submodel_name(front_bogie_path));
            assembled->set_rear_bogie(_submodel_name(rear_bogie_path));
            assembled->set_front_rolling_wheels(_submodel_names(front_rolling_wheel_paths));
            assembled->set_powered_wheels(_submodel_names(powered_wheel_paths));
            assembled->set_rear_rolling_wheels(_submodel_names(rear_rolling_wheel_paths));
            assembled->set_pantograph_front_arms(_submodel_names(pantograph_front_arm_paths));
            assembled->set_pantograph_rear_arms(_submodel_names(pantograph_rear_arm_paths));
            assembled->set_wiper_arms(_submodel_names(wiper_arm_paths));
            assembled->set_mirrors(_submodel_names(mirror_paths));
            // where the model sits in this node
            if (const Node3D *model_node = Object::cast_to<Node3D>(get_node_or_null(model_instance_path));
                model_node != nullptr) {
                assembled->set_model_transform(
                        get_global_transform().affine_inverse() * model_node->get_global_transform());
            }
        }
        drawing->vehicle_set_appearance(rid, assembled);
    }

    /* A part of a model is one of its submodels, and a submodel node is named after it */
    String RailVehicle3D::_submodel_name(const NodePath &p_path) const {
        const Node *node = p_path.is_empty() ? nullptr : get_node_or_null(p_path);
        return node != nullptr ? String(node->get_name()).to_lower() : String();
    }

    PackedStringArray RailVehicle3D::_submodel_names(const TypedArray<NodePath> &p_paths) const {
        PackedStringArray names;
        for (int index = 0; index < p_paths.size(); ++index) {
            names.push_back(_submodel_name(p_paths[index]));
        }
        return names;
    }

    RID RailVehicle3D::_model_instance(const NodePath &p_path) const {
        Node *model = p_path.is_empty() ? nullptr : get_node_or_null(p_path);
        // E3DModelInstance is a GDScript node, unknown at build time
        return model != nullptr ? RID(model->call("get_e3d_instance")) : RID();
    }

    void RailVehicle3D::_on_tracks_changed() {
        _place_on_start_track();
    }

    /* Placing needs both the track and the vehicle: whichever comes last places it. */
    void RailVehicle3D::_place_on_start_track() {
        TrackServer *tracks = TrackServer::get_instance();
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        if (!start_track_pending || !rid.is_valid() || tracks == nullptr || rail_vehicles == nullptr) {
            return;
        }
        const RID track = tracks->track_get_rid_by_name(start_track_name);
        if (!track.is_valid()) {
            return;
        }
        start_track_pending = false;
        rail_vehicles->vehicle_set_track(rid, track, start_track_offset, start_direction);
    }

    void RailVehicle3D::set_editable_in_editor(const bool p_editable) {
        editable_in_editor = p_editable;
        RailVehicleRenderingServer *drawing = RailVehicleRenderingServer::get_instance();
        if (drawing != nullptr && rid.is_valid()) {
            drawing->vehicle_set_editable(rid, editable_in_editor);
        }
    }

    bool RailVehicle3D::is_editable_in_editor() const {
        return editable_in_editor;
    }

    RID RailVehicle3D::get_rid() const {
        return rid;
    }

    Ref<RailVehicleController> RailVehicle3D::get_controller() const {
        const VehicleServer *server = VehicleServer::get_instance();
        return server != nullptr && rid.is_valid() ? Ref<RailVehicleController>(server->vehicle_get_controller(rid))
                                                   : Ref<RailVehicleController>();
    }

#define DEFINE_PROPERTY(type, name)                                                                                    \
    void RailVehicle3D::set_##name(const type &p_value) {                                                              \
        name = p_value;                                                                                                \
    }                                                                                                                  \
    type RailVehicle3D::get_##name() const {                                                                           \
        return name;                                                                                                   \
    }
    DEFINE_PROPERTY(Ref<RailVehicleAppearance>, appearance)
    DEFINE_PROPERTY(NodePath, front_bogie_path)
    DEFINE_PROPERTY(NodePath, rear_bogie_path)
    DEFINE_PROPERTY(TypedArray<NodePath>, front_rolling_wheel_paths)
    DEFINE_PROPERTY(TypedArray<NodePath>, powered_wheel_paths)
    DEFINE_PROPERTY(TypedArray<NodePath>, rear_rolling_wheel_paths)
    DEFINE_PROPERTY(TypedArray<NodePath>, pantograph_front_arm_paths)
    DEFINE_PROPERTY(TypedArray<NodePath>, pantograph_rear_arm_paths)
    DEFINE_PROPERTY(TypedArray<NodePath>, wiper_arm_paths)
    DEFINE_PROPERTY(TypedArray<NodePath>, mirror_paths)
#undef DEFINE_PROPERTY

    /* A path naming a part of the scene connects this node to that part instead */
#define DEFINE_PART_PATH(name)                                                                                         \
    void RailVehicle3D::set_##name(const NodePath &p_value) {                                                          \
        if (is_inside_tree()) {                                                                                        \
            _connect_scene_parts(false);                                                                               \
        }                                                                                                              \
        name = p_value;                                                                                                \
        if (is_inside_tree()) {                                                                                        \
            _connect_scene_parts(true);                                                                                \
        }                                                                                                              \
    }                                                                                                                  \
    NodePath RailVehicle3D::get_##name() const {                                                                       \
        return name;                                                                                                   \
    }
    DEFINE_PART_PATH(controller_path)
    DEFINE_PART_PATH(model_instance_path)
    DEFINE_PART_PATH(low_poly_cabin_path)
#undef DEFINE_PART_PATH

    /* A placed vehicle is placed again where the scene now says */
    void RailVehicle3D::set_start_track_name(const String &p_value) {
        start_track_name = p_value;
        start_track_pending = !start_track_name.is_empty();
        _place_on_start_track();
    }

    String RailVehicle3D::get_start_track_name() const {
        return start_track_name;
    }

    void RailVehicle3D::set_start_track_offset(const double p_value) {
        start_track_offset = p_value;
        start_track_pending = !start_track_name.is_empty();
        _place_on_start_track();
    }

    double RailVehicle3D::get_start_track_offset() const {
        return start_track_offset;
    }

    void RailVehicle3D::set_start_direction(const TrackServer::Direction p_value) {
        start_direction = p_value;
        start_track_pending = !start_track_name.is_empty();
        _place_on_start_track();
    }

    TrackServer::Direction RailVehicle3D::get_start_direction() const {
        return start_direction;
    }

    void RailVehicle3D::set_head_display_material(const Ref<Material> &p_value) {
        head_display_material = p_value;
        if (RailVehicleRenderingServer *drawing = RailVehicleRenderingServer::get_instance();
            drawing != nullptr && drawing->vehicle_is_attached(rid)) {
            drawing->vehicle_set_head_display_material(rid, head_display_material);
        }
    }

    Ref<Material> RailVehicle3D::get_head_display_material() const {
        return head_display_material;
    }
} // namespace godot
