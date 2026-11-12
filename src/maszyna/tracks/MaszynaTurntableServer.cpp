#include "MaszynaTurntableServer.hpp"
#include "maszyna/e3d/MaszynaE3DRenderingServer.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "tracks/TrackCurve.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    MaszynaTurntableServer *MaszynaTurntableServer::singleton = nullptr;

    void MaszynaTurntableServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("turntable_create", "track"), &MaszynaTurntableServer::turntable_create);
        ClassDB::bind_method(D_METHOD("turntable_free", "turntable"), &MaszynaTurntableServer::turntable_free);
        ClassDB::bind_method(
                D_METHOD("turntable_set_model", "turntable", "instance", "submodel"),
                &MaszynaTurntableServer::turntable_set_model);
        ClassDB::bind_method(
                D_METHOD("turntable_set_joined_event", "turntable", "event"),
                &MaszynaTurntableServer::turntable_set_joined_event);
        ClassDB::bind_method(D_METHOD("turntable_unlock", "turntable"), &MaszynaTurntableServer::turntable_unlock);
        ClassDB::bind_method(D_METHOD("turntable_lock", "turntable"), &MaszynaTurntableServer::turntable_lock);
        ClassDB::bind_method(
                D_METHOD("turntable_is_locked", "turntable"), &MaszynaTurntableServer::turntable_is_locked);
    }

    MaszynaTurntableServer *MaszynaTurntableServer::get_instance() {
        return singleton;
    }

    MaszynaTurntableServer::MaszynaTurntableServer() {
        singleton = this;
    }

    MaszynaTurntableServer::~MaszynaTurntableServer() {
        singleton = nullptr;
    }

    RID MaszynaTurntableServer::turntable_create(const RID &p_track) {
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL_V(tracks, RID());
        const Ref<TrackCurve> curve = tracks->track_get_curve(p_track);
        ERR_FAIL_COND_V(curve.is_null(), RID());
        TurntableData turntable;
        turntable.track = p_track;
        // the middle of the curve as a Bezier's at half its parameter, its control points relative
        // to its ends (Segment::Init(), Track.cpp:600)
        const Vector3 p1 = curve->get_p1();
        const Vector3 p2 = curve->get_p2();
        const Vector3 middle = p1.bezier_interpolate(p1 + curve->get_c1(), p2 + curve->get_c2(), p2, CURVE_MIDDLE);
        turntable.location = (p1 + middle + p2) / LOCATION_POINTS;
        turntable.half_length = CURVE_MIDDLE * tracks->track_get_length(p_track);
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        turntables.insert(rid, turntable);
        return rid;
    }

    void MaszynaTurntableServer::turntable_free(const RID &p_turntable) {
        _set_following(p_turntable, false);
        turntables.erase(p_turntable);
    }

    void MaszynaTurntableServer::turntable_set_model(
            const RID &p_turntable, const RID &p_instance, const String &p_submodel) {
        TurntableData *turntable = turntables.getptr(p_turntable);
        ERR_FAIL_NULL(turntable);
        const ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        turntable->instance = p_instance;
        turntable->submodel = p_submodel;
        // the yaw the model was placed with, as the scenery's `angles` give it (ModelData.rotation)
        turntable->model_yaw = Math::rad_to_deg(models->instance_get_transform(p_instance).basis.get_euler().y);
        _set_following(p_turntable, turntable->unlocked);
    }

    void MaszynaTurntableServer::turntable_set_joined_event(const RID &p_turntable, const RID &p_event) {
        TurntableData *turntable = turntables.getptr(p_turntable);
        ERR_FAIL_NULL(turntable);
        turntable->joined_event = p_event;
    }

    void MaszynaTurntableServer::turntable_unlock(const RID &p_turntable) {
        TurntableData *turntable = turntables.getptr(p_turntable);
        ERR_FAIL_NULL(turntable);
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        // its neighbours let go of, and the AI kept off it (Track.cpp:1791-1805)
        tracks->track_disconnect(turntable->track);
        tracks->track_set_velocity(turntable->track, 0.0);
        turntable->unlocked = true;
        _set_following(p_turntable, true);
    }

    void MaszynaTurntableServer::turntable_lock(const RID &p_turntable) {
        TurntableData *turntable = turntables.getptr(p_turntable);
        ERR_FAIL_NULL(turntable);
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        // the last step of its turning, then joined where it stands (Track.cpp:1810-1813)
        _follow(*turntable);
        tracks->track_connect(turntable->track);
        bool joined = false;
        for (const TrackServer::EndpointIndex end: {TrackServer::CURVE1_P1, TrackServer::CURVE1_P2}) {
            const TypedArray<TrackEndpointRef> neighbours =
                    tracks->track_get_endpoint_connections(turntable->track, end);
            for (int64_t i = 0; i < neighbours.size(); ++i) {
                joined = true;
                // a neighbour stopped for it is let on again (Track.cpp:1817-1825)
                const RID neighbour = Ref<TrackEndpointRef>(neighbours[i])->get_track_rid();
                if (tracks->track_get_velocity(neighbour) == 0.0) {
                    tracks->track_set_velocity(neighbour, JOINED_VELOCITY);
                }
            }
        }
        if (joined) {
            tracks->track_set_velocity(turntable->track, JOINED_VELOCITY);
            // the confirmation of the joining, such as a signal showing proceed (Track.cpp:1826-1829)
            if (MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance();
                events != nullptr && turntable->joined_event.is_valid()) {
                events->event_add_to_query(turntable->joined_event);
            }
        }
        turntable->unlocked = false;
        _set_following(p_turntable, false);
    }

    bool MaszynaTurntableServer::turntable_is_locked(const RID &p_turntable) const {
        const TurntableData *turntable = turntables.getptr(p_turntable);
        ERR_FAIL_NULL_V(turntable, true);
        return !turntable->unlocked;
    }

    void MaszynaTurntableServer::_follow(TurntableData &p_turntable) const {
        const MaszynaE3DRenderingServer *e3d = MaszynaE3DRenderingServer::get_instance();
        TrackServer *tracks = TrackServer::get_instance();
        // a locked one, or one of no model or no animated submodel, does not move (Track.cpp:2026-2029)
        if (!p_turntable.unlocked || !p_turntable.instance.is_valid() || p_turntable.submodel.is_empty() ||
            e3d == nullptr || tracks == nullptr) {
            return;
        }
        // the angle about the submodel's z, and its translation as the scenery's axes
        // (TAnimContainer::AngleGet(), TransGet(), AnimModel.h:92-96)
        const double angle = e3d->instance_get_submodel_rotation(p_turntable.instance, p_turntable.submodel).z;
        const Vector3 offset = e3d->instance_get_submodel_translation(p_turntable.instance, p_turntable.submodel);
        const Vector3 translation(T3D_SIDE_SIGN * offset.x, offset.z, offset.y);
        // compared with the angle it was turned to, the model's yaw included, as the original does
        if (angle == p_turntable.offset && translation == p_turntable.translation) {
            return;
        }
        // a straight track through its middle, as long as it was loaded (Track.cpp:2039-2051)
        p_turntable.offset = p_turntable.model_yaw + angle;
        p_turntable.translation = translation;
        const double radians = Math::deg_to_rad(p_turntable.offset);
        const Vector3 half(
                -p_turntable.half_length * Math::sin(radians), 0.0, -p_turntable.half_length * Math::cos(radians));
        const Vector3 middle = p_turntable.location + translation;
        Ref<TrackCurve> curve;
        curve.instantiate();
        curve->set_p1(middle + half);
        curve->set_p2(middle - half);
        // the vehicles on it are moved with it by RailVehicleServer (Track.cpp:2053-2056)
        tracks->track_update_curves(p_turntable.track, curve, Ref<Resource>());
    }

    void MaszynaTurntableServer::_on_simulation_advanced(const double /* p_seconds */) {
        for (const RID &rid: following) {
            if (TurntableData *turntable = turntables.getptr(rid); turntable != nullptr) {
                _follow(*turntable);
            }
        }
    }

    void MaszynaTurntableServer::_set_following(const RID &p_turntable, const bool p_following) {
        const TurntableData *turntable = turntables.getptr(p_turntable);
        const bool follows = p_following && turntable != nullptr && turntable->instance.is_valid();
        const bool was_empty = following.is_empty();
        if (follows && !following.has(p_turntable)) {
            following.push_back(p_turntable);
        } else if (!follows) {
            following.erase(p_turntable);
        }
        SimulationServer *simulation = SimulationServer::get_instance();
        if (simulation == nullptr || was_empty == following.is_empty()) {
            return;
        }
        // stepped only while one turns
        if (was_empty) {
            simulation->connect(
                    SimulationServer::simulation_advanced_signal,
                    callable_mp(this, &MaszynaTurntableServer::_on_simulation_advanced));
        } else {
            simulation->disconnect(
                    SimulationServer::simulation_advanced_signal,
                    callable_mp(this, &MaszynaTurntableServer::_on_simulation_advanced));
        }
    }
} // namespace godot
