#include "RailVehicleServer.hpp"
#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehiclePowerSupply.hpp"
#include "vehicles/rail/RailVehicleRadio.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"

#include "logging/GameLog.hpp"
#include "radio/RadioServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "traction/TractionServer.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleServer.hpp"

#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /* Reports physics inconsistencies with push_error (see _check_movement) */
    constexpr const char *DIAGNOSTICS_SETTING = "maszyna/debug/physics_diagnostics";
    const char *RailVehicleServer::vehicle_placed_signal = "vehicle_placed";
    const char *RailVehicleServer::vehicle_placement_changed_signal = "vehicle_placement_changed";
    const char *RailVehicleServer::vehicle_trainset_changed_signal = "vehicle_trainset_changed";
    const char *RailVehicleServer::vehicle_derailed_signal = "vehicle_derailed";
    const char *RailVehicleServer::vehicle_driver_cabin_changed_signal = "vehicle_driver_cabin_changed";
    const char *RailVehicleServer::vehicle_coupler_attached_signal = "vehicle_coupler_attached";
    const char *RailVehicleServer::vehicle_coupler_detached_signal = "vehicle_coupler_detached";
    const char *RailVehicleServer::vehicle_coupler_adapter_attached_signal = "vehicle_coupler_adapter_attached";
    const char *RailVehicleServer::vehicle_coupler_adapter_removed_signal = "vehicle_coupler_adapter_removed";
    const char *RailVehicleServer::vehicle_heading_to_track_start_signal = "vehicle_heading_to_track_start";
    const char *RailVehicleServer::vehicle_heading_to_track_end_signal = "vehicle_heading_to_track_end";
    const char *RailVehicleServer::vehicle_stopped_on_track_signal = "vehicle_stopped_on_track";
    const char *RailVehicleServer::vehicle_secondary_axle_stopped_on_track_signal =
            "vehicle_secondary_axle_stopped_on_track";
    const char *RailVehicleServer::vehicle_emergency_signal_received_signal = "vehicle_emergency_signal_received";
    const char *RailVehicleServer::vehicle_pantograph_contact_lost_signal = "vehicle_pantograph_contact_lost";

    RailVehicleServer::RailVehicleServer() {
        ProjectSettings *settings = ProjectSettings::get_singleton();
        diagnostics = settings->get_setting(DIAGNOSTICS_SETTING, false);
        settings->connect("settings_changed", callable_mp(this, &RailVehicleServer::_on_project_settings_changed));
        // the vehicle is VehicleServer's: freed there, it leaves the route; driven by another
        // controller, that one is stepped
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &RailVehicleServer::vehicle_detach));
        vehicle_server->connect(
                VehicleServer::vehicle_controller_changed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_controller_changed));
        vehicle_server->connect(
                VehicleServer::vehicle_configured_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_configured));
        vehicle_server->connect(
                VehicleServer::vehicle_config_changed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_config_changed));
        // who drives from which cabin is the controller's occupied cab
        vehicle_server->connect(
                VehicleServer::cabin_person_entered_signal,
                callable_mp(this, &RailVehicleServer::_on_cabin_person_entered));
        vehicle_server->connect(
                VehicleServer::cabin_person_left_signal, callable_mp(this, &RailVehicleServer::_on_cabin_person_left));
        vehicle_server->connect(
                VehicleServer::cabin_person_role_changed_signal,
                callable_mp(this, &RailVehicleServer::_on_cabin_person_role_changed));
        vehicle_server->connect(
                VehicleServer::cabin_person_moved_signal,
                callable_mp(this, &RailVehicleServer::_on_cabin_person_moved));
        vehicle_server->connect(
                VehicleServer::vehicle_cabin_detached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_cabin_detached));
        // where a neighbour scan goes depends on the switches and the network
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        tracks->connect(
                TrackServer::switch_active_track_changed_signal,
                callable_mp(this, &RailVehicleServer::_on_switch_active_track_changed));
        tracks->connect(
                TrackServer::tracks_changed_signal, callable_mp(this, &RailVehicleServer::_on_topology_changed));
        tracks->connect(
                TrackServer::topology_changed_signal, callable_mp(this, &RailVehicleServer::_on_topology_changed));
    }

    void RailVehicleServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("trainset_create"), &RailVehicleServer::trainset_create);
        ClassDB::bind_method(D_METHOD("trainset_free", "trainset"), &RailVehicleServer::trainset_free);
        ClassDB::bind_method(D_METHOD("trainset_set_name", "trainset", "name"), &RailVehicleServer::trainset_set_name);
        ClassDB::bind_method(D_METHOD("trainset_get_name", "trainset"), &RailVehicleServer::trainset_get_name);
        ClassDB::bind_method(
                D_METHOD("trainset_set_track", "trainset", "track", "offset"), &RailVehicleServer::trainset_set_track);
        ClassDB::bind_method(D_METHOD("trainset_clear", "trainset"), &RailVehicleServer::trainset_clear);
        ClassDB::bind_method(
                D_METHOD("trainset_add_vehicle", "trainset", "vehicle", "direction", "gap", "coupling"),
                &RailVehicleServer::trainset_add_vehicle);
        ClassDB::bind_method(D_METHOD("trainset_get_vehicles", "trainset"), &RailVehicleServer::trainset_get_vehicles);
        ClassDB::bind_method(D_METHOD("trainset_place", "trainset"), &RailVehicleServer::trainset_place);
        ClassDB::bind_method(D_METHOD("vehicle_attach", "vehicle"), &RailVehicleServer::vehicle_attach);
        ClassDB::bind_method(
                D_METHOD("vehicle_add_front_cabin", "vehicle"), &RailVehicleServer::vehicle_add_front_cabin);
        ClassDB::bind_method(D_METHOD("vehicle_add_rear_cabin", "vehicle"), &RailVehicleServer::vehicle_add_rear_cabin);
        ClassDB::bind_method(
                D_METHOD("vehicle_add_machine_room", "vehicle"), &RailVehicleServer::vehicle_add_machine_room);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_front_cabin", "vehicle"), &RailVehicleServer::vehicle_get_front_cabin);
        ClassDB::bind_method(D_METHOD("vehicle_get_rear_cabin", "vehicle"), &RailVehicleServer::vehicle_get_rear_cabin);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_machine_room", "vehicle"), &RailVehicleServer::vehicle_get_machine_room);
        ClassDB::bind_method(D_METHOD("cabin_get_kind", "cabin"), &RailVehicleServer::cabin_get_kind);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_driver_cabin", "vehicle"), &RailVehicleServer::vehicle_get_driver_cabin);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_leading_cabin", "vehicle"), &RailVehicleServer::vehicle_get_leading_cabin);
        ClassDB::bind_method(
                D_METHOD("person_enter_front_cabin", "person", "vehicle", "role"),
                &RailVehicleServer::person_enter_front_cabin);
        ClassDB::bind_method(
                D_METHOD("person_enter_rear_cabin", "person", "vehicle", "role"),
                &RailVehicleServer::person_enter_rear_cabin);
        ClassDB::bind_method(
                D_METHOD("person_enter_machine_room", "person", "vehicle", "role"),
                &RailVehicleServer::person_enter_machine_room);
        ClassDB::bind_method(
                D_METHOD("person_move_to_front_cabin", "person"), &RailVehicleServer::person_move_to_front_cabin);
        ClassDB::bind_method(
                D_METHOD("person_change_cabin", "person", "direction"), &RailVehicleServer::person_change_cabin);
        BIND_ENUM_CONSTANT(CABIN_CHANGE_FORWARD);
        BIND_ENUM_CONSTANT(CABIN_CHANGE_BACKWARD);
        BIND_ENUM_CONSTANT(PANTOGRAPH_CONTACT_LOSS_NOT_REACHING);
        BIND_ENUM_CONSTANT(PANTOGRAPH_CONTACT_LOSS_NO_WIRE);
        BIND_ENUM_CONSTANT(PANTOGRAPH_CONTACT_LOSS_DEAD_WIRE);
        ClassDB::bind_method(
                D_METHOD("person_move_to_rear_cabin", "person"), &RailVehicleServer::person_move_to_rear_cabin);
        ClassDB::bind_method(
                D_METHOD("person_move_to_machine_room", "person"), &RailVehicleServer::person_move_to_machine_room);
        ClassDB::bind_method(
                D_METHOD("vehicle_front_cabin_has_person_role", "vehicle", "role"),
                &RailVehicleServer::vehicle_front_cabin_has_person_role);
        ClassDB::bind_method(
                D_METHOD("vehicle_rear_cabin_has_person_role", "vehicle", "role"),
                &RailVehicleServer::vehicle_rear_cabin_has_person_role);
        ClassDB::bind_method(
                D_METHOD("vehicle_machine_room_has_person_role", "vehicle", "role"),
                &RailVehicleServer::vehicle_machine_room_has_person_role);
        ClassDB::bind_method(
                D_METHOD("vehicle_front_cabin_list_persons", "vehicle", "role"),
                &RailVehicleServer::vehicle_front_cabin_list_persons);
        ClassDB::bind_method(
                D_METHOD("vehicle_rear_cabin_list_persons", "vehicle", "role"),
                &RailVehicleServer::vehicle_rear_cabin_list_persons);
        ClassDB::bind_method(
                D_METHOD("vehicle_machine_room_list_persons", "vehicle", "role"),
                &RailVehicleServer::vehicle_machine_room_list_persons);
        ClassDB::bind_method(D_METHOD("vehicle_detach", "vehicle"), &RailVehicleServer::vehicle_detach);
        ClassDB::bind_method(D_METHOD("vehicle_is_attached", "vehicle"), &RailVehicleServer::vehicle_is_attached);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_type_name", "vehicle", "type_name"), &RailVehicleServer::vehicle_set_type_name);
        ClassDB::bind_method(D_METHOD("vehicle_get_type_name", "vehicle"), &RailVehicleServer::vehicle_get_type_name);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_load", "vehicle", "load_name", "load_amount"),
                &RailVehicleServer::vehicle_set_load);
        ClassDB::bind_method(
                D_METHOD("load_add", "vehicle", "amount", "side", "load_name"), &RailVehicleServer::load_add,
                DEFVAL(String()));
        ClassDB::bind_method(D_METHOD("load_remove", "vehicle", "amount", "side"), &RailVehicleServer::load_remove);
        ClassDB::bind_method(D_METHOD("load_get_exchange_time", "vehicle"), &RailVehicleServer::load_get_exchange_time);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_rids_in_rect", "rect"), &RailVehicleServer::vehicle_get_rids_in_rect);
        ClassDB::bind_method(
                D_METHOD("vehicle_component_get", "vehicle", "type"), &RailVehicleServer::vehicle_component_get);
        ClassDB::bind_method(
                D_METHOD("vehicle_couple", "vehicle", "end", "other", "other_end", "coupling"),
                &RailVehicleServer::vehicle_couple);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_coupled", "vehicle", "end", "flags"), &RailVehicleServer::vehicle_get_coupled);
        ClassDB::bind_method(D_METHOD("vehicle_find_powered", "vehicle"), &RailVehicleServer::vehicle_find_powered);
        ClassDB::bind_method(
                D_METHOD("vehicle_find_pantograph_carrier", "vehicle"),
                &RailVehicleServer::vehicle_find_pantograph_carrier);

        ClassDB::bind_method(
                D_METHOD("vehicle_set_track", "vehicle", "track", "track_offset", "track_direction"),
                &RailVehicleServer::vehicle_set_track);
        ClassDB::bind_method(D_METHOD("vehicle_move", "vehicle", "distance"), &RailVehicleServer::vehicle_move);
        ClassDB::bind_method(D_METHOD("trainset_move", "vehicle", "distance"), &RailVehicleServer::trainset_move);
        ClassDB::bind_method(
                D_METHOD("trainset_get_doorway_open", "vehicle", "side"),
                &RailVehicleServer::trainset_get_doorway_open);
        ClassDB::bind_method(
                D_METHOD("trainset_get_door_open", "vehicle", "side"), &RailVehicleServer::trainset_get_door_open);
        ClassDB::bind_method(
                D_METHOD("trainset_get_door_permit", "vehicle", "side"), &RailVehicleServer::trainset_get_door_permit);
        ClassDB::bind_method(
                D_METHOD("trainset_determine_type", "vehicle"), &RailVehicleServer::trainset_determine_type);
        ClassDB::bind_method(D_METHOD("trainset_get_type", "vehicle"), &RailVehicleServer::trainset_get_type);
        BIND_ENUM_CONSTANT(TRAINSET_TYPE_NONE);
        BIND_ENUM_CONSTANT(TRAINSET_TYPE_PASSENGER);
        BIND_ENUM_CONSTANT(TRAINSET_TYPE_CARGO);
        BIND_ENUM_CONSTANT(TRAINSET_TYPE_MIXED);
        ClassDB::bind_method(
                D_METHOD("vehicle_process_movement", "vehicle", "delta"), &RailVehicleServer::vehicle_process_movement);
        ClassDB::bind_method(D_METHOD("vehicle_get_transform", "vehicle"), &RailVehicleServer::vehicle_get_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_coupler_adapter_model", "vehicle", "end"),
                &RailVehicleServer::vehicle_get_coupler_adapter_model);
        ClassDB::bind_method(
                D_METHOD("vehicle_is_coupler_automatic", "vehicle", "end"),
                &RailVehicleServer::vehicle_is_coupler_automatic);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_coupler_joinable_flags", "vehicle", "end"),
                &RailVehicleServer::vehicle_get_coupler_joinable_flags);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_coupler_adapter_length", "vehicle", "end"),
                &RailVehicleServer::vehicle_get_coupler_adapter_length);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_coupler_adapter_height", "vehicle", "end"),
                &RailVehicleServer::vehicle_get_coupler_adapter_height);
        ClassDB::bind_method(
                D_METHOD(
                        "vehicle_set_pantograph_geometry", "vehicle", "pantograph", "position", "lower_length",
                        "upper_length", "horizontal", "lower_rest_angle", "upper_rest_angle", "slider_height"),
                &RailVehicleServer::vehicle_set_pantograph_geometry);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_pantograph_position", "vehicle", "pantograph"),
                &RailVehicleServer::vehicle_get_pantograph_position);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_pantograph_raise", "vehicle", "pantograph"),
                &RailVehicleServer::vehicle_get_pantograph_raise);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_bogie_transform", "vehicle", "bogie"),
                &RailVehicleServer::vehicle_get_bogie_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_track_position", "vehicle"), &RailVehicleServer::vehicle_get_track_position);
        ClassDB::bind_method(
                D_METHOD("vehicle_trace_route", "vehicle", "direction", "distance"),
                &RailVehicleServer::vehicle_trace_route);
        ClassDB::bind_method(
                D_METHOD("vehicle_find_vehicle", "vehicle", "end", "distance"),
                &RailVehicleServer::vehicle_find_vehicle);
        ClassDB::bind_method(D_METHOD("vehicle_get_curve", "vehicle"), &RailVehicleServer::vehicle_get_curve);

        ADD_SIGNAL(MethodInfo(vehicle_emergency_signal_received_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(
                vehicle_pantograph_contact_lost_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::INT, "pantograph"),
                PropertyInfo(
                        Variant::INT, "cause", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM,
                        "RailVehicleServer.PantographContactLoss")));
        ADD_SIGNAL(MethodInfo(vehicle_trainset_changed_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(
                vehicle_derailed_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(
                        Variant::INT, "cause", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "RailVehicleController.DerailCause")));
        ADD_SIGNAL(MethodInfo(
                vehicle_driver_cabin_changed_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "cabin")));
        ADD_SIGNAL(MethodInfo(vehicle_placed_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(vehicle_placement_changed_signal, PropertyInfo(Variant::RID, "vehicle")));
        const PropertyInfo coupling_flag(
                Variant::INT, "flag", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD,
                "RailVehicleController.CouplingFlags");
        ADD_SIGNAL(MethodInfo(vehicle_coupler_attached_signal, PropertyInfo(Variant::RID, "vehicle"), coupling_flag));
        ADD_SIGNAL(MethodInfo(vehicle_coupler_detached_signal, PropertyInfo(Variant::RID, "vehicle"), coupling_flag));
        ADD_SIGNAL(MethodInfo(
                vehicle_coupler_adapter_attached_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::INT, "end")));
        ADD_SIGNAL(MethodInfo(
                vehicle_coupler_adapter_removed_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::INT, "end")));
        ADD_SIGNAL(MethodInfo(
                vehicle_heading_to_track_start_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "track")));
        ADD_SIGNAL(MethodInfo(
                vehicle_heading_to_track_end_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "track")));
        ADD_SIGNAL(MethodInfo(
                vehicle_stopped_on_track_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "track")));
        ADD_SIGNAL(MethodInfo(
                vehicle_secondary_axle_stopped_on_track_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "track")));
    }


    void RailVehicleServer::vehicle_set_pantograph_geometry(
            const RID &p_vehicle, const RailVehicleEnginePowerSource::PantographSelector p_pantograph,
            const Vector3 &p_position, const double p_lower_length, const double p_upper_length,
            const double p_horizontal, const double p_lower_rest_angle, const double p_upper_rest_angle,
            const double p_slider_height) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        ERR_FAIL_INDEX(static_cast<int>(p_pantograph), 2);
        Pantograph &pantograph = placement->pantographs[p_pantograph];
        // built lowered; a model rebuilt later changes how the arms are drawn, not how far they are up
        if (!pantograph.present) {
            pantograph.present = true;
            pantograph.lower_angle = p_lower_rest_angle;
            pantograph.upper_angle = p_upper_rest_angle;
            pantograph.height = (p_lower_length * Math::sin(p_lower_rest_angle)) +
                                (p_upper_length * Math::sin(p_upper_rest_angle)) + p_slider_height;
            pantograph.reaches_wire = false;
        }
        pantograph.position = p_position;
        pantograph.lower_length = p_lower_length;
        pantograph.upper_length = p_upper_length;
        pantograph.horizontal = p_horizontal;
        pantograph.lower_rest_angle = p_lower_rest_angle;
        pantograph.upper_rest_angle = p_upper_rest_angle;
        pantograph.slider_height = p_slider_height;
    }

    Vector3 RailVehicleServer::vehicle_get_pantograph_position(
            const RID &p_vehicle, const RailVehicleEnginePowerSource::PantographSelector p_pantograph) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(placement, Vector3());
        ERR_FAIL_INDEX_V(static_cast<int>(p_pantograph), 2, Vector3());
        return placement->pantographs[p_pantograph].position;
    }

    Vector2 RailVehicleServer::vehicle_get_pantograph_raise(
            const RID &p_vehicle, const RailVehicleEnginePowerSource::PantographSelector p_pantograph) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(placement, Vector2());
        ERR_FAIL_INDEX_V(static_cast<int>(p_pantograph), 2, Vector2());
        const Pantograph &pantograph = placement->pantographs[p_pantograph];
        return {static_cast<real_t>(pantograph.lower_angle - pantograph.lower_rest_angle),
                static_cast<real_t>(pantograph.upper_angle - pantograph.upper_rest_angle)};
    }

    String RailVehicleServer::_track_position_text(const RID &p_vehicle) const {
        const TrackServer *tracks = TrackServer::get_instance();
        if (tracks == nullptr) {
            return String("unknown track");
        }
        const Dictionary position = vehicle_get_track_position(p_vehicle);
        const RID track = position.get("track_rid", RID());
        if (!track.is_valid()) {
            return String("no track");
        }
        const String name = tracks->track_get_name(track);
        return vformat(
                "%s at %.2f m", name.is_empty() ? String("(unnamed track)") : name,
                static_cast<double>(position.get("along", 0.0)));
    }

    Dictionary RailVehicleServer::_find_pantograph_wire(
            const RID &p_vehicle, Pantograph &p_pantograph, const int p_index, const Transform3D &p_frame,
            const double p_half_width) {
        TractionServer *traction = TractionServer::get_instance();
        if (traction == nullptr) {
            /* "No wire in reach", the answer the search gives when it finds none: an absent height
             * would read as 0.0 - "the wire is right here" - and fold the pantograph */
            Dictionary missing;
            missing["rid"] = RID();
            missing["height"] = INFINITY;
            return missing;
        }
        const Vector3 contact_point = p_frame.xform(p_pantograph.position);
        const Vector3 up = p_frame.basis.get_column(1);
        const Vector3 forward = -p_frame.basis.get_column(2);
        const Vector3 left = -p_frame.basis.get_column(0);
        /* The wire it was on is followed: running off the end of a span is not a loss of contact,
         * the neighbouring span is reached along the chain in the same step (DynObj.cpp:8742-8770).
         * Its height is taken anew every step - a kept height made the wire step up and down. */
        if (p_pantograph.wire.is_valid()) {
            const Dictionary followed = traction->wire_follow_above(
                    p_pantograph.wire, contact_point, up, forward, left, p_half_width, PANTOGRAPH_HORN_WIDTH);
            if (RID(followed["rid"]).is_valid()) {
                p_pantograph.wire = followed["rid"];
                return followed;
            }
        }
        // the chain ran out: the region is searched as update_traction() does (DynObj.cpp:8799)
        const Dictionary found = traction->wire_find_above_with_height(
                contact_point, up, forward, left, p_half_width, PANTOGRAPH_HORN_WIDTH);
        /* A pantograph that had a wire and now has none is a loss of line voltage that trips the
         * main switch; the original reports it with the place (scene.cpp:112, "Bad traction"), the
         * only way to tell a hole in the scenery's wiring from a defect in this search. */
        if (const VehicleServer *vehicle_server = VehicleServer::get_instance();
            p_pantograph.wire.is_valid() && !RID(found["rid"]).is_valid() && vehicle_server != nullptr) {
            UtilityFunctions::push_warning(
                    vformat("Bad traction: %s lost the wire under pantograph %d - %s, %v",
                            vehicle_server->vehicle_get_id(p_vehicle), p_index, _track_position_text(p_vehicle),
                            contact_point));
        }
        p_pantograph.wire = found["rid"];
        return found;
    }

    void RailVehicleServer::Pantograph::raise(
            const double p_gap, const bool p_active, const double p_speed_factor, const double p_delta) {
        double angle = lower_angle;
        if (p_speed_factor > 0.0 && p_active) {
            if (p_gap > PANTOGRAPH_SETTLED_GAP) {
                angle += MIN(p_speed_factor, PANTOGRAPH_RISE_SHARE * p_gap);
            } else if (p_gap < -PANTOGRAPH_SETTLED_GAP) {
                angle += PANTOGRAPH_PRESS_SHARE * p_gap;
            }
        } else {
            if (angle > lower_rest_angle) {
                angle -= PANTOGRAPH_FALL_RATE * p_delta;
            }
            angle = MAX(angle, lower_rest_angle);
        }
        if (!Math::is_equal_approx(angle, lower_angle)) {
            const double upper = Math::acos(((lower_length * Math::cos(angle)) + horizontal) / upper_length);
            if (angle + upper < Math::PI) {
                lower_angle = angle;
                upper_angle = upper;
                height = (lower_length * Math::sin(angle)) + (upper_length * Math::sin(upper)) + slider_height;
            }
        }
        reaches_wire = p_active && p_gap < PANTOGRAPH_CONTACT_GAP;
    }

    RailVehicleController *RailVehicleServer::_get_controller(const VehiclePlacement &p_placement) const {
        if (p_placement.controller_id.is_null()) {
            return nullptr;
        }
        return Object::cast_to<RailVehicleController>(ObjectDB::get_instance(p_placement.controller_id));
    }

    void RailVehicleServer::vehicle_attach(const RID &p_vehicle) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_COND_MSG(!vehicle_server->vehicle_exists(p_vehicle), "Not a vehicle of VehicleServer");
        if (vehicles.has(p_vehicle)) {
            return;
        }
        VehiclePlacement &placement = vehicles.insert(p_vehicle, VehiclePlacement())->value;
        placement.controller_id = ObjectID(vehicle_server->vehicle_get_controller_instance_id(p_vehicle));
        _connect_relays(p_vehicle, placement);
        _update_driver_cabin(p_vehicle);
    }

    bool RailVehicleServer::vehicle_is_attached(const RID &p_vehicle) const {
        return vehicles.has(p_vehicle);
    }

    void RailVehicleServer::vehicle_set_type_name(const RID &p_vehicle, const String &p_type_name) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        placement->type_name = p_type_name;
        if (RailVehicleController *controller = _get_controller(*placement); controller != nullptr) {
            controller->set_type_name(p_type_name);
        }
    }

    String RailVehicleServer::vehicle_get_type_name(const RID &p_vehicle) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        return placement != nullptr ? placement->type_name : String();
    }

    void
    RailVehicleServer::vehicle_set_load(const RID &p_vehicle, const String &p_load_name, const double p_load_amount) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        placement->load_name = p_load_name;
        placement->load_amount = p_load_amount;
        if (RailVehicleController *controller = _get_controller(*placement); controller != nullptr) {
            controller->set_load_name(p_load_name);
            controller->set_load_amount(p_load_amount);
        }
    }

    RailVehicleLoad *RailVehicleServer::_get_load(const RID &p_vehicle) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr ? Object::cast_to<RailVehicleLoad>(
                                               controller->get_component(VehicleComponentType::COMPONENT_LOAD).ptr())
                                     : nullptr;
    }

    void RailVehicleServer::load_add(
            const RID &p_vehicle, const double p_amount, const RailVehicleLoad::PlatformSide p_side,
            const String &p_load_name) {
        RailVehicleLoad *load = _get_load(p_vehicle);
        ERR_FAIL_NULL_MSG(load, "The vehicle carries no load");
        load->load_add(p_amount, p_side, p_load_name);
    }

    void RailVehicleServer::load_remove(
            const RID &p_vehicle, const double p_amount, const RailVehicleLoad::PlatformSide p_side) {
        RailVehicleLoad *load = _get_load(p_vehicle);
        ERR_FAIL_NULL_MSG(load, "The vehicle carries no load");
        load->load_remove(p_amount, p_side);
    }

    double RailVehicleServer::load_get_exchange_time(const RID &p_vehicle) const {
        const RailVehicleLoad *load = _get_load(p_vehicle);
        return load != nullptr ? load->get_load_exchange_time() : 0.0;
    }

    void RailVehicleServer::_on_load_add_command(const double p_amount, const int p_side, const RID &p_vehicle) {
        load_add(p_vehicle, p_amount, static_cast<RailVehicleLoad::PlatformSide>(p_side));
    }

    void RailVehicleServer::_on_load_remove_command(const double p_amount, const int p_side, const RID &p_vehicle) {
        load_remove(p_vehicle, p_amount, static_cast<RailVehicleLoad::PlatformSide>(p_side));
    }

    void RailVehicleServer::vehicle_detach(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        if (placement == nullptr) {
            return;
        }
        _disconnect_relays(p_vehicle, *placement);
        _disconnect_radio(p_vehicle, *placement);
        const AxleFollower axles[AXLE_COUNT] = {placement->axles[AXLE_FRONT], placement->axles[AXLE_REAR]};
        const RID listed_track = placement->axles[placement->primary_axle].track;
        _track_changed(listed_track);
        vehicles.erase(p_vehicle);
        // Ours: the original never takes a removed vehicle's axles off its sections, which then
        // stay occupied - a vehicle detached here leaves its tracks as it would by driving off
        if (TrackServer *tracks = TrackServer::get_instance(); tracks != nullptr) {
            for (const AxleFollower &axle: axles) {
                tracks->track_axle_left(axle.track, p_vehicle);
            }
            tracks->track_vehicle_left(listed_track, p_vehicle);
        }
    }

    RID RailVehicleServer::trainset_create() {
        const RID trainset = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        trainsets[trainset] = Trainset();
        return trainset;
    }

    void RailVehicleServer::trainset_free(const RID &p_trainset) {
        trainsets.erase(p_trainset);
    }

    void RailVehicleServer::trainset_set_name(const RID &p_trainset, const String &p_name) {
        Trainset *trainset = trainsets.getptr(p_trainset);
        ERR_FAIL_NULL(trainset);
        trainset->name = p_name;
    }

    String RailVehicleServer::trainset_get_name(const RID &p_trainset) const {
        const Trainset *trainset = trainsets.getptr(p_trainset);
        ERR_FAIL_NULL_V(trainset, String());
        return trainset->name;
    }

    void RailVehicleServer::trainset_set_track(const RID &p_trainset, const RID &p_track, const double p_offset) {
        Trainset *trainset = trainsets.getptr(p_trainset);
        ERR_FAIL_NULL(trainset);
        trainset->track = p_track;
        trainset->offset = p_offset;
    }

    void RailVehicleServer::trainset_clear(const RID &p_trainset) {
        Trainset *trainset = trainsets.getptr(p_trainset);
        ERR_FAIL_NULL(trainset);
        trainset->members.clear();
    }

    void RailVehicleServer::trainset_add_vehicle(
            const RID &p_trainset, const RID &p_vehicle, const TrackServer::Direction p_direction, const double p_gap,
            const BitField<RailVehicleController::CouplingFlags> p_coupling) {
        Trainset *trainset = trainsets.getptr(p_trainset);
        ERR_FAIL_NULL(trainset);
        trainset->members.push_back({p_vehicle, p_direction, p_gap, p_coupling});
    }

    TypedArray<RID> RailVehicleServer::trainset_get_vehicles(const RID &p_trainset) const {
        TypedArray<RID> result;
        const Trainset *trainset = trainsets.getptr(p_trainset);
        ERR_FAIL_NULL_V(trainset, result);
        for (const TrainsetMember &member: trainset->members) {
            result.push_back(member.vehicle);
        }
        return result;
    }

    void RailVehicleServer::trainset_place(const RID &p_trainset) {
        Trainset *trainset = trainsets.getptr(p_trainset);
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(trainset);
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_NULL(tracks);
        trainset->placement_pending = true;
        if (!tracks->track_exists(trainset->track)) {
            return;
        }
        for (const TrainsetMember &member: trainset->members) {
            if (!vehicles.has(member.vehicle) || !vehicle_server->vehicle_is_simulation_ready(member.vehicle)) {
                return;
            }
        }
        trainset->placement_pending = false;
        const Vector<TrainsetMember> members = trainset->members;
        // standing anew in another order, the vehicles let go of each other first - coupled again
        // pair by pair, the old pairs would close the trainset into a ring
        HashSet<RID> member_vehicles;
        for (const TrainsetMember &member: members) {
            member_vehicles.insert(member.vehicle);
        }
        for (const TrainsetMember &member: members) {
            RailVehicleController *controller = _get_controller(*vehicles.getptr(member.vehicle));
            for (const RailVehicleController::CouplerEnd end:
                 {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
                const Ref<RailVehicleController> other = controller->is_coupled(end)
                                                                 ? controller->get_coupled_controller(end)
                                                                 : Ref<RailVehicleController>();
                if (other.is_valid() && member_vehicles.has(other->get_rid())) {
                    controller->uncouple(end);
                }
            }
        }
        // the front of each at the trainset's offset less its gap - none standing reversed - its
        // centre half its length back (DynObj.cpp:2308), the next one behind it
        // (simulationstateserializer.cpp:1066-1076)
        // AttachNext: the front vehicle's end is its rear, the next vehicle's end its front - each
        // the other way round standing reversed (iDirection, DynObj.cpp:1807, 2590); of a pair with
        // one automatic coupler the other end takes the automatic one's adapter, which stands
        // between them (DynObj.cpp:2740-2765)
        const auto ahead_end = [](const TrainsetMember &p_member) {
            return p_member.direction == TrackServer::DIRECTION_REVERSED ? RailVehicleController::COUPLER_END_FRONT
                                                                         : RailVehicleController::COUPLER_END_REAR;
        };
        const auto behind_end = [](const TrainsetMember &p_member) {
            return p_member.direction == TrackServer::DIRECTION_REVERSED ? RailVehicleController::COUPLER_END_REAR
                                                                         : RailVehicleController::COUPLER_END_FRONT;
        };
        Vector<double> adapter_lengths;
        adapter_lengths.resize(members.size());
        adapter_lengths.fill(0.0);
        Vector<bool> adapted;
        adapted.resize(members.size());
        adapted.fill(false);
        for (int index = 1; index < members.size(); ++index) {
            const RailVehicleController *ahead = _get_controller(*vehicles.getptr(members[index - 1].vehicle));
            const RailVehicleController *behind = _get_controller(*vehicles.getptr(members[index].vehicle));
            const bool ahead_automatic = ahead->is_coupler_automatic(ahead_end(members[index - 1]));
            if (ahead_automatic != behind->is_coupler_automatic(behind_end(members[index]))) {
                adapted.set(index, true);
                adapter_lengths.set(
                        index,
                        ahead_automatic ? ahead->get_coupler_adapter_length() : behind->get_coupler_adapter_length());
            }
        }
        double front = trainset->offset;
        for (int index = 0; index < members.size(); ++index) {
            const TrainsetMember &member = members[index];
            const double gap = member.direction == TrackServer::DIRECTION_REVERSED ? 0.0 : member.gap;
            const double length = _get_controller(*vehicles.getptr(member.vehicle))->get_dimensions_length();
            front -= adapter_lengths[index];
            vehicle_set_track(member.vehicle, trainset->track, front - gap - (0.5 * length), member.direction);
            front -= gap + length;
        }
        for (int index = 1; index < members.size(); ++index) {
            const TrainsetMember &ahead = members[index - 1];
            const TrainsetMember &behind = members[index];
            vehicle_couple(ahead.vehicle, ahead_end(ahead), behind.vehicle, behind_end(behind), ahead.coupling);
            if (adapted[index]) {
                RailVehicleController *ahead_controller = _get_controller(*vehicles.getptr(ahead.vehicle));
                if (ahead_controller->is_coupler_automatic(ahead_end(ahead))) {
                    _get_controller(*vehicles.getptr(behind.vehicle))->coupler_adapter_fit(behind_end(behind));
                } else {
                    ahead_controller->coupler_adapter_fit(ahead_end(ahead));
                }
            }
        }
    }

    /* A trainset asked to stand before all its vehicles had their simulation stands now */
    void RailVehicleServer::_on_project_settings_changed() {
        diagnostics = ProjectSettings::get_singleton()->get_setting(DIAGNOSTICS_SETTING, false);
    }

    void RailVehicleServer::_on_vehicle_configured(const RID &p_vehicle) {
        if (VehiclePlacement *placement = vehicles.getptr(p_vehicle); placement != nullptr) {
            _connect_radio(p_vehicle, *placement);
        }
        Vector<RID> pending;
        for (const KeyValue<RID, Trainset> &trainset: trainsets) {
            if (!trainset.value.placement_pending) {
                continue;
            }
            for (const TrainsetMember &member: trainset.value.members) {
                if (member.vehicle == p_vehicle) {
                    pending.push_back(trainset.key);
                    break;
                }
            }
        }
        for (const RID &trainset: pending) {
            trainset_place(trainset);
        }
    }

    Ref<RailVehicleRadio> RailVehicleServer::_get_radio(const VehiclePlacement &p_placement) const {
        return Ref<RailVehicleRadio>(
                Object::cast_to<RailVehicleRadio>(ObjectDB::get_instance(p_placement.radio_component_id)));
    }

    void RailVehicleServer::_connect_radio(const RID &p_vehicle, VehiclePlacement &p_placement) {
        const RailVehicleController *controller = _get_controller(p_placement);
        const Ref<RailVehicleRadio> radio = controller != nullptr
                                                    ? controller->get_component(VehicleComponentType::COMPONENT_RADIO)
                                                    : Ref<VehicleComponent>();
        if (radio.is_null() || ObjectID(radio->get_instance_id()) == p_placement.radio_component_id) {
            return;
        }
        _disconnect_radio(p_vehicle, p_placement);
        p_placement.radio_component_id = ObjectID(radio->get_instance_id());
        radio->connect(
                RailVehicleRadio::radio_stop_sent_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_radio_stop_sent).bind(p_vehicle));
        radio->connect(
                RailVehicleRadio::radio_call_sent_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_radio_call_sent).bind(p_vehicle));
        // a radio of the ether for as long as the vehicle has it, powered or not: one switched on
        // mid-message hears the rest of it, as the original's cab plays every message muted until
        // the radio is on (TTrain::radio_message(), Train.cpp:11010-11029) - the component sends
        // only when powered, and the cab plays only what a powered radio tuned to it hears
        if (RadioServer *radios = RadioServer::get_instance(); radios != nullptr) {
            p_placement.radio = radios->radio_create();
            radios->radio_set_vehicle(p_placement.radio, p_vehicle);
            radios->radio_set_receive_callback(
                    p_placement.radio,
                    callable_mp(this, &RailVehicleServer::_on_vehicle_radio_received).bind(p_vehicle));
        }
    }

    void RailVehicleServer::_disconnect_radio(const RID &p_vehicle, VehiclePlacement &p_placement) {
        if (RadioServer *radios = RadioServer::get_instance(); radios != nullptr && p_placement.radio.is_valid()) {
            radios->radio_free(p_placement.radio);
        }
        p_placement.radio = RID();
        if (const Ref<RailVehicleRadio> radio = _get_radio(p_placement); radio.is_valid()) {
            radio->disconnect(
                    RailVehicleRadio::radio_stop_sent_signal,
                    callable_mp(this, &RailVehicleServer::_on_vehicle_radio_stop_sent).bind(p_vehicle));
            radio->disconnect(
                    RailVehicleRadio::radio_call_sent_signal,
                    callable_mp(this, &RailVehicleServer::_on_vehicle_radio_call_sent).bind(p_vehicle));
        }
        p_placement.radio_component_id = ObjectID();
    }

    // Radio-Stop reaches every vehicle within reach, the sender itself included
    // (basic_region::RadioStop, scene.cpp:1269) - the ether keeps a radio from hearing itself, so the
    // sender is handed its own
    void RailVehicleServer::_on_vehicle_radio_stop_sent(const RID &p_vehicle) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RadioServer *radios = RadioServer::get_instance();
        if (placement == nullptr || radios == nullptr || !placement->radio.is_valid()) {
            return;
        }
        radios->radio_transmit(
                placement->radio, RadioServer::CHANNEL_ALL, RadioServer::RADIO_STOP_REACH,
                RadioServer::KIND_RADIO_STOP);
        _on_vehicle_radio_received(
                RadioServer::KIND_RADIO_STOP, RadioServer::CHANNEL_ALL, Variant(), Vector3(), p_vehicle);
    }

    // A call goes out on the radio's channel, as far as anybody listens (Event.cpp:2255-2268)
    void RailVehicleServer::_on_vehicle_radio_call_sent(const int64_t p_call, const RID &p_vehicle) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RadioServer *radios = RadioServer::get_instance();
        const Ref<RailVehicleRadio> radio = placement != nullptr ? _get_radio(*placement) : Ref<RailVehicleRadio>();
        if (radios == nullptr || radio.is_null() || !placement->radio.is_valid()) {
            return;
        }
        radios->radio_transmit(placement->radio, radio->get_channel(), 0.0, RadioServer::KIND_CALL, p_call);
    }

    // What the vehicle's radio heard is handed down to it, with whether somebody drives the vehicle
    void RailVehicleServer::_on_vehicle_radio_received(
            const int64_t p_kind, const int64_t p_channel, const Variant &p_payload, const Vector3 & /* p_position */,
            const RID &p_vehicle) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const Ref<RailVehicleRadio> radio = placement != nullptr ? _get_radio(*placement) : Ref<RailVehicleRadio>();
        if (vehicle_server == nullptr || radio.is_null()) {
            return;
        }
        const bool driven =
                vehicle_server->vehicle_has_person_role(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
        if (radio->radio_receive(
                    static_cast<RadioServer::Kind>(p_kind), static_cast<int>(p_channel), p_payload, driven) &&
            p_kind == RadioServer::KIND_RADIO_STOP) {
            emit_signal(vehicle_emergency_signal_received_signal, p_vehicle);
        }
    }

    TypedArray<RID> RailVehicleServer::vehicle_get_coupled(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end,
            const BitField<RailVehicleController::CouplingFlags> p_flags) const {
        TypedArray<RID> result;
        // no flag at all joins every end, coupled or not
        ERR_FAIL_COND_V_MSG(static_cast<int64_t>(p_flags) == 0, result, "No coupling to follow.");
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RailVehicleController *first = placement != nullptr ? _get_controller(*placement) : nullptr;
        if (first == nullptr) {
            return result;
        }
        // out through p_end to the last vehicle; entering a neighbour by one end, the walk leaves it
        // by the other, whichever way round it stands
        RailVehicleController::CouplerEnd end = p_end;
        while (first->is_coupled_by(end, p_flags)) {
            const RailVehicleController::CouplerEnd entered = first->get_coupled_end(end);
            first = first->get_coupled_controller(end).ptr();
            end = RailVehicleController::opposite_end(entered);
        }
        // and back, from that end, through every vehicle joined the same way
        RailVehicleController *vehicle = first;
        end = RailVehicleController::opposite_end(end);
        while (vehicle != nullptr) {
            result.push_back(vehicle->get_rid());
            if (!vehicle->is_coupled_by(end, p_flags)) {
                break;
            }
            const RailVehicleController::CouplerEnd entered = vehicle->get_coupled_end(end);
            vehicle = vehicle->get_coupled_controller(end).ptr();
            end = RailVehicleController::opposite_end(entered);
        }
        return result;
    }

    /* TDynamicObject::find_vehicle() (DynObj.h:886-903): this vehicle, then those joined towards its
     * rear, then towards its front - the first that satisfies p_predicate */
    template<typename Predicate>
    static RailVehicleController *find_joined(
            RailVehicleController *p_first, const BitField<RailVehicleController::CouplingFlags> p_flags,
            Predicate p_predicate) {
        if (p_predicate(p_first)) {
            return p_first;
        }
        for (const RailVehicleController::CouplerEnd start:
             {RailVehicleController::COUPLER_END_REAR, RailVehicleController::COUPLER_END_FRONT}) {
            RailVehicleController *vehicle = p_first;
            RailVehicleController::CouplerEnd end = start;
            while (vehicle->is_coupled_by(end, p_flags)) {
                const RailVehicleController::CouplerEnd entered = vehicle->get_coupled_end(end);
                vehicle = vehicle->get_coupled_controller(end).ptr();
                end = RailVehicleController::opposite_end(entered);
                if (p_predicate(vehicle)) {
                    return vehicle;
                }
            }
        }
        return nullptr;
    }

    RID RailVehicleServer::vehicle_find_powered(const RID &p_vehicle) const {
        /* Power > 1.0 (DynObj.cpp:7793) */
        constexpr double POWERED = 1.0;
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RailVehicleController *first = placement != nullptr ? _get_controller(*placement) : nullptr;
        if (first == nullptr) {
            return RID();
        }
        const RailVehicleController::TrainType train_type = first->get_train_type();
        const RailVehicleController::CouplingFlags flag =
                train_type == RailVehicleController::TRAIN_TYPE_EZT ||
                                train_type == RailVehicleController::TRAIN_TYPE_DMU
                        ? RailVehicleController::COUPLING_FLAG_PERMANENT
                        : RailVehicleController::COUPLING_FLAG_CONTROL;
        const RailVehicleController *powered = find_joined(
                first, flag, [](const RailVehicleController *p_vehicle) { return p_vehicle->get_power() > POWERED; });
        return powered != nullptr ? powered->get_rid() : p_vehicle;
    }

    RID RailVehicleServer::vehicle_find_pantograph_carrier(const RID &p_vehicle) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RailVehicleController *first = placement != nullptr ? _get_controller(*placement) : nullptr;
        if (first == nullptr) {
            return RID();
        }
        const auto carries = [](const RailVehicleController *p_vehicle) {
            const Ref<RailVehicleEnginePowerSource> source =
                    p_vehicle->get_rail_component(RailVehicleComponentType::COMPONENT_ENGINE_POWER_SOURCE);
            return source.is_valid() &&
                   source->get_source_type() == RailVehicleController::POWER_SOURCE_CURRENTCOLLECTOR &&
                   source->get_current_collector_number_of_collectors() > 0;
        };
        for (const RailVehicleController::CouplingFlags flag:
             {RailVehicleController::COUPLING_FLAG_PERMANENT, RailVehicleController::COUPLING_FLAG_CONTROL}) {
            if (const RailVehicleController *carrier = find_joined(first, flag, carries); carrier != nullptr) {
                return carrier->get_rid();
            }
        }
        return RID();
    }

    void RailVehicleServer::vehicle_wake(const RID &p_vehicle) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        RailVehicleController *controller = _get_controller(*placement);
        ERR_FAIL_NULL(controller);
        controller->wake();
    }

    Ref<VehicleComponent>
    RailVehicleServer::vehicle_component_get(const RID &p_vehicle, const RailVehicleComponentType::Type p_type) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr ? controller->get_rail_component(p_type) : Ref<VehicleComponent>();
    }

    String RailVehicleServer::vehicle_get_coupler_adapter_model(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr ? controller->get_coupler_adapter_fitted_model(p_end) : String();
    }

    bool RailVehicleServer::vehicle_is_coupler_automatic(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr && controller->is_coupler_automatic(p_end);
    }

    BitField<RailVehicleController::CouplingFlags> RailVehicleServer::vehicle_get_coupler_joinable_flags(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr ? controller->get_coupler_joinable_flags(p_end)
                                     : RailVehicleController::COUPLING_FLAG_NONE;
    }

    double RailVehicleServer::vehicle_get_coupler_adapter_length(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr ? controller->get_coupler_adapter_fitted_length(p_end) : 0.0;
    }

    double RailVehicleServer::vehicle_get_coupler_adapter_height(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        return controller != nullptr ? controller->get_coupler_adapter_fitted_height(p_end) : 0.0;
    }

    void RailVehicleServer::vehicle_couple(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end, const RID &p_other,
            const RailVehicleController::CouplerEnd p_other_end,
            const BitField<RailVehicleController::CouplingFlags> p_coupling) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const VehiclePlacement *other = vehicles.getptr(p_other);
        ERR_FAIL_COND(placement == nullptr || other == nullptr);
        RailVehicleController *controller = _get_controller(*placement);
        RailVehicleController *other_controller = _get_controller(*other);
        ERR_FAIL_COND(controller == nullptr || other_controller == nullptr);
        controller->couple(Ref<RailVehicleController>(other_controller), p_end, p_other_end, p_coupling);
    }

    void RailVehicleServer::_on_vehicle_controller_changed(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (placement == nullptr || vehicle_server == nullptr) {
            return;
        }
        _disconnect_relays(p_vehicle, *placement);
        _disconnect_radio(p_vehicle, *placement);
        placement->controller_id = ObjectID(vehicle_server->vehicle_get_controller_instance_id(p_vehicle));
        // the scans that found it hold the previous controller
        _track_changed(placement->axles[placement->primary_axle].track);
        _connect_relays(p_vehicle, *placement);
        // the axle distance is the new controller's
        _place_axles(p_vehicle, _vehicle_centre(*placement));
        placement = vehicles.getptr(p_vehicle); // the track signals may have rehashed
        // a newly bound controller takes the rail values before its simulation starts
        // (VehicleServer::vehicle_bind_controller() tells this before starting it)
        if (RailVehicleController *controller = _get_controller(*placement); controller != nullptr) {
            controller->set_type_name(placement->type_name);
            controller->set_load_name(placement->load_name);
            controller->set_load_amount(placement->load_amount);
            controller->emit_position_changed_if_needed();
        }
        _hand_driver_cabin_kind(p_vehicle);
    }

    /* The axle distance comes from the wheels' configuration and the vehicle's length, and the
     * body hangs between the axles */
    void RailVehicleServer::_on_vehicle_config_changed(const RID &p_vehicle) {
        if (const VehiclePlacement *placement = vehicles.getptr(p_vehicle); placement != nullptr) {
            _place_axles(p_vehicle, _vehicle_centre(*placement));
        }
    }

    void RailVehicleServer::_connect_relays(const RID &p_vehicle, const VehiclePlacement &p_placement) {
        RailVehicleController *controller = _get_controller(p_placement);
        if (controller == nullptr) {
            return;
        }
        controller->connect(
                RailVehicleController::trainset_changed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_trainset_changed).bind(p_vehicle));
        controller->connect(
                RailVehicleController::derailed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_derailed).bind(p_vehicle));
        controller->connect(
                RailVehicleController::coupler_attached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_attached).bind(p_vehicle));
        controller->connect(
                RailVehicleController::coupler_detached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_detached).bind(p_vehicle));
        controller->connect(
                RailVehicleController::coupler_adapter_attached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_adapter_attached).bind(p_vehicle));
        controller->connect(
                RailVehicleController::coupler_adapter_removed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_adapter_removed).bind(p_vehicle));
        // the vehicle's load commands are the server's operations under its handle
        controller->register_command(
                "load_add", callable_mp(this, &RailVehicleServer::_on_load_add_command).bind(p_vehicle));
        controller->register_command(
                "load_remove", callable_mp(this, &RailVehicleServer::_on_load_remove_command).bind(p_vehicle));
    }

    void RailVehicleServer::_disconnect_relays(const RID &p_vehicle, const VehiclePlacement &p_placement) {
        RailVehicleController *controller = _get_controller(p_placement);
        if (controller == nullptr) {
            return;
        }
        controller->disconnect(
                RailVehicleController::trainset_changed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_trainset_changed).bind(p_vehicle));
        controller->disconnect(
                RailVehicleController::derailed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_derailed).bind(p_vehicle));
        controller->disconnect(
                RailVehicleController::coupler_attached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_attached).bind(p_vehicle));
        controller->disconnect(
                RailVehicleController::coupler_detached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_detached).bind(p_vehicle));
        controller->disconnect(
                RailVehicleController::coupler_adapter_attached_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_adapter_attached).bind(p_vehicle));
        controller->disconnect(
                RailVehicleController::coupler_adapter_removed_signal,
                callable_mp(this, &RailVehicleServer::_on_vehicle_coupler_adapter_removed).bind(p_vehicle));
        controller->unregister_command("load_add");
        controller->unregister_command("load_remove");
    }

    void RailVehicleServer::_on_cabin_person_entered(
            const RID &p_cabin, const RID & /* p_person */, VehiclePersonRole::Role /* p_role */) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        _update_driver_cabin(vehicle_server->cabin_get_vehicle(p_cabin));
    }

    void RailVehicleServer::_on_cabin_person_left(const RID &p_cabin, const RID & /* p_person */) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        _update_driver_cabin(vehicle_server->cabin_get_vehicle(p_cabin));
    }

    void RailVehicleServer::_on_cabin_person_role_changed(
            const RID &p_cabin, const RID & /* p_person */, VehiclePersonRole::Role /* p_role */) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        _update_driver_cabin(vehicle_server->cabin_get_vehicle(p_cabin));
    }

    void
    RailVehicleServer::_on_cabin_person_moved(const RID & /* p_person */, const RID &p_cabin, const RID &p_previous) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        const RID vehicle = vehicle_server->cabin_get_vehicle(p_cabin);
        const RID previous_vehicle = vehicle_server->cabin_get_vehicle(p_previous);
        _update_driver_cabin(vehicle);
        if (previous_vehicle != vehicle) {
            _update_driver_cabin(previous_vehicle);
        }
    }

    /* A cabin taken off the vehicle is no cabin of its kind any more */
    void RailVehicleServer::_on_vehicle_cabin_detached(const RID &p_vehicle, const RID &p_cabin) {
        if (VehiclePlacement *placement = vehicles.getptr(p_vehicle); placement != nullptr) {
            placement->cabin_kinds.erase(p_cabin);
        }
    }

    /* A vehicle left standing has switched its simulation off, and whoever sits down to drive it
     * wakes it */
    void RailVehicleServer::_update_driver_cabin(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        if (placement == nullptr) {
            return;
        }
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        placement->driver = _find_driver(p_vehicle);
        const RID cabin = placement->driver.is_valid() ? vehicle_server->person_get_cabin(placement->driver) : RID();
        if (placement->driver_cabin == cabin) {
            return;
        }
        placement->driver_cabin = cabin;
        _hand_driver_cabin_kind(p_vehicle);
        if (RailVehicleController *controller = _get_controller(*placement);
            controller != nullptr && cabin.is_valid()) {
            controller->wake();
        }
        emit_signal(vehicle_driver_cabin_changed_signal, p_vehicle, cabin);
    }

    /* The controller holds what its occupied cab is (CabOccupied), and only this server tells it -
     * again to every controller the vehicle gets */
    void RailVehicleServer::_hand_driver_cabin_kind(const RID &p_vehicle) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        if (controller != nullptr) {
            controller->set_driver_cabin_kind(cabin_get_kind(placement->driver_cabin));
        }
    }

    void RailVehicleServer::_on_vehicle_trainset_changed(const RID &p_vehicle) {
        emit_signal(vehicle_trainset_changed_signal, p_vehicle);
    }

    void RailVehicleServer::_on_vehicle_derailed(const int64_t p_cause, const RID &p_vehicle) {
        emit_signal(vehicle_derailed_signal, p_vehicle, p_cause);
    }

    void RailVehicleServer::_on_vehicle_coupler_attached(const int64_t p_flag, const RID &p_vehicle) {
        emit_signal(vehicle_coupler_attached_signal, p_vehicle, p_flag);
    }

    void RailVehicleServer::_on_vehicle_coupler_detached(const int64_t p_flag, const RID &p_vehicle) {
        emit_signal(vehicle_coupler_detached_signal, p_vehicle, p_flag);
    }

    void RailVehicleServer::_on_vehicle_coupler_adapter_attached(const int64_t p_end, const RID &p_vehicle) {
        emit_signal(vehicle_coupler_adapter_attached_signal, p_vehicle, p_end);
    }

    void RailVehicleServer::_on_vehicle_coupler_adapter_removed(const int64_t p_end, const RID &p_vehicle) {
        emit_signal(vehicle_coupler_adapter_removed_signal, p_vehicle, p_end);
    }

    TypedArray<RID> RailVehicleServer::vehicle_get_rids_in_rect(const Rect2 &p_rect) const {
        TypedArray<RID> result;
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL_V(tracks, result);
        for (const KeyValue<RID, VehiclePlacement> &entry: vehicles) {
            // a vehicle not standing on a track has no place on the map
            if (!tracks->track_exists(entry.value.axles[entry.value.primary_axle].track)) {
                continue;
            }
            const Vector3 position = entry.value.body_transform.origin;
            if (p_rect.has_point(Vector2(position.x, position.z))) {
                result.push_back(entry.key);
            }
        }
        return result;
    }

    void RailVehicleServer::vehicle_set_track(
            const RID &p_vehicle, const RID &p_track, const double p_track_offset,
            const TrackServer::Direction p_track_direction) {
        if (!vehicles.has(p_vehicle)) {
            return;
        }
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        AxleFollower centre;
        centre.track = p_track;
        centre.track_direction = p_track_direction;
        centre.track_is_switch = tracks->track_is_switch(p_track);
        centre.switch_track = centre.track_is_switch
                                      ? tracks->switch_get_active_track(tracks->track_get_switch(p_track))
                                      : TrackServer::TRACK_COMMON;
        centre.track_offset = CLAMP(p_track_offset, 0.0, tracks->track_get_length(p_track, centre.switch_track));
        const double remaining_offset = p_track_offset - centre.track_offset;
        const double direction_sign = p_track_direction == TrackServer::DIRECTION_NORMAL ? -1.0 : 1.0;
        _walk_axle(centre, remaining_offset * direction_sign, false, nullptr);
        _place_axles(p_vehicle, centre);
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement); // a listener of the track signals freed it
        placement->placement_unreported = false;
        emit_signal(vehicle_placed_signal, p_vehicle);
    }

    void RailVehicleServer::_place_axles(const RID &p_vehicle, const AxleFollower &p_centre) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        const RailVehicleController *controller = _get_controller(*placement);
        const Ref<RailVehicleWheels> wheels =
                controller != nullptr ? controller->get_component(VehicleComponentType::COMPONENT_WHEELS)
                                      : Ref<VehicleComponent>();
        double axle_distance = 0.0;
        if (controller != nullptr && wheels.is_valid()) {
            // fAxleDist = clamp(max(BDist, ADist), 0.2, L - 0.2) (DynObj.cpp:2499-2502); the lower
            // bound wins for a vehicle shorter than both margins, where std::clamp is undefined
            axle_distance = MAX(
                    AXLE_DISTANCE_MARGIN, MIN(MAX(wheels->get_bogie_pivot_spacing(), wheels->get_bogie_axle_spacing()),
                                              controller->get_dimensions_length() - AXLE_DISTANCE_MARGIN));
        }
        const AxleFollower previous[AXLE_COUNT] = {placement->axles[AXLE_FRONT], placement->axles[AXLE_REAR]};
        const RID previous_listed = previous[placement->primary_axle].track;
        placement->axle_distance = axle_distance;
        placement->moved = true;
        placement->location_stale = true;
        placement->placement_unreported = true;
        for (int axle = AXLE_FRONT; axle < AXLE_COUNT; ++axle) {
            placement->axles[axle] = p_centre;
            _walk_axle(
                    placement->axles[axle], AXLE_SIDES[axle] * AXLE_HALF_DISTANCE_FACTOR * axle_distance, false,
                    nullptr);
        }
        const RID axle_tracks[AXLE_COUNT] = {placement->axles[AXLE_FRONT].track, placement->axles[AXLE_REAR].track};
        const RID listed = axle_tracks[placement->primary_axle];
        _place_body(*placement);
        _track_changed(previous_listed);
        _track_changed(listed);
        // counted where each axle stands, not on the tracks it passed to get there: the original
        // counts those too (Move(false) still changes tracks, TrkFoll.cpp:88-91), but at load,
        // before any section has its events (TIsolated::AssignEvents(), Track.cpp:3559) - counted
        // here they would fire :busy and :free of sections the vehicle never stood on. The new
        // track first, as on a move, so a section both are in never reads empty.
        for (int axle = AXLE_FRONT; axle < AXLE_COUNT; ++axle) {
            tracks->track_axle_entered(axle_tracks[axle], p_vehicle);
            tracks->track_axle_left(previous[axle].track, p_vehicle);
        }
        tracks->track_vehicle_left(previous_listed, p_vehicle);
        tracks->track_vehicle_entered(listed, p_vehicle);
    }

    RailVehicleServer::AxleFollower RailVehicleServer::_vehicle_centre(const VehiclePlacement &p_placement) {
        AxleFollower centre = p_placement.axles[AXLE_REAR];
        _walk_axle(
                centre, AXLE_SIDES[AXLE_FRONT] * AXLE_HALF_DISTANCE_FACTOR * p_placement.axle_distance, false, nullptr);
        return centre;
    }

    void RailVehicleServer::vehicle_move(const RID &p_vehicle, const double p_distance) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const TrackServer *tracks = TrackServer::get_instance();
        if (placement == nullptr || tracks == nullptr ||
            !tracks->track_exists(placement->axles[placement->primary_axle].track)) {
            return;
        }
        _move_vehicle(p_vehicle, p_distance);
        placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement); // a listener of the track signals freed it
        if (RailVehicleController *controller = _get_controller(*placement); controller != nullptr) {
            controller->emit_position_changed_if_needed();
        }
        vehicle_report_placement(p_vehicle);
    }

    void RailVehicleServer::_move_vehicle(const RID &p_vehicle, const double p_distance) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        if (Math::is_zero_approx(p_distance)) {
            return;
        }
        // the primary axle changes only while both stand on one track, so the vehicle is never
        // listed on a track none of its axles is on; it is the second axle in the direction of
        // travel, so the AI does not clear the signal it passes too early (DynObj.cpp:2645-2655:
        // "żeby AI nie wyłączało sobie semafora za szybko")
        if (const RailVehicleController *controller = _get_controller(*placement);
            controller != nullptr && placement->axles[AXLE_FRONT].track == placement->axles[AXLE_REAR].track &&
            controller->get_speed() > PRIMARY_AXLE_SWITCH_SPEED) {
            placement->primary_axle = controller->get_velocity() >= 0.0 ? AXLE_REAR : AXLE_FRONT;
        }
        placement->moved = true;
        placement->location_stale = true;
        placement->placement_unreported = true;
        _track_changed(placement->axles[placement->primary_axle].track);
        // the leading axle first (DynObj.cpp:2656-2670) - it sets a switch on the way for the one
        // behind; this server's distance counts towards the rear, so a positive one leads with
        // the rear axle. The trailing axle goes as far as the leading one went, so the two stay
        // the axle distance apart where the leading one stopped at a track's end.
        const Axle leading = p_distance > 0.0 ? AXLE_REAR : AXLE_FRONT;
        const Axle trailing = leading == AXLE_REAR ? AXLE_FRONT : AXLE_REAR;
        LocalVector<TrackCrossing> crossings;
        const double walked = _walk_axle(placement->axles[leading], p_distance, true, &crossings);
        _report_crossings(p_vehicle, leading, crossings);
        placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement); // a listener of the track signals freed it
        crossings.clear();
        _walk_axle(placement->axles[trailing], walked, true, &crossings);
        _report_crossings(p_vehicle, trailing, crossings);
        placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        _place_body(*placement);
        const RID listed = placement->axles[placement->primary_axle].track;
        _track_changed(listed);
        // a moving vehicle that turned back on its track: the events of its new heading
        // (TrkFoll.cpp:136-161 - a track's event1 and event2 each once, whichever comes)
        const RailVehicleController *controller = _get_controller(*placement);
        if (controller == nullptr || controller->get_speed() <= STANDING_SPEED) {
            return;
        }
        const TrackHeading heading =
                placement->axles[placement->primary_axle].travel_sign > 0.0 ? HEADING_TO_END : HEADING_TO_START;
        if (heading == placement->reported_heading && listed == placement->reported_track) {
            return;
        }
        placement->reported_heading = heading;
        placement->reported_track = listed;
        emit_signal(
                heading == HEADING_TO_END ? vehicle_heading_to_track_end_signal : vehicle_heading_to_track_start_signal,
                p_vehicle, listed);
    }

    /* Every axle counts on every track it comes onto, the primary one also moves the vehicle
     * between the tracks' lists and fires their events (TTrackFollower::Move(), TrkFoll.cpp:88-91,
     * 112-161, 194-241) - track by track, a track shorter than the step's travel included */
    void RailVehicleServer::_report_crossings(
            const RID &p_vehicle, const Axle p_axle, const LocalVector<TrackCrossing> &p_crossings) {
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        for (const TrackCrossing &crossing: p_crossings) {
            // the new track first, so a section both belong to never reads empty
            tracks->track_axle_entered(crossing.entered, p_vehicle);
            tracks->track_axle_left(crossing.left, p_vehicle);
            VehiclePlacement *placement = vehicles.getptr(p_vehicle);
            if (placement == nullptr || !(placement->primary_axle == p_axle)) {
                continue; // freed by a listener, or an axle that only counts
            }
            tracks->track_vehicle_left(crossing.left, p_vehicle);
            tracks->track_vehicle_entered(crossing.entered, p_vehicle);
            _track_changed(crossing.left);
            _track_changed(crossing.entered);
            // only a moving vehicle fires a track's event1 and event2 (ismoving, TrkFoll.cpp:104)
            const RailVehicleController *controller = _get_controller(*placement);
            if (controller == nullptr || controller->get_speed() <= STANDING_SPEED) {
                continue;
            }
            const TrackHeading heading = crossing.travel_sign > 0.0 ? HEADING_TO_END : HEADING_TO_START;
            placement->reported_heading = heading;
            placement->reported_track = crossing.entered;
            emit_signal(
                    heading == HEADING_TO_END ? vehicle_heading_to_track_end_signal
                                              : vehicle_heading_to_track_start_signal,
                    p_vehicle, crossing.entered);
        }
    }

    /* The original moves every vehicle by the distance times its own DirectionGet(), the way the
     * trainset drives; here the way is the given vehicle's front, and each other vehicle's sign
     * says whether it stands the same way round. The leading vehicle goes first, so that a switch
     * on the way is set once for all of them. The couplers need nothing: the next sub-step
     * refreshes every location and neighbour before any force. */
    template<typename Answers>
    bool RailVehicleServer::_trainset_any_doors(
            const RID &p_vehicle, const RailVehicleDoors::Side p_side, Answers p_answers) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RailVehicleController *first = placement != nullptr ? _get_controller(*placement) : nullptr;
        if (first == nullptr) {
            return false;
        }
        const RailVehicleDoors::Side other_side =
                p_side == RailVehicleDoors::SIDE_LEFT ? RailVehicleDoors::SIDE_RIGHT : RailVehicleDoors::SIDE_LEFT;
        const auto answers = [&](const RailVehicleController *p_controller, const bool p_same_way) {
            const RailVehicleDoors *doors = Object::cast_to<RailVehicleDoors>(
                    p_controller->get_component(VehicleComponentType::COMPONENT_DOORS).ptr());
            return doors != nullptr && p_answers(*doors, p_same_way ? p_side : other_side);
        };
        if (answers(first, true)) {
            return true;
        }
        // out through each end; a neighbour entered by the same end as the one left stands the other
        // way round
        for (const RailVehicleController::CouplerEnd start:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            const RailVehicleController *vehicle = first;
            RailVehicleController::CouplerEnd end = start;
            bool same_way = true;
            while (vehicle->is_coupled_by(end, RailVehicleController::COUPLING_FLAG_COUPLER)) {
                const RailVehicleController::CouplerEnd entered = vehicle->get_coupled_end(end);
                same_way = same_way == (entered != end);
                vehicle = vehicle->get_coupled_controller(end).ptr();
                end = RailVehicleController::opposite_end(entered);
                if (vehicle == first) {
                    break;
                }
                if (answers(vehicle, same_way)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool RailVehicleServer::trainset_get_doorway_open(const RID &p_vehicle, const RailVehicleDoors::Side p_side) const {
        return _trainset_any_doors(
                p_vehicle, p_side, [](const RailVehicleDoors &p_doors, const RailVehicleDoors::Side p_at) {
                    return p_doors.get_close_method() != RailVehicleDoors::CONTROLS_AUTOMATIC &&
                           !(p_at == RailVehicleDoors::SIDE_LEFT ? p_doors.get_left_closed()
                                                                 : p_doors.get_right_closed());
                });
    }

    bool RailVehicleServer::trainset_get_door_open(const RID &p_vehicle, const RailVehicleDoors::Side p_side) const {
        return _trainset_any_doors(
                p_vehicle, p_side, [](const RailVehicleDoors &p_doors, const RailVehicleDoors::Side p_at) {
                    return p_doors.get_close_method() != RailVehicleDoors::CONTROLS_AUTOMATIC &&
                           !(p_at == RailVehicleDoors::SIDE_LEFT ? p_doors.get_left_door_closed()
                                                                 : p_doors.get_right_door_closed());
                });
    }

    bool RailVehicleServer::trainset_get_door_permit(const RID &p_vehicle, const RailVehicleDoors::Side p_side) const {
        return _trainset_any_doors(
                p_vehicle, p_side, [](const RailVehicleDoors &p_doors, const RailVehicleDoors::Side p_at) {
                    return p_doors.get_permit_required() &&
                           (p_at == RailVehicleDoors::SIDE_LEFT ? p_doors.get_left_open_permit()
                                                                : p_doors.get_right_open_permit());
                });
    }

    void RailVehicleServer::trainset_determine_type(const RID &p_vehicle) {
        const TypedArray<RID> trainset = vehicle_get_coupled(
                p_vehicle, RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLING_FLAG_COUPLER);
        bool passenger = false;
        bool cargo = false;
        for (const Variant &member: trainset) {
            const VehiclePlacement *placement = vehicles.getptr(member);
            const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
            // a car is a vehicle without power (Power < 1, Driver.cpp:2156)
            if (controller == nullptr || controller->get_power() >= 1.0) {
                continue;
            }
            const Ref<RailVehicleBrake> brake =
                    controller->get_rail_component(RailVehicleComponentType::COMPONENT_BRAKES);
            const int delays = brake.is_valid() ? brake->get_cntrl_brake_delays() : RailVehicleBrake::BRAKE_DELAY_NONE;
            if ((delays & RailVehicleBrake::BRAKE_DELAY_G) != 0 && (delays & RailVehicleBrake::BRAKE_DELAY_R) == 0) {
                cargo = true;
            } else {
                passenger = true;
            }
        }
        TrainsetType type = TRAINSET_TYPE_NONE;
        if (passenger && cargo) {
            type = TRAINSET_TYPE_MIXED;
        } else if (cargo) {
            type = TRAINSET_TYPE_CARGO;
        } else if (passenger) {
            type = TRAINSET_TYPE_PASSENGER;
        }
        for (const Variant &member: trainset) {
            if (VehiclePlacement *placement = vehicles.getptr(member)) {
                placement->trainset_type = type;
            }
        }
    }

    RailVehicleServer::TrainsetType RailVehicleServer::trainset_get_type(const RID &p_vehicle) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        return placement != nullptr ? placement->trainset_type : TRAINSET_TYPE_NONE;
    }

    void RailVehicleServer::trainset_move(const RID &p_vehicle, const double p_distance) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        if (controller == nullptr) {
            return;
        }
        // per end of the vehicle, its neighbours outwards and the sign of the distance each moves
        Vector<RID> sides[2];
        Vector<double> signs[2];
        for (const RailVehicleController::CouplerEnd side:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            RailVehicleController *vehicle = controller;
            RailVehicleController::CouplerEnd end = side;
            double sign = 1.0;
            while (vehicle->is_coupled_by(end, RailVehicleController::COUPLING_FLAG_COUPLER)) {
                const RailVehicleController::CouplerEnd entered = vehicle->get_coupled_end(end);
                vehicle = vehicle->get_coupled_controller(end).ptr();
                // entered by the same end it was left by: that neighbour stands the other way round
                if (entered == end) {
                    sign = -sign;
                }
                end = RailVehicleController::opposite_end(entered);
                sides[side].push_back(vehicle->get_rid());
                signs[side].push_back(sign);
            }
        }
        // the side the trainset moves towards leads, from its far end in; vehicle_move() measures
        // towards the rear, as vehicle_process_movement() does
        const RailVehicleController::CouplerEnd leading =
                p_distance > 0.0 ? RailVehicleController::COUPLER_END_FRONT : RailVehicleController::COUPLER_END_REAR;
        const RailVehicleController::CouplerEnd trailing = RailVehicleController::opposite_end(leading);
        for (int index = static_cast<int>(sides[leading].size() - 1); index >= 0; index--) {
            vehicle_move(sides[leading][index], -p_distance * signs[leading][index]);
        }
        vehicle_move(p_vehicle, -p_distance);
        for (int index = 0; index < sides[trailing].size(); index++) {
            vehicle_move(sides[trailing][index], -p_distance * signs[trailing][index]);
        }
    }

    double RailVehicleServer::_walk_axle(
            AxleFollower &p_axle, const double p_distance, const bool p_force_switch_state,
            LocalVector<TrackCrossing> *p_crossings) {
        if (Math::is_zero_approx(p_distance)) {
            return 0.0;
        }
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL_V(tracks, 0.0);

        RID current_track = p_axle.track;
        TrackServer::SwitchTrack current_switch_track = p_axle.switch_track;
        bool current_is_switch = p_axle.track_is_switch;
        // Length of the occupied branch, kept across the loop: it used to be asked twice for the
        // same track on every step.
        double current_length = tracks->track_get_length(current_track, current_switch_track);
        double current_track_offset = CLAMP(p_axle.track_offset, 0.0, current_length);
        TrackServer::Direction current_track_direction = p_axle.track_direction;
        bool beyond_track_end = false;
        double remaining = Math::abs(p_distance);
        const double request_sign = p_distance < 0.0 ? -1.0 : 1.0;
        Vector3 start_point;
        if (diagnostics && p_force_switch_state) {
            const Ref<Curve3D> curve = tracks->track_get_domain_curve(current_track, current_switch_track);
            if (curve.is_valid()) {
                start_point = curve->sample_baked(static_cast<real_t>(current_track_offset), false);
            }
        }
        // Convert movement relative to the vehicle front into curve offset movement. Positive sign
        // moves toward the branch end, negative toward the branch start.
        double movement_sign = (current_track_direction == TrackServer::DIRECTION_NORMAL ? -1.0 : 1.0) * request_sign;

        while (remaining > MOVEMENT_EPSILON) {
            // track_get_length() returns 0 for a track that is gone, so this covers track_exists()
            if (current_length <= 0.0) {
                break;
            }

            // The endpoint this movement heads toward on the occupied branch.
            const double distance_to_endpoint =
                    movement_sign > 0.0 ? current_length - current_track_offset : current_track_offset;
            int endpoint_index = 0;
            if (current_is_switch) {
                endpoint_index = movement_sign > 0.0
                                         ? tracks->track_get_branch_end_endpoint(current_track, current_switch_track)
                                         : tracks->track_get_branch_start_endpoint(current_track, current_switch_track);
                // Reversing through a switch blade on a non-active branch keeps the branch the
                // vehicle already occupies and forces the switch back.
                const RID current_switch = tracks->track_get_switch(current_track);
                if (tracks->switch_get_active_track(current_switch) != current_switch_track) {
                    const double requested_distance = MIN(remaining, distance_to_endpoint);
                    const double next_offset_on_track = current_track_offset + (movement_sign * requested_distance);
                    const double blade_boundary_offset =
                            tracks->track_get_blade_boundary_offset(current_track, current_switch_track);
                    if (p_force_switch_state && current_track_offset > blade_boundary_offset &&
                        next_offset_on_track <= blade_boundary_offset) {
                        tracks->switch_set_active_track(current_switch, current_switch_track);
                    }
                }
            } else {
                endpoint_index = movement_sign > 0.0 ? TrackServer::CURVE1_P2 : TrackServer::CURVE1_P1;
            }

            // Hot path: an ordinary step stays within the current branch and never asks topology
            // for the next track.
            if (remaining <= distance_to_endpoint) {
                current_track_offset += movement_sign * remaining;
                remaining = 0.0;
                break;
            }

            current_track_offset = movement_sign > 0.0 ? current_length : 0.0;
            remaining -= distance_to_endpoint;

            // Large init/debug jumps cross endpoints by following the single unambiguous
            // connection. An ambiguous node stops the movement at the endpoint.
            RID next_track;
            int next_endpoint = 0;
            if (!_motion_connection(current_track, endpoint_index, p_force_switch_state, next_track, next_endpoint)) {
                // nothing connected at all, not an ambiguous node: the vehicle runs off the rails
                beyond_track_end = tracks->track_get_endpoint_connections(
                                                 current_track, static_cast<TrackServer::EndpointIndex>(endpoint_index))
                                           .is_empty();
                break;
            }

            const RID left_track = current_track;
            current_track = next_track;
            current_is_switch = tracks->track_is_switch(current_track);
            // The connection endpoint is where the vehicle enters the next track; on a switch it
            // also says which branch it now occupies.
            if (current_is_switch) {
                current_switch_track = tracks->track_get_endpoint_branch(
                        current_track, static_cast<TrackServer::EndpointIndex>(next_endpoint));
                const bool entered_at_end =
                        next_endpoint == tracks->track_get_branch_end_endpoint(current_track, current_switch_track);
                movement_sign = entered_at_end ? -1.0 : 1.0;
            } else {
                current_switch_track = TrackServer::TRACK_COMMON;
                movement_sign = next_endpoint == TrackServer::CURVE1_P2 ? -1.0 : 1.0;
            }
            // Entering at the branch start means the offset grows; entering at the branch end
            // means it decreases from the branch length.
            current_length = tracks->track_get_length(current_track, current_switch_track);
            current_track_offset = movement_sign > 0.0 ? 0.0 : current_length;
            current_track_direction = movement_sign * request_sign < 0.0 ? TrackServer::DIRECTION_NORMAL
                                                                         : TrackServer::DIRECTION_REVERSED;
            if (p_crossings != nullptr) {
                p_crossings->push_back({left_track, current_track, movement_sign});
            }
        }

        p_axle.track = current_track;
        p_axle.track_is_switch = current_is_switch;
        p_axle.travel_sign = movement_sign;
        p_axle.track_offset = current_track_offset;
        p_axle.track_direction = current_track_direction;
        p_axle.switch_track = current_switch_track;
        p_axle.beyond_track_end = beyond_track_end;
        const double walked = Math::abs(p_distance) - remaining;
        if (diagnostics && p_force_switch_state) {
            _check_movement(p_axle, start_point, walked);
        }
        return request_sign * walked;
    }

    /* Diagnostics: an axle must move in the world by the distance it moved along the track (a
     * step is far shorter than any curve radius, so the chord equals the arc) - a mismatch shifts
     * the simulated location and kicks the coupled vehicles. */
    void
    RailVehicleServer::_check_movement(const AxleFollower &p_axle, const Vector3 &p_start, const double p_moved) const {
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        const Ref<Curve3D> curve = tracks->track_get_domain_curve(p_axle.track, p_axle.switch_track);
        if (curve.is_null()) {
            return;
        }
        const Vector3 end_point = curve->sample_baked(static_cast<real_t>(p_axle.track_offset), false);
        const double world_moved = p_start.distance_to(end_point);
        if (Math::abs(world_moved - p_moved) > DIAGNOSTICS_MOVE_TOLERANCE) {
            UtilityFunctions::push_error(
                    vformat("RailVehicleServer: moved %.4f m in the world instead of %.4f m on track %s (offset %.3f)",
                            world_moved, p_moved, tracks->track_get_name(p_axle.track), p_axle.track_offset));
        }
    }

    /* Where the vehicle's body is: hung between its two axles, as the original computes it
     * (DynObj.cpp:2671-2716) */
    Transform3D RailVehicleServer::vehicle_get_transform(const RID &p_vehicle) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const TrackServer *tracks = TrackServer::get_instance();
        if (placement == nullptr || tracks == nullptr ||
            !tracks->track_exists(placement->axles[placement->primary_axle].track)) {
            return Transform3D();
        }
        return placement->body_transform;
    }

    /* Composed once where the axles move, and handed down to the controller as the vehicle's
     * world transform - the state its owner hands down, as the driver cabin kind - with the track
     * under it for its physics */
    void RailVehicleServer::_place_body(VehiclePlacement &p_placement) {
        const TrackServer *tracks = TrackServer::get_instance();
        RailVehicleController *controller = _get_controller(p_placement);
        const AxleFollower &front = p_placement.axles[AXLE_FRONT];
        const AxleFollower &rear = p_placement.axles[AXLE_REAR];
        const AxleFollower &primary = p_placement.axles[p_placement.primary_axle];
        const bool on_track = tracks != nullptr && tracks->track_exists(primary.track);
        Transform3D body;
        if (on_track) {
            p_placement.axle_transforms[AXLE_FRONT] = _axle_transform(front);
            p_placement.axle_transforms[AXLE_REAR] = _axle_transform(rear);
            // the axles' cants, each signed for the way it stands on its track, and the body
            // leaning by their mean (Roll() * 0.5, DynObj.cpp:2690; ts.dHrail, :3167)
            const double cant = Math::deg_to_rad(AXLE_ROLL_MEAN_FACTOR * (_axle_roll(front) + _axle_roll(rear)));
            const Vector3 front_origin = p_placement.axle_transforms[AXLE_FRONT].origin;
            const Vector3 rear_origin = p_placement.axle_transforms[AXLE_REAR].origin;
            Vector3 body_forward = front_origin - rear_origin;
            // axles at one point - a vehicle whose spacings are unknown yet - is the track there
            body = p_placement.axle_transforms[AXLE_FRONT];
            if (!body_forward.is_zero_approx()) {
                // vFront between the outer axles, vLeft level, vUp square to both, then the
                // cant about vFront; the centre midway between the axles (DynObj.cpp:2673-2697).
                // The original also takes fAdjustment = |vFront| - fAxleDist here (:2677) and uses
                // it nowhere (its uses, :2659-2684, are commented out): not ported.
                body_forward.normalize();
                Vector3 reference_up(0.0, 1.0, 0.0);
                if (Math::abs(body_forward.dot(reference_up)) > UP_PARALLEL_DOT_LIMIT) {
                    reference_up = Vector3(1.0, 0.0, 0.0);
                }
                const Vector3 z_axis = -body_forward;
                const Vector3 x_axis = reference_up.cross(z_axis).normalized();
                const Vector3 y_axis = z_axis.cross(x_axis).normalized();
                body = Transform3D(
                        Basis(x_axis, y_axis, z_axis).orthonormalized().rotated(body_forward, cant).orthonormalized(),
                        (front_origin + rear_origin) * AXLE_MIDPOINT_FACTOR);
            }
            // ts.R from the yaw difference of the axles - over the pivot spacing (BDist), not
            // the axle distance it was measured over, as the original divides it
            // (DynObj.cpp:3145-3161, MASZYNA_ORIGINAL_QUIRKS.md)
            const Vector3 front_heading = -p_placement.axle_transforms[AXLE_FRONT].basis.get_column(2);
            const Vector3 rear_heading = -p_placement.axle_transforms[AXLE_REAR].basis.get_column(2);
            // yaw as the original turns its models, atan2(-x, z) (modelRot.z, DynObj.cpp:2687)
            const double yaw_difference = Math::wrapf(
                    Math::atan2(-front_heading.x, front_heading.z) - Math::atan2(-rear_heading.x, rear_heading.z),
                    -Math::PI, Math::PI);
            const Ref<RailVehicleWheels> wheels =
                    controller != nullptr ? controller->get_component(VehicleComponentType::COMPONENT_WHEELS)
                                          : Ref<VehicleComponent>();
            const double pivot_spacing = wheels.is_valid() ? wheels->get_bogie_pivot_spacing() : 0.0;
            double radius = 0.0;
            if (!Math::is_zero_approx(Math::sin(yaw_difference * CURVE_RADIUS_HALF_FACTOR))) {
                radius = -CURVE_RADIUS_HALF_FACTOR * pivot_spacing /
                         Math::sin(yaw_difference * CURVE_RADIUS_HALF_FACTOR);
            }
            if (Math::abs(radius) > CURVE_RADIUS_LIMIT) {
                radius = 0.0;
            }
            p_placement.track_curve = Vector2(radius, cant);
        } else {
            p_placement.axle_transforms[AXLE_FRONT] = body;
            p_placement.axle_transforms[AXLE_REAR] = body;
            p_placement.track_curve = Vector2();
        }
        if (controller != nullptr) {
            // a derailed body beside its track, across it on the level, and below it
            // (TDynamicObject::Move(), DynObj.cpp:2706-2709)
            const Vector2 offset = controller->get_body_offset();
            const Vector3 left = -body.basis.get_column(0);
            body.origin += Vector3(left.x, 0.0, left.z) * offset.x + Vector3(0.0, offset.y, 0.0);
        }
        p_placement.body_transform = body;
        if (controller == nullptr) {
            return;
        }
        controller->set_world_transform(body);
        if (!on_track) {
            return;
        }
        // the track under it for its physics, as TDynamicObject::Update() hands it to the Mover
        // (DynObj.cpp:3145-3240): the curve between the axles, the rise towards the front
        // (ts.dHtrack = -vFront.y), and the gauge, friction, kind and damage of MyTrack - the
        // primary axle's track
        RailVehicleController::TrackConditions conditions;
        conditions.radius = p_placement.track_curve.x;
        conditions.cant = p_placement.track_curve.y;
        conditions.grade = body.basis.get_column(2).y;
        conditions.gauge = tracks->track_get_width(primary.track);
        // times the scenery's multiplier and the weather's (tp.friction, DynObj.cpp:3176)
        const SimulationServer *simulation = SimulationServer::get_instance();
        conditions.friction = tracks->track_get_friction(primary.track) *
                              (simulation != nullptr ? simulation->get_friction() *
                                                               simulation->simulation_get_weather_friction_factor()
                                                     : 1.0);
        conditions.group = tracks->track_get_group(primary.track);
        conditions.damage = tracks->track_get_damage_flag(primary.track);
        // either axle run off a track's end has the vehicle off the rails - the leading one does
        // first, while the primary one is the trailing one (TTrack::NullCreate(), TrkFoll.cpp:79-83)
        if (front.beyond_track_end || rear.beyond_track_end) {
            conditions.damage.set_flag(TrackServer::TRACK_DAMAGE_NO_RAIL);
        }
        controller->set_track_conditions(conditions);
    }

    Transform3D
    RailVehicleServer::vehicle_get_bogie_transform(const RID &p_vehicle, const RailVehicleWheels::Bogie p_bogie) const {
        // the bogie comes from scripts as a bare int
        ERR_FAIL_INDEX_V(p_bogie, AXLE_COUNT, Transform3D());
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        // BOGIE_FRONT is the front axle's, BOGIE_REAR the rear one's
        return placement != nullptr ? placement->axle_transforms[p_bogie] : Transform3D();
    }

    Transform3D RailVehicleServer::_axle_transform(const AxleFollower &p_axle) const {
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL_V(tracks, Transform3D());
        const Ref<Curve3D> curve = tracks->track_get_domain_curve(p_axle.track, p_axle.switch_track);
        if (curve.is_null()) {
            return Transform3D();
        }

        const double length = curve->get_baked_length();
        const double safe_offset = CLAMP(p_axle.track_offset, 0.0, length);
        // linear on purpose: the cubic interpolation has no neighbour point at the curve ends, so
        // the position advanced there only about 60% of the offset - every vehicle lost
        // centimetres at each track joint, which kicked the trainset through its couplers
        const Vector3 origin = curve->sample_baked(static_cast<real_t>(safe_offset), false);
        const double sample_distance = MIN(HEADING_SAMPLE_DISTANCE, length);
        double previous_offset = CLAMP(safe_offset - sample_distance, 0.0, length);
        double next_offset = CLAMP(safe_offset + sample_distance, 0.0, length);
        if (Math::is_equal_approx(previous_offset, next_offset)) {
            previous_offset = 0.0;
            next_offset = length;
        }
        Vector3 forward = curve->sample_baked(static_cast<real_t>(next_offset), false) -
                          curve->sample_baked(static_cast<real_t>(previous_offset), false);
        if (forward.length_squared() <= HEADING_MIN_LENGTH_SQUARED) {
            forward = Vector3(0.0, 0.0, -1.0);
        } else {
            forward = forward.normalized();
        }

        Vector3 reference_up(0.0, 1.0, 0.0);
        if (Math::abs(forward.dot(reference_up)) > UP_PARALLEL_DOT_LIMIT) {
            reference_up = Vector3(1.0, 0.0, 0.0);
        }

        const Vector3 z_axis = -forward;
        const Vector3 x_axis = reference_up.cross(z_axis).normalized();
        const Vector3 y_axis = z_axis.cross(x_axis).normalized();
        const Vector2 rolls = tracks->track_get_roll(p_axle.track, p_axle.switch_track);
        const double roll =
                length <= 0.0 ? rolls.x : Math::lerp(rolls.x, rolls.y, CLAMP(safe_offset / length, 0.0, 1.0));
        Transform3D track_transform(
                Basis(x_axis, y_axis, z_axis)
                        .orthonormalized()
                        .rotated(forward, static_cast<real_t>(Math::deg_to_rad(roll)))
                        .orthonormalized(),
                origin);
        if (p_axle.track_direction == TrackServer::DIRECTION_REVERSED) {
            track_transform.basis =
                    track_transform.basis.rotated(track_transform.basis.get_column(1).normalized(), Math::PI)
                            .orthonormalized();
        }
        track_transform.origin.y += static_cast<real_t>(tracks->get_rail_height());
        return track_transform;
    }

    double RailVehicleServer::_axle_roll(const AxleFollower &p_axle) const {
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL_V(tracks, 0.0);
        const double length = tracks->track_get_length(p_axle.track, p_axle.switch_track);
        if (length <= 0.0) {
            return 0.0;
        }
        const Vector2 rolls = tracks->track_get_roll(p_axle.track, p_axle.switch_track);
        const double roll = Math::lerp(
                static_cast<double>(rolls.x), static_cast<double>(rolls.y),
                CLAMP(p_axle.track_offset / length, 0.0, 1.0));
        // a track run backwards has its roll the other way (TTrackFollower::ComputatePosition(),
        // TrkFoll.cpp:270-275)
        return p_axle.track_direction == TrackServer::DIRECTION_REVERSED ? -roll : roll;
    }

    Dictionary RailVehicleServer::vehicle_get_track_position(const RID &p_vehicle) const {
        Dictionary result;
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const TrackServer *tracks = TrackServer::get_instance();
        const AxleFollower *primary = placement != nullptr ? &placement->axles[placement->primary_axle] : nullptr;
        if (primary == nullptr || tracks == nullptr || !tracks->track_exists(primary->track)) {
            result["track_rid"] = RID();
            result["along"] = 0.0;
            return result;
        }
        result["track_rid"] = primary->track;
        // moving forward decreases the offset on a track run in its normal direction
        result["along"] = primary->track_direction == TrackServer::DIRECTION_NORMAL ? -primary->track_offset
                                                                                    : primary->track_offset;
        return result;
    }

    TypedArray<TrackRouteSegment>
    RailVehicleServer::vehicle_trace_route(const RID &p_vehicle, const int p_direction, const double p_distance) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        TrackServer *tracks = TrackServer::get_instance();
        const AxleFollower *primary = placement != nullptr ? &placement->axles[placement->primary_axle] : nullptr;
        if (primary == nullptr || tracks == nullptr || !tracks->track_exists(primary->track)) {
            return TypedArray<TrackRouteSegment>();
        }
        const double length = tracks->track_get_length(primary->track, primary->switch_track);
        const double offset = CLAMP(primary->track_offset, 0.0, length);
        // as _walk_axle(): positive moves toward the branch end. Its distance counts from the
        // rear, the vehicle's front (the mover's V > 0) is its negative (vehicle_process_movement())
        const bool toward_end = (primary->track_direction == TrackServer::DIRECTION_NORMAL) == (p_direction >= 0);
        // the track it stands on is entered behind it; its branch is where it stands, not a setting
        const double start = (toward_end ? length - offset : offset) - length;
        return tracks->track_trace_route(primary->track, primary->switch_track, false, toward_end, start, p_distance);
    }

    Ref<RailVehicleNeighbour> RailVehicleServer::vehicle_find_vehicle(
            const RID &p_vehicle, const RailVehicleController::CouplerEnd p_end, const double p_distance) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const TrackServer *tracks = TrackServer::get_instance();
        if (placement == nullptr || tracks == nullptr ||
            !tracks->track_exists(placement->axles[placement->primary_axle].track)) {
            return Ref<RailVehicleNeighbour>();
        }
        RID found;
        RailVehicleController::CouplerEnd found_end = RailVehicleController::COUPLER_END_FRONT;
        double found_distance = 0.0;
        LocalVector<RID> scanned_tracks;
        if (!_find_vehicle(
                    p_vehicle, *placement, p_end, p_distance, found, found_end, found_distance, scanned_tracks)) {
            return Ref<RailVehicleNeighbour>();
        }
        Ref<RailVehicleNeighbour> neighbour;
        neighbour.instantiate();
        neighbour->set_vehicle_rid(found);
        neighbour->set_end(found_end);
        neighbour->set_distance(found_distance);
        return neighbour;
    }

    Dictionary RailVehicleServer::vehicle_get_curve(const RID &p_vehicle) const {
        Dictionary result;
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const Vector2 curve = placement != nullptr ? placement->track_curve : Vector2();
        result["radius"] = curve.x;
        result["cant"] = curve.y;
        return result;
    }

    /* The next track (TrackServer::track_find_next()); entering a switch from a branch side
     * physically selects that branch, so it is forced here - after the connection was proven
     * unique, so an ambiguous node cannot change switch state as a side effect. */
    bool RailVehicleServer::_motion_connection(
            const RID &p_track, const int p_endpoint_index, const bool p_force_switch_state, RID &p_track_out,
            int &p_endpoint_out) {
        TrackServer *tracks = TrackServer::get_instance();
        if (tracks == nullptr) {
            return false;
        }
        int forced_switch_track = TrackServer::NO_FORCED_SWITCH_TRACK;
        if (!tracks->track_find_next(p_track, p_endpoint_index, p_track_out, p_endpoint_out, forced_switch_track)) {
            return false;
        }
        if (!p_force_switch_state || forced_switch_track == TrackServer::NO_FORCED_SWITCH_TRACK) {
            return true;
        }
        const RID entered_switch = tracks->track_get_switch(p_track_out);
        const auto forced = static_cast<TrackServer::SwitchTrack>(forced_switch_track);
        if (!(tracks->switch_get_active_track(entered_switch) == forced)) {
            tracks->switch_set_active_track(entered_switch, forced);
        }
        return true;
    }

    /* The whole step of every registered vehicle, in the phase order of the original's
     * vehicle_table::update() (DynObj.cpp:8686-8724): locations and neighbours once, then the
     * forces of all before the movement of all in each sub-iteration.
     *
     * It runs on the rendered frame, not on Godot's fixed tick, exactly like the original - on the
     * fixed tick the same step ran several times per frame to catch up and the vehicles juddered.
     * The vehicles are handed their new placement at the end of this (apply_track_placement)
     * rather than pulling it themselves on their own beat. */
    void RailVehicleServer::vehicle_process_movement(const RID &p_vehicle, const double p_delta) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        if (placement == nullptr) {
            return;
        }
        const TrackServer *tracks = TrackServer::get_instance();
        const RID track = placement->axles[placement->primary_axle].track;
        if (track.is_valid() && (tracks == nullptr || !tracks->track_exists(track))) {
            return;
        }
        RailVehicleController *controller = _get_controller(*placement);
        if (controller == nullptr) {
            return;
        }
        // the controller's distance is front-relative (it mirrors the mover's V); this server's
        // track-offset math is rear-relative - negate at the boundary
        const double distance = -controller->process_movement(p_delta);
        _move_vehicle(p_vehicle, distance);
    }

    void RailVehicleServer::_track_changed(const RID &p_track) {
        if (p_track.is_valid()) {
            track_change_serials[p_track] = ++track_change_serial;
        }
    }

    void RailVehicleServer::_on_topology_changed() {
        topology_change_serial = ++track_change_serial;
    }

    void RailVehicleServer::_on_switch_active_track_changed(const RID &p_switch, const int p_active_track) {
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        _track_changed(tracks->switch_get_track(p_switch));
    }

    void RailVehicleServer::vehicle_report_position(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        if (placement->moved) {
            if (RailVehicleController *controller = _get_controller(*placement); controller != nullptr) {
                controller->emit_position_changed_if_needed();
            }
        }
        placement->moved = false;
    }

    /* A vehicle that has not moved keeps its location, sampling the track is not needed. */
    void RailVehicleServer::vehicle_update_location(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        if (placement->location_stale) {
            if (RailVehicleController *controller = _get_controller(*placement); controller != nullptr) {
                controller->update_location();
            }
        }
        placement->location_stale = false;
    }

    void RailVehicleServer::vehicle_update_neighbours(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        _update_neighbours(p_vehicle, *placement);
    }

    void RailVehicleServer::vehicle_collect_current(const RID &p_vehicle, const double p_delta) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        RailVehicleController *controller = _get_controller(*placement);
        ERR_FAIL_NULL(controller);
        // the pantographs at the wire the vehicle now stands under, before its circuits run on
        // what they collect (DynObj.cpp:3714-3920)
        // only a vehicle standing on a track is under a wire, as every one of the original's is
        if (const Ref<RailVehicleEnginePowerSource> power_source =
                    controller->get_rail_component(RailVehicleComponentType::COMPONENT_ENGINE_POWER_SOURCE);
            power_source.is_valid() && placement->axles[placement->primary_axle].track.is_valid()) {
            const VehicleServer *vehicle_server = VehicleServer::get_instance();
            ERR_FAIL_NULL(vehicle_server);
            const Transform3D frame = vehicle_get_transform(p_vehicle);
            VehiclePlacement *powered = vehicles.getptr(p_vehicle);
            // half the FIZ's slider (Power: CSW=, DynObj.cpp:5630) - none without it, as in the original
            const double half_width = 0.5 * power_source->get_current_collector_sliding_width();
            const bool emu = (controller->get_train_type() & RailVehicleController::TRAIN_TYPE_EZT) ==
                             RailVehicleController::TRAIN_TYPE_EZT;
            const double pressure = power_source->get_collector_pantograph_tank_pressure();
            double speed_factor = 0.0;
            const Ref<RailVehiclePowerSupply> power_supply =
                    controller->get_rail_component(RailVehicleComponentType::COMPONENT_POWER_SUPPLY);
            if (pressure > (emu ? PANTOGRAPH_EMU_RAISING_PRESSURE : PANTOGRAPH_RAISING_PRESSURE) &&
                power_supply.is_valid() &&
                (power_supply->get_power24_available() || power_supply->get_power110_available())) {
                speed_factor = MAX(0.0, PANTOGRAPH_RAISE_RATE * pressure * p_delta);
            }
            const bool active[2] = {
                    power_source->get_collector_pantograph_first_active(),
                    power_source->get_collector_pantograph_second_active()};
            for (int pantograph = 0; pantograph < 2; ++pantograph) {
                Pantograph &collector = powered->pantographs[pantograph];
                // a model without the arms samples the wire where it stands, reaching it
                if (!collector.present) {
                    continue;
                }
                // a lowered pantograph comes down whatever the wire (DynObj.cpp:3775), nothing searched
                const double gap = active[pantograph]
                                           ? static_cast<double>(_find_pantograph_wire(
                                                     p_vehicle, collector, pantograph, frame, half_width)["height"]) -
                                                     collector.height
                                           : Math::INF;
                collector.raise(gap, active[pantograph], speed_factor, p_delta);
            }
            const double assumed_voltage =
                    MAX(Math::abs(power_source->get_collector_pantograph_first_voltage()),
                        Math::abs(power_source->get_collector_pantograph_second_voltage()));
            const int collecting = static_cast<int>(active[0] && powered->pantographs[0].reaches_wire) +
                                   static_cast<int>(active[1] && powered->pantographs[1].reaches_wire);
            // a car with no engine of its own (31WE B and C) draws nothing through its own pantographs
            const Ref<RailVehicleEngine> engine = controller->get_component(VehicleComponentType::COMPONENT_ENGINE);
            const double current = collecting > 0 && engine.is_valid() ? engine->get_current0() / collecting : 0.0;
            double fed = 0.0;
            for (int pantograph = 0; pantograph < 2; ++pantograph) {
                Pantograph &collector = powered->pantographs[pantograph];
                /* The third way a raised pantograph reads no voltage, and the only one that is
                 * not about the wire: the arm has not reached it (DynObj.cpp:3784). Reported on
                 * the transition, like the other two - from the cab all three look the same. */
                if (active[pantograph] && collector.touching && !collector.reaches_wire) {
                    UtilityFunctions::push_warning(vformat(
                            "Lost contact: %s pantograph %d is not reaching the wire - %s",
                            vehicle_server->vehicle_get_id(p_vehicle), pantograph, _track_position_text(p_vehicle)));
                    // one announcement a loss: the arm short of a wire found, or no wire over it
                    emit_signal(
                            vehicle_pantograph_contact_lost_signal, p_vehicle, pantograph,
                            collector.wire.is_valid() ? PANTOGRAPH_CONTACT_LOSS_NOT_REACHING
                                                      : PANTOGRAPH_CONTACT_LOSS_NO_WIRE);
                }
                collector.touching = active[pantograph] && collector.reaches_wire;
                double voltage = 0.0;
                // the arms found the span this step; a model without them searches it here
                RID wire = collector.wire;
                if (collector.touching && !collector.present) {
                    wire = _find_pantograph_wire(p_vehicle, collector, pantograph, frame, half_width)["rid"];
                }
                if (TractionServer *traction = TractionServer::get_instance();
                    collector.touching && wire.is_valid() && traction != nullptr) {
                    voltage = traction->wire_get_voltage(wire, assumed_voltage, current);
                    traction->wire_draw_current(wire, assumed_voltage, current);
                    /* A span overhead that carries nothing is another defect than a hole in the
                     * wiring - the network behind it has no source, or the resistance never
                     * reached it - and the two look the same from the cab, as a dead line. */
                    if (collector.powered && Math::is_zero_approx(voltage)) {
                        UtilityFunctions::push_warning(
                                vformat("Dead traction: %s has a wire under pantograph %d carrying no voltage - %s, %v",
                                        vehicle_server->vehicle_get_id(p_vehicle), pantograph,
                                        _track_position_text(p_vehicle), frame.xform(collector.position)));
                        emit_signal(
                                vehicle_pantograph_contact_lost_signal, p_vehicle, pantograph,
                                PANTOGRAPH_CONTACT_LOSS_DEAD_WIRE);
                    }
                    collector.powered = !Math::is_zero_approx(voltage);
                }
                power_source->set_pantograph_wire_voltage(
                        static_cast<RailVehicleEnginePowerSource::PantographSelector>(pantograph),
                        static_cast<float>(voltage));
                fed = MAX(fed, Math::abs(voltage));
            }
            // a short loss keeps the last voltage (DynObj.cpp:3132-3140)
            if (fed > 0.0) {
                powered->no_voltage_time = 0.0;
            } else {
                powered->no_voltage_time += p_delta;
                if (powered->no_voltage_time <= NO_VOLTAGE_HOLD) {
                    fed = power_source->get_collector_voltage();
                }
            }
            power_source->set_collector_voltage(static_cast<float>(fed));
        }
    }

    /* Reported once as it stops: the track events of a standing vehicle go on by themselves
     * (ScenarioEventServer queues event0 again while it stands) */
    void RailVehicleServer::vehicle_report_track_standing(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        const RailVehicleController *controller = _get_controller(*placement);
        ERR_FAIL_NULL(controller);
        const RID track = placement->axles[placement->primary_axle].track;
        if (controller->get_speed() > STANDING_SPEED ||
            (placement->reported_heading == HEADING_STANDING && placement->reported_track == track)) {
            return;
        }
        placement->reported_heading = HEADING_STANDING;
        placement->reported_track = track;
        const Axle secondary = placement->primary_axle == AXLE_FRONT ? AXLE_REAR : AXLE_FRONT;
        const RID secondary_track = placement->axles[secondary].track;
        emit_signal(vehicle_stopped_on_track_signal, p_vehicle, track);
        // every axle of a standing vehicle fires its track's event0 (TrkFoll.cpp:122-133 has no
        // bPrimary) - the other track's too when the axles stand on two
        if (secondary_track.is_valid() && !(secondary_track == track)) {
            emit_signal(vehicle_secondary_axle_stopped_on_track_signal, p_vehicle, secondary_track);
        }
    }

    void RailVehicleServer::vehicle_report_placement(const RID &p_vehicle) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(placement);
        if (!placement->placement_unreported) {
            return;
        }
        placement->placement_unreported = false;
        emit_signal(vehicle_placement_changed_signal, p_vehicle);
    }

    /* Original engine: TDynamicObject::update_neighbours() (DynObj.cpp:7544) - a coupled end keeps
     * its coupled vehicle, a free end looks for the nearest vehicle on the route. */
    void RailVehicleServer::_clear_neighbour(
            RailVehicleController *p_controller, VehiclePlacement &p_placement,
            const RailVehicleController::CouplerEnd p_end) {
        if (p_placement.neighbour_cleared[p_end]) {
            return;
        }
        p_placement.neighbour_cleared[p_end] = true;
        p_controller->clear_neighbour(p_end);
    }

    void RailVehicleServer::_update_neighbours(const RID &p_vehicle, VehiclePlacement &p_placement) {
        RailVehicleController *controller = _get_controller(p_placement);
        const TrackServer *tracks = TrackServer::get_instance();
        if (controller == nullptr || tracks == nullptr) {
            return;
        }
        const double velocity = controller->get_velocity();
        const double scan_range = MAX(SCAN_RANGE_MINIMUM, Math::abs(velocity)) + SCAN_RANGE_MARGIN;
        // the track does not change between the two ends, so it is asked about once
        const bool on_track = tracks->track_exists(p_placement.axles[p_placement.primary_axle].track);
        for (const RailVehicleController::CouplerEnd end:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            // a coupled end is not cleared by this call: the original recomputes its coupler
            // distance on every update (DynObj.cpp:7550-7559) and CouplerForce() starts from it on
            // every step (Mover.cpp:4781) - skip it and the couplers stretch with no force
            if (controller->is_coupled(end)) {
                p_placement.neighbour_scans[end].valid = false;
                p_placement.neighbour_cleared[end] = false;
                controller->clear_neighbour(end);
                continue;
            }
            VehiclePlacement::NeighbourScan &scan = p_placement.neighbour_scans[end];
            if (!on_track) {
                scan.valid = false;
                _clear_neighbour(controller, p_placement, end);
                continue;
            }
            // nothing on the tracks the last scan went along has changed: it would find the same
            bool scan_current = scan.valid && topology_change_serial <= scan.serial;
            for (uint32_t index = 0; scan_current && index < scan.tracks.size(); ++index) {
                const uint64_t *changed = track_change_serials.getptr(scan.tracks[index]);
                scan_current = changed == nullptr || *changed <= scan.serial;
            }
            if (scan_current) {
                continue;
            }
            scan.valid = true;
            scan.serial = track_change_serial;
            RID found;
            RailVehicleController::CouplerEnd found_end = RailVehicleController::COUPLER_END_FRONT;
            double found_distance = 0.0;
            if (!_find_vehicle(
                        p_vehicle, p_placement, end, scan_range, found, found_end, found_distance, scan.tracks)) {
                _clear_neighbour(controller, p_placement, end);
                continue;
            }
            p_placement.neighbour_cleared[end] = false;
            const VehiclePlacement *other = vehicles.getptr(found);
            controller->update_neighbour(
                    end, Ref<RailVehicleController>(_get_controller(*other)), found_end, found_distance);
        }
    }

    /* Original engine: TDynamicObject::find_vehicle() (DynObj.cpp:7688) - scans the route from the
     * vehicle's primary axle towards the given end, for the primary axles of the others listed on
     * the tracks (ABuFindObject(), DynObj.cpp:1544-1640); the distance is between the two. */
    bool RailVehicleServer::_find_vehicle(
            const RID &p_vehicle, const VehiclePlacement &p_placement, const RailVehicleController::CouplerEnd p_end,
            const double p_scan_range, RID &p_found_out, RailVehicleController::CouplerEnd &p_found_end_out,
            double &p_found_distance_out, LocalVector<RID> &p_tracks_out) {
        TrackServer *tracks = TrackServer::get_instance();
        if (tracks == nullptr) {
            return false;
        }
        // server distances are rear-relative, see the step's own note on this
        const double request_sign = p_end == RailVehicleController::COUPLER_END_FRONT ? -1.0 : 1.0;
        AxleFollower cursor = p_placement.axles[p_placement.primary_axle];
        double scanned = 0.0;
        double min_along = 0.0;
        p_tracks_out.clear();

        while (scanned < p_scan_range) {
            p_tracks_out.push_back(cursor.track);
            // same conversion to the curve offset direction as in _walk_axle()
            const double movement_sign =
                    (cursor.track_direction == TrackServer::DIRECTION_NORMAL ? -1.0 : 1.0) * request_sign;
            RID found_rid;
            double found_along = INFINITY;
            for (const RID &other_rid: tracks->track_get_vehicle_list(cursor.track)) {
                const VehiclePlacement *placement = vehicles.getptr(other_rid);
                if (other_rid == p_vehicle || placement == nullptr) {
                    continue;
                }
                const AxleFollower &other = placement->axles[placement->primary_axle];
                if (other.switch_track != cursor.switch_track) {
                    continue;
                }
                const double along = (other.track_offset - cursor.track_offset) * movement_sign;
                if (along > min_along && along < found_along) {
                    found_rid = other_rid;
                    found_along = along;
                }
            }
            if (found_rid.is_valid()) {
                const VehiclePlacement *found = vehicles.getptr(found_rid);
                const double found_front_sign =
                        found->axles[found->primary_axle].track_direction == TrackServer::DIRECTION_NORMAL ? 1.0 : -1.0;
                p_found_out = found_rid;
                p_found_end_out = Math::is_equal_approx(found_front_sign, -movement_sign)
                                          ? RailVehicleController::COUPLER_END_FRONT
                                          : RailVehicleController::COUPLER_END_REAR;
                p_found_distance_out = scanned + found_along;
                return true;
            }

            const double length = tracks->track_get_length(cursor.track, cursor.switch_track);
            const double distance_to_endpoint =
                    movement_sign > 0.0 ? length - cursor.track_offset : cursor.track_offset;
            const RID previous_track = cursor.track;
            _walk_axle(cursor, request_sign * (distance_to_endpoint + SCAN_ENDPOINT_EPSILON), false, nullptr);
            if (cursor.track == previous_track) {
                return false;
            }
            scanned += distance_to_endpoint + SCAN_ENDPOINT_EPSILON;
            // a vehicle standing right at the entry point is still ahead
            min_along = -SCAN_ENDPOINT_EPSILON;
        }
        return false;
    }

    RID RailVehicleServer::_vehicle_add_cabin(const RID &p_vehicle, const RailVehicleCabinKind::Kind p_kind) {
        VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(placement, RID());
        ERR_FAIL_NULL_V(vehicle_server, RID());
        ERR_FAIL_COND_V_MSG(
                _vehicle_get_cabin(p_vehicle, p_kind).is_valid(), RID(), "The vehicle has that cabin already");
        const RID cabin = vehicle_server->cabin_create();
        vehicle_server->vehicle_cabin_attach(p_vehicle, cabin);
        placement->cabin_kinds.insert(cabin, p_kind);
        return cabin;
    }

    RID RailVehicleServer::_vehicle_get_cabin(const RID &p_vehicle, const RailVehicleCabinKind::Kind p_kind) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        if (placement == nullptr) {
            return RID();
        }
        for (const KeyValue<RID, RailVehicleCabinKind::Kind> &entry: placement->cabin_kinds) {
            if (entry.value == p_kind) {
                return entry.key;
            }
        }
        return RID();
    }

    RID RailVehicleServer::vehicle_add_front_cabin(const RID &p_vehicle) {
        return _vehicle_add_cabin(p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT);
    }

    RID RailVehicleServer::vehicle_add_rear_cabin(const RID &p_vehicle) {
        return _vehicle_add_cabin(p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR);
    }

    RID RailVehicleServer::vehicle_add_machine_room(const RID &p_vehicle) {
        return _vehicle_add_cabin(p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE);
    }

    RID RailVehicleServer::vehicle_get_front_cabin(const RID &p_vehicle) const {
        return _vehicle_get_cabin(p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT);
    }

    RID RailVehicleServer::vehicle_get_rear_cabin(const RID &p_vehicle) const {
        return _vehicle_get_cabin(p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR);
    }

    RID RailVehicleServer::vehicle_get_machine_room(const RID &p_vehicle) const {
        return _vehicle_get_cabin(p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE);
    }

    RailVehicleCabinKind::Kind RailVehicleServer::cabin_get_kind(const RID &p_cabin) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE);
        const VehiclePlacement *placement = vehicles.getptr(vehicle_server->cabin_get_vehicle(p_cabin));
        const RailVehicleCabinKind::Kind *kind =
                placement != nullptr ? placement->cabin_kinds.getptr(p_cabin) : nullptr;
        return kind != nullptr ? *kind : RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
    }

    /* The original has one driver to a vehicle; with more, the vehicle keeps answering to the one
     * it has while that one drives it - a driver sitting down in another cab takes nothing over -
     * and when it is gone, to the driver of the cab switched on, else the first of its cabins */
    RID RailVehicleServer::_find_driver(const RID &p_vehicle) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, RID());
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(placement, RID());
        if (placement->driver.is_valid() && vehicle_server->person_get_vehicle(placement->driver) == p_vehicle &&
            vehicle_server->person_get_role(placement->driver) == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER) {
            return placement->driver;
        }
        const TypedArray<VehiclePerson> drivers =
                vehicle_server->vehicle_list_persons(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
        if (drivers.is_empty()) {
            return RID();
        }
        const RailVehicleController *controller = _get_controller(*placement);
        const RailVehicleCabinKind::Kind active = controller != nullptr ? controller->get_active_cabin_kind()
                                                                        : RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
        if (active != RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE) {
            for (int index = 0; index < drivers.size(); ++index) {
                const Ref<VehiclePerson> driver = drivers[index];
                if (cabin_get_kind(driver->get_cabin()) == active) {
                    return driver->get_person();
                }
            }
        }
        return Ref<VehiclePerson>(drivers[0])->get_person();
    }

    /* Decided again on every change of the occupancy (_update_driver_cabin()), and answered from
     * what was decided */
    RID RailVehicleServer::vehicle_get_driver_cabin(const RID &p_vehicle) const {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        return placement != nullptr ? placement->driver_cabin : RID();
    }

    RID RailVehicleServer::vehicle_get_leading_cabin(const RID &p_vehicle) {
        const VehiclePlacement *placement = vehicles.getptr(p_vehicle);
        const RailVehicleController *controller = placement != nullptr ? _get_controller(*placement) : nullptr;
        const double velocity = controller != nullptr ? controller->get_velocity() : 0.0;
        const VehicleController::Direction direction =
                controller != nullptr ? controller->get_direction() : VehicleController::DIRECTION_NEUTRAL;
        // the mover's V > 0 moves the vehicle towards its front; standing, the reverser says where to
        const bool rearwards =
                velocity < 0.0 || (velocity == 0.0 && direction == VehicleController::DIRECTION_BACKWARD);
        const RID front = vehicle_get_front_cabin(p_vehicle);
        const RID rear = vehicle_get_rear_cabin(p_vehicle);
        RID facing = rearwards ? rear : front;
        if (facing.is_valid()) {
            return facing;
        }
        const RID other_end = rearwards ? front : rear;
        return other_end.is_valid() ? other_end : vehicle_get_machine_room(p_vehicle);
    }

    Error RailVehicleServer::_person_enter_cabin(
            const RID &p_person, const RID &p_vehicle, const RailVehicleCabinKind::Kind p_kind,
            const VehiclePersonRole::Role p_role) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, ERR_UNCONFIGURED);
        const RID cabin = _vehicle_get_cabin(p_vehicle, p_kind);
        return cabin.is_valid() ? vehicle_server->cabin_person_enter(cabin, p_person, p_role) : ERR_DOES_NOT_EXIST;
    }

    Error RailVehicleServer::_person_move_to_cabin(const RID &p_person, const RailVehicleCabinKind::Kind p_kind) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, ERR_UNCONFIGURED);
        const RID cabin = _vehicle_get_cabin(
                vehicle_server->cabin_get_vehicle(vehicle_server->person_get_cabin(p_person)), p_kind);
        return cabin.is_valid() ? vehicle_server->cabin_person_move(p_person, cabin) : ERR_DOES_NOT_EXIST;
    }

    Error RailVehicleServer::person_enter_front_cabin(
            const RID &p_person, const RID &p_vehicle, const VehiclePersonRole::Role p_role) {
        return _person_enter_cabin(p_person, p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT, p_role);
    }

    Error RailVehicleServer::person_enter_rear_cabin(
            const RID &p_person, const RID &p_vehicle, const VehiclePersonRole::Role p_role) {
        return _person_enter_cabin(p_person, p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR, p_role);
    }

    Error RailVehicleServer::person_enter_machine_room(
            const RID &p_person, const RID &p_vehicle, const VehiclePersonRole::Role p_role) {
        return _person_enter_cabin(p_person, p_vehicle, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE, p_role);
    }

    RID RailVehicleServer::_vehicle_find_cabin(
            const RID &p_vehicle, const std::initializer_list<RailVehicleCabinKind::Kind> p_kinds) const {
        for (const RailVehicleCabinKind::Kind kind: p_kinds) {
            if (const RID cabin = _vehicle_get_cabin(p_vehicle, kind); cabin.is_valid()) {
                return cabin;
            }
        }
        return RID();
    }

    Error RailVehicleServer::person_change_cabin(const RID &p_person, const CabinChange p_direction) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, ERR_UNAVAILABLE);
        const RID cabin = vehicle_server->person_get_cabin(p_person);
        const RID vehicle = vehicle_server->cabin_get_vehicle(cabin);
        const VehiclePlacement *placement = vehicles.getptr(vehicle);
        ERR_FAIL_NULL_V(placement, ERR_DOES_NOT_EXIST);
        const RailVehicleController *controller = _get_controller(*placement);
        const bool forward = p_direction == CABIN_CHANGE_FORWARD;
        const bool driver = placement->driver == p_person;
        // over to the next cabin of the vehicle - REAR, MACHINE, FRONT from the rear to the front; a
        // position without a cabin is passed by, where the original stops on it (Train.cpp:10346)
        const RailVehicleCabinKind::Kind kind = cabin_get_kind(cabin);
        RID target;
        if (kind == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR) {
            target = forward ? _vehicle_find_cabin(
                                       vehicle, {RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE,
                                                 RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT})
                             : RID();
        } else if (kind == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE) {
            target = _vehicle_get_cabin(
                    vehicle, forward ? RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT
                                     : RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR);
        } else if (kind == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT) {
            target = forward ? RID()
                             : _vehicle_find_cabin(
                                       vehicle, {RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE,
                                                 RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR});
        }
        if (target.is_valid()) {
            if (driver) {
                vehicle_server->vehicle_send_command(vehicle, "cab_deactivation_auto");
            }
            const Error moved = vehicle_server->cabin_person_move(p_person, target);
            if (driver) {
                vehicle_server->vehicle_send_command(vehicle, "cab_controls_reset");
                vehicle_server->vehicle_send_command(vehicle, "cab_activation_auto");
            }
            return moved;
        }
        // out of the vehicle's end through the gangways, on to the first vehicle with a cabin - the
        // original enters the neighbour by the cab facing it whether it has one or not
        // (Train.cpp:8301-8306; MASZYNA_ORIGINAL_QUIRKS.md)
        const RailVehicleController *current = controller;
        RailVehicleController::CouplerEnd end =
                forward ? RailVehicleController::COUPLER_END_FRONT : RailVehicleController::COUPLER_END_REAR;
        while (current != nullptr && !target.is_valid() &&
               current->is_coupled_by(end, RailVehicleController::COUPLING_FLAG_GANGWAY)) {
            const RailVehicleController::CouplerEnd entered = current->get_coupled_end(end);
            current = current->get_coupled_controller(end).ptr();
            if (current == controller) {
                break;
            }
            target = entered == RailVehicleController::COUPLER_END_FRONT
                             ? _vehicle_find_cabin(
                                       current->get_rid(), {RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT,
                                                            RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE,
                                                            RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR})
                             : _vehicle_find_cabin(
                                       current->get_rid(), {RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR,
                                                            RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE,
                                                            RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT});
            end = RailVehicleController::opposite_end(entered);
        }
        if (!target.is_valid()) {
            return ERR_UNAVAILABLE;
        }
        const RID target_vehicle = vehicle_server->cabin_get_vehicle(target);
        if (driver) {
            // whoever drove the vehicle rides along: there is one driver to a trainset's cab
            // (TController::MoveTo(), Driver.cpp:5866-5880)
            const TypedArray<VehiclePerson> drivers =
                    vehicle_server->vehicle_list_persons(target_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
            for (int index = 0; index < drivers.size(); ++index) {
                const Ref<VehiclePerson> other = drivers[index];
                vehicle_server->cabin_person_change_role(
                        other->get_cabin(), other->get_person(), VehiclePersonRole::VEHICLE_PERSON_ROLE_OBSERVER);
            }
            vehicle_server->vehicle_send_command(vehicle, "cabin_leave");
        }
        const Error moved = vehicle_server->cabin_person_move(p_person, target);
        if (driver) {
            vehicle_server->vehicle_send_command(target_vehicle, "cabin_enter");
        }
        return moved;
    }

    Error RailVehicleServer::person_move_to_front_cabin(const RID &p_person) {
        return _person_move_to_cabin(p_person, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT);
    }

    Error RailVehicleServer::person_move_to_rear_cabin(const RID &p_person) {
        return _person_move_to_cabin(p_person, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR);
    }

    Error RailVehicleServer::person_move_to_machine_room(const RID &p_person) {
        return _person_move_to_cabin(p_person, RailVehicleCabinKind::RAIL_VEHICLE_CABIN_MACHINE);
    }

    bool RailVehicleServer::vehicle_front_cabin_has_person_role(
            const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, false);
        return vehicle_server->cabin_has_person_role(vehicle_get_front_cabin(p_vehicle), p_role);
    }

    bool RailVehicleServer::vehicle_rear_cabin_has_person_role(
            const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, false);
        return vehicle_server->cabin_has_person_role(vehicle_get_rear_cabin(p_vehicle), p_role);
    }

    bool RailVehicleServer::vehicle_machine_room_has_person_role(
            const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, false);
        return vehicle_server->cabin_has_person_role(vehicle_get_machine_room(p_vehicle), p_role);
    }

    TypedArray<VehiclePerson> RailVehicleServer::vehicle_front_cabin_list_persons(
            const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const RID cabin = vehicle_get_front_cabin(p_vehicle);
        return vehicle_server != nullptr && cabin.is_valid() ? vehicle_server->cabin_list_persons(cabin, p_role)
                                                             : TypedArray<VehiclePerson>();
    }

    TypedArray<VehiclePerson> RailVehicleServer::vehicle_rear_cabin_list_persons(
            const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const RID cabin = vehicle_get_rear_cabin(p_vehicle);
        return vehicle_server != nullptr && cabin.is_valid() ? vehicle_server->cabin_list_persons(cabin, p_role)
                                                             : TypedArray<VehiclePerson>();
    }

    TypedArray<VehiclePerson> RailVehicleServer::vehicle_machine_room_list_persons(
            const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const RID cabin = vehicle_get_machine_room(p_vehicle);
        return vehicle_server != nullptr && cabin.is_valid() ? vehicle_server->cabin_list_persons(cabin, p_role)
                                                             : TypedArray<VehiclePerson>();
    }
} // namespace godot
