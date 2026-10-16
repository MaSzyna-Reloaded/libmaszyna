#include "legacy/e3d/E3DRenderingServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "tracks/TrackServer.hpp"
#include "traffic/RoadTrafficServer.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /* [km/h] to [m/s] */
        constexpr double KMH_PER_MS = static_cast<double>(LibMaszynaUnits::SECONDS_PER_HOUR) /
                                      static_cast<double>(LibMaszynaUnits::METRES_PER_KILOMETRE);
    } // namespace

    RoadTrafficServer::RoadTrafficServer() {
        _on_project_settings_changed();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &RoadTrafficServer::_on_project_settings_changed));
    }

    RoadTrafficServer::~RoadTrafficServer() {
        _free_fleet();
    }

    void RoadTrafficServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("traffic_set_vehicle_types", "types"), &RoadTrafficServer::traffic_set_vehicle_types);
        ClassDB::bind_method(
                D_METHOD("traffic_set_time_of_day_curve", "curve"), &RoadTrafficServer::traffic_set_time_of_day_curve);
        ClassDB::bind_method(
                D_METHOD("traffic_set_density_noise", "noise"), &RoadTrafficServer::traffic_set_density_noise);
        ClassDB::bind_method(D_METHOD("traffic_set_scenario", "scenario"), &RoadTrafficServer::traffic_set_scenario);
        ClassDB::bind_method(D_METHOD("road_add_many", "tracks"), &RoadTrafficServer::road_add_many);
        ClassDB::bind_method(D_METHOD("road_remove_many", "tracks"), &RoadTrafficServer::road_remove_many);
        ClassDB::bind_method(D_METHOD("vehicle_get_rids"), &RoadTrafficServer::vehicle_get_rids);
        ClassDB::bind_method(D_METHOD("vehicle_get_track", "vehicle"), &RoadTrafficServer::vehicle_get_track);
        ClassDB::bind_method(D_METHOD("vehicle_get_transform", "vehicle"), &RoadTrafficServer::vehicle_get_transform);
        ClassDB::bind_method(D_METHOD("vehicle_get_speed", "vehicle"), &RoadTrafficServer::vehicle_get_speed);
    }

    RoadTrafficServer::TravelDirection RoadTrafficServer::_opposite(const TravelDirection p_direction) {
        return p_direction == TOWARD_P2 ? TOWARD_P1 : TOWARD_P2;
    }

    void RoadTrafficServer::_on_project_settings_changed() {
        const ProjectSettings *settings = ProjectSettings::get_singleton();
        density = settings->get_setting(DENSITY_SETTING, DEFAULT_DENSITY);
        const int previous_max_vehicles = max_vehicles;
        max_vehicles = settings->get_setting(MAX_VEHICLES_SETTING, DEFAULT_MAX_VEHICLES);
        if (max_vehicles != previous_max_vehicles) {
            _build_fleet();
        }
    }

    void RoadTrafficServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        processing = p_processing;
        if (p_processing) {
            simulation->connect(
                    SimulationServer::simulation_advanced_signal,
                    callable_mp(this, &RoadTrafficServer::_on_simulation_advanced));
            return;
        }
        simulation->disconnect(
                SimulationServer::simulation_advanced_signal,
                callable_mp(this, &RoadTrafficServer::_on_simulation_advanced));
    }

    void RoadTrafficServer::_free_fleet() {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        for (const Vehicle &vehicle: fleet) {
            if (models != nullptr && vehicle.instance.is_valid()) {
                models->instance_free(vehicle.instance);
            }
        }
        for (KeyValue<RID, Road> &road: roads) {
            for (Vector<uint32_t> &lane: road.value.lanes) {
                lane.clear();
            }
        }
        fleet.clear();
        idle.clear();
        fleet_indices.clear();
        _set_processing(false);
    }

    /* The fleet drawn from the types by their weights, every vehicle's instance built and hidden */
    void RoadTrafficServer::_build_fleet() {
        _free_fleet();
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        double total_weight = 0.0;
        for (int index = 0; index < vehicle_types.size(); ++index) {
            const Ref<RoadTrafficVehicleType> type = vehicle_types[index];
            total_weight += type.is_valid() ? Math::max(type->get_weight(), 0.0) : 0.0;
        }
        if (models == nullptr || total_weight <= 0.0) {
            return;
        }
        // a model is loaded once for every vehicle of its type
        LocalVector<Ref<E3DModel>> type_models;
        type_models.resize(vehicle_types.size());
        for (int count = 0; count < max_vehicles; ++count) {
            double pick = UtilityFunctions::randf_range(0.0, total_weight);
            int type_index = 0;
            for (; type_index < vehicle_types.size() - 1; ++type_index) {
                const Ref<RoadTrafficVehicleType> type = vehicle_types[type_index];
                pick -= type.is_valid() ? Math::max(type->get_weight(), 0.0) : 0.0;
                if (pick < 0.0) {
                    break;
                }
            }
            const Ref<RoadTrafficVehicleType> type = vehicle_types[type_index];
            if (type.is_null()) {
                continue;
            }
            if (type_models[type_index].is_null()) {
                type_models[type_index] = models->model_load(type->get_data_path(), type->get_model_filename());
            }
            if (type_models[type_index].is_null()) {
                continue;
            }
            Vehicle vehicle;
            next_vehicle_id += 1;
            vehicle.rid = UtilityFunctions::rid_from_int64(next_vehicle_id);
            vehicle.length = type->get_length();
            vehicle.width = type->get_width();
            vehicle.max_velocity = type->get_max_velocity() / KMH_PER_MS;
            vehicle.instance = models->instance_create(
                    type_models[type_index], E3DRenderingServer::INSTANCER_OPTIMIZED,
                    E3DRenderingServer::INSTANCE_KIND_DYNAMIC);
            models->instance_set_options(
                    vehicle.instance, type->get_data_path(), type->get_skins(), Array(), FORCE_ALPHA, {}, 0);
            models->instance_set_scenario(vehicle.instance, scenario);
            models->instance_set_visible(vehicle.instance, false);
            models->instance_build(vehicle.instance);
            fleet_indices.insert(vehicle.rid, fleet.size());
            idle.push_back(fleet.size());
            fleet.push_back(vehicle);
        }
        _set_processing(!fleet.is_empty() && !roads.is_empty());
    }

    void RoadTrafficServer::traffic_set_vehicle_types(const TypedArray<RoadTrafficVehicleType> &p_types) {
        vehicle_types = p_types;
        _build_fleet();
    }

    void RoadTrafficServer::traffic_set_time_of_day_curve(const Ref<Curve> &p_curve) {
        time_of_day_curve = p_curve;
    }

    void RoadTrafficServer::traffic_set_density_noise(const Ref<FastNoiseLite> &p_noise) {
        density_noise = p_noise;
    }

    void RoadTrafficServer::traffic_set_scenario(const RID &p_scenario) {
        scenario = p_scenario;
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        for (const Vehicle &vehicle: fleet) {
            models->instance_set_scenario(vehicle.instance, scenario);
        }
    }

    void RoadTrafficServer::road_add_many(const TypedArray<RID> &p_tracks) {
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        HashSet<RID> added;
        for (int index = 0; index < p_tracks.size(); ++index) {
            const RID track = p_tracks[index];
            const double friction = tracks->track_get_friction(track);
            if (roads.has(track) || tracks->track_get_group(track) != TrackServer::GROUP_ROAD ||
                (friction < MIN_ROAD_FRICTION &&
                 !Math::is_equal_approx(friction, TrackServer::DEFAULT_TRACK_FRICTION))) {
                continue;
            }
            const Ref<Curve3D> curve = tracks->track_get_domain_curve(track);
            if (curve.is_null()) {
                continue;
            }
            Road road;
            road.track = track;
            road.curve = curve;
            road.length = road.curve->get_baked_length();
            road.width = tracks->track_get_width(track);
            const double velocity = tracks->track_get_velocity(track);
            road.velocity = (velocity > 0.0 ? velocity : DEFAULT_ROAD_VELOCITY) / KMH_PER_MS;
            road.middle = road.curve->sample_baked(road.length / 2.0);
            // the noise is -1-1
            const double heat =
                    density_noise.is_valid()
                            ? Math::lerp(
                                      MIN_HEAT, 1.0,
                                      (density_noise->get_noise_2d(road.middle.x, road.middle.z) + 1.0) / 2.0)
                            : 1.0;
            const double width_weight =
                    Math::clamp(road.width / REFERENCE_ROAD_WIDTH, MIN_WIDTH_WEIGHT, MAX_WIDTH_WEIGHT);
            road.demand = (road.length / static_cast<double>(LibMaszynaUnits::METRES_PER_KILOMETRE)) *
                          BASE_VEHICLES_PER_KM * heat * width_weight;
            roads.insert(track, road);
            added.insert(track);
        }
        // a road leads on to the roads its ends meet; a road known before learns of the new one
        // beside it from the other side
        for (const RID &track: added) {
            Road *road = roads.getptr(track);
            for (const TrackServer::EndpointIndex endpoint: {TrackServer::CURVE1_P1, TrackServer::CURVE1_P2}) {
                const TravelDirection leaving = endpoint == TrackServer::CURVE1_P2 ? TOWARD_P2 : TOWARD_P1;
                const TypedArray<TrackEndpointRef> connections =
                        tracks->track_get_endpoint_connections(track, endpoint);
                for (int index = 0; index < connections.size(); ++index) {
                    const Ref<TrackEndpointRef> connection = connections[index];
                    Road *other = roads.getptr(connection->get_track_rid());
                    if (other == nullptr || connection->get_endpoint_index() > TrackServer::CURVE1_P2) {
                        continue;
                    }
                    const TravelDirection entering =
                            connection->get_endpoint_index() == TrackServer::CURVE1_P1 ? TOWARD_P2 : TOWARD_P1;
                    road->exits[leaving].push_back({other, entering});
                    if (!added.has(other->track)) {
                        other->exits[_opposite(entering)].push_back({road, _opposite(leaving)});
                    }
                }
            }
        }
        _set_processing(!fleet.is_empty() && !roads.is_empty());
        // the roads near the camera are filled at once - they come with the world, under its
        // loading screen
        const SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        if (added.is_empty() || streaming == nullptr || !streaming->streaming_has_camera()) {
            return;
        }
        const Vector3 camera = streaming->streaming_get_camera_position();
        _refresh_active_roads(camera);
        _spawn(static_cast<int>(idle.size()), SPAWN_ANYWHERE, camera);
    }

    void RoadTrafficServer::road_remove_many(const TypedArray<RID> &p_tracks) {
        for (int index = 0; index < p_tracks.size(); ++index) {
            const RID track = p_tracks[index];
            Road *road = roads.getptr(track);
            if (road == nullptr) {
                continue;
            }
            for (const Vector<uint32_t> &lane: road->lanes) {
                // released one by one, each leaving the lane
                while (!lane.is_empty()) {
                    _release(lane[0]);
                }
            }
            for (int direction = 0; direction < TRAVEL_DIRECTION_COUNT; ++direction) {
                for (const Exit &exit: road->exits[direction]) {
                    Vector<Exit> &back = exit.road->exits[_opposite(exit.direction)];
                    for (int64_t back_index = back.size() - 1; back_index >= 0; --back_index) {
                        if (back[back_index].road == road) {
                            back.remove_at(back_index);
                        }
                    }
                }
            }
            for (Vehicle &vehicle: fleet) {
                if (vehicle.next.road == road) {
                    _choose_next(vehicle);
                }
            }
            roads.erase(track);
        }
        active_roads.clear();
        active_roads_known = false;
        _set_processing(!fleet.is_empty() && !roads.is_empty());
    }

    void RoadTrafficServer::_refresh_active_roads(const Vector3 &p_camera) {
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        active_roads.clear();
        const TypedArray<RID> near = tracks->tracks_find_in_aabb(Rect2(
                p_camera.x - ACTIVE_RADIUS, p_camera.z - ACTIVE_RADIUS, 2.0 * ACTIVE_RADIUS, 2.0 * ACTIVE_RADIUS));
        for (int index = 0; index < near.size(); ++index) {
            Road *road = roads.getptr(near[index]);
            if (road != nullptr && road->middle.distance_to(p_camera) <= ACTIVE_RADIUS) {
                active_roads.push_back(road);
            }
        }
        active_center = p_camera;
        active_roads_known = true;
    }

    /* Up to p_count vehicles of the fleet onto the roads near the camera short of their share, the
     * more short the likelier, each where it has room */
    void RoadTrafficServer::_spawn(const int p_count, const SpawnPlace p_place, const Vector3 &p_camera) {
        if (idle.is_empty() || active_roads.is_empty()) {
            return;
        }
        const SimulationServer *simulation = SimulationServer::get_instance();
        const double time_factor = time_of_day_curve.is_valid() && simulation != nullptr
                                           ? time_of_day_curve->sample_baked(simulation->get_time_of_day())
                                           : 1.0;
        LocalVector<double> shortages;
        shortages.resize(active_roads.size());
        double total_shortage = 0.0;
        for (uint32_t index = 0; index < active_roads.size(); ++index) {
            const Road *road = active_roads[index];
            const double wanted = road->demand * density * time_factor;
            const double held = static_cast<double>(road->lanes[TOWARD_P2].size() + road->lanes[TOWARD_P1].size());
            shortages[index] = Math::max(wanted - held, 0.0);
            total_shortage += shortages[index];
        }
        for (int attempt = 0; attempt < p_count && !idle.is_empty() && total_shortage > 0.0; ++attempt) {
            double pick = UtilityFunctions::randf_range(0.0, total_shortage);
            uint32_t road_index = 0;
            for (; road_index < active_roads.size() - 1; ++road_index) {
                pick -= shortages[road_index];
                if (pick < 0.0) {
                    break;
                }
            }
            Road *road = active_roads[road_index];
            const TravelDirection direction =
                    UtilityFunctions::randi() % TRAVEL_DIRECTION_COUNT == 0 ? TOWARD_P2 : TOWARD_P1;
            const double distance = UtilityFunctions::randf_range(0.0, road->length);
            const double along = direction == TOWARD_P2 ? distance : road->length - distance;
            if (p_place == SPAWN_HIDDEN &&
                road->curve->sample_baked(along).distance_to(p_camera) < SPAWN_HIDDEN_DISTANCE) {
                continue;
            }
            const uint32_t idle_index = static_cast<uint32_t>(UtilityFunctions::randi() % idle.size());
            const uint32_t vehicle_index = idle[idle_index];
            const Vehicle &vehicle = fleet[vehicle_index];
            // room ahead of it and behind it in its lane, the frontmost first
            const Vector<uint32_t> &lane = road->lanes[direction];
            int64_t lane_index = 0;
            while (lane_index < lane.size() && fleet[lane[lane_index]].distance > distance) {
                ++lane_index;
            }
            const auto clear = [&](const Vehicle &p_other) {
                return Math::abs(p_other.distance - distance) - ((p_other.length + vehicle.length) / 2.0) >= MIN_GAP;
            };
            if ((lane_index > 0 && !clear(fleet[lane[lane_index - 1]])) ||
                (lane_index < lane.size() && !clear(fleet[lane[lane_index]]))) {
                continue;
            }
            idle.remove_at_unordered(idle_index);
            _enter(vehicle_index, road, direction, distance, static_cast<uint32_t>(lane_index));
            Vehicle &entered = fleet[vehicle_index];
            entered.speed = _cruise_velocity(entered);
            _place(entered);
            E3DRenderingServer::get_instance()->instance_set_visible(entered.instance, true);
            const double taken = Math::min(shortages[road_index], 1.0);
            shortages[road_index] -= taken;
            total_shortage -= taken;
        }
    }

    void RoadTrafficServer::_enter(
            const uint32_t p_vehicle, Road *p_road, const TravelDirection p_direction, const double p_distance,
            const uint32_t p_lane_index) {
        Vehicle &vehicle = fleet[p_vehicle];
        if (vehicle.road == nullptr) {
            vehicle.driver_factor = UtilityFunctions::randf_range(DRIVER_FACTOR_MIN, DRIVER_FACTOR_MAX);
        }
        vehicle.road = p_road;
        vehicle.direction = p_direction;
        vehicle.distance = p_distance;
        Vector<uint32_t> &lane = p_road->lanes[p_direction];
        lane.insert(p_lane_index, p_vehicle);
        for (int64_t index = p_lane_index; index < lane.size(); ++index) {
            fleet[lane[index]].lane_index = static_cast<uint32_t>(index);
        }
        // a driven car's lane (DynObj.cpp:2310-2321), on the right of the way it goes
        const double lane_free_space = (p_road->width / 2.0) - vehicle.width;
        if (p_road->width < NARROW_ROAD_WIDTH) {
            vehicle.lane_offset = 0.0;
        } else if (lane_free_space > WIDE_LANE_FREE_SPACE) {
            vehicle.lane_offset = (vehicle.width / 2.0) + WIDE_LANE_MARGIN;
        } else if (lane_free_space > NARROW_LANE_FREE_SPACE) {
            vehicle.lane_offset = HALF_LANE_SHARE * p_road->width;
        } else {
            vehicle.lane_offset = (vehicle.width / 2.0) + TIGHT_LANE_MARGIN;
        }
        _choose_next(vehicle);
    }

    void RoadTrafficServer::_leave_lane(const uint32_t p_vehicle) {
        const Vehicle &vehicle = fleet[p_vehicle];
        Vector<uint32_t> &lane = vehicle.road->lanes[vehicle.direction];
        lane.remove_at(vehicle.lane_index);
        for (int64_t index = vehicle.lane_index; index < lane.size(); ++index) {
            fleet[lane[index]].lane_index = static_cast<uint32_t>(index);
        }
    }

    void RoadTrafficServer::_release(const uint32_t p_vehicle) {
        _leave_lane(p_vehicle);
        Vehicle &vehicle = fleet[p_vehicle];
        vehicle.road = nullptr;
        vehicle.next = Exit();
        idle.push_back(p_vehicle);
        E3DRenderingServer::get_instance()->instance_set_visible(vehicle.instance, false);
    }

    double RoadTrafficServer::_cruise_velocity(const Vehicle &p_vehicle) {
        const double limit = p_vehicle.max_velocity > 0.0 ? Math::min(p_vehicle.max_velocity, p_vehicle.road->velocity)
                                                          : p_vehicle.road->velocity;
        return limit * p_vehicle.driver_factor;
    }

    /* A random one of the ways on from its road's end, as the original's car takes a crossing
     * (Driver.cpp:660-690) */
    void RoadTrafficServer::_choose_next(Vehicle &p_vehicle) {
        const Vector<Exit> &exits = p_vehicle.road->exits[p_vehicle.direction];
        p_vehicle.next = exits.is_empty() ? Exit() : exits[UtilityFunctions::randi() % exits.size()];
    }

    /* Where the vehicle stands: on its road, on the right of the way it goes, facing it - in the
     * frame every MaSzyna vehicle model is authored in, +Z the way it goes and +X its left
     * (DynObj.cpp:2506-2508) */
    void RoadTrafficServer::_place(Vehicle &p_vehicle) {
        const Road *road = p_vehicle.road;
        const double along = Math::clamp(
                p_vehicle.direction == TOWARD_P2 ? p_vehicle.distance : road->length - p_vehicle.distance, 0.0,
                road->length);
        const Vector3 position = road->curve->sample_baked(along);
        const double ahead = Math::min(along + HEADING_SAMPLE_DISTANCE, road->length);
        const Vector3 heading = road->curve->sample_baked(ahead) -
                                road->curve->sample_baked(Math::max(ahead - HEADING_SAMPLE_DISTANCE, 0.0));
        const Vector3 forward = (p_vehicle.direction == TOWARD_P2 ? heading : -heading).normalized();
        const Vector3 left = Vector3(0.0, 1.0, 0.0).cross(forward).normalized();
        const Vector3 up = forward.cross(left);
        p_vehicle.transform = Transform3D(Basis(left, up, forward), position - left * p_vehicle.lane_offset);
        E3DRenderingServer::get_instance()->instance_set_transform(p_vehicle.instance, p_vehicle.transform);
    }

    /* Every SPAWN_INTERVAL the roads near the camera are looked for again if it moved, the vehicles
     * too far are taken off and new ones come; every slice each vehicle keeps its distance to the
     * one ahead - on its road, or the last one of the road it goes on to - and moves on */
    void RoadTrafficServer::_on_simulation_advanced(const double p_seconds) {
        const SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        spawn_time += p_seconds;
        if (spawn_time >= SPAWN_INTERVAL && streaming != nullptr && streaming->streaming_has_camera()) {
            spawn_time = 0.0;
            const Vector3 camera = streaming->streaming_get_camera_position();
            if (!active_roads_known || active_center.distance_to(camera) > ACTIVE_REFRESH_DISTANCE) {
                _refresh_active_roads(camera);
            }
            for (uint32_t index = 0; index < fleet.size(); ++index) {
                if (fleet[index].road != nullptr &&
                    fleet[index].transform.origin.distance_to(camera) > DESPAWN_RADIUS) {
                    _release(index);
                }
            }
            _spawn(SPAWNS_PER_TICK, SPAWN_HIDDEN, camera);
        }
        for (uint32_t index = 0; index < fleet.size(); ++index) {
            Vehicle &vehicle = fleet[index];
            if (vehicle.road == nullptr) {
                continue;
            }
            double gap = Math::INF;
            if (vehicle.lane_index > 0) {
                const Vehicle &leader = fleet[vehicle.road->lanes[vehicle.direction][vehicle.lane_index - 1]];
                gap = leader.distance - vehicle.distance - ((leader.length + vehicle.length) / 2.0);
            } else if (vehicle.next.road != nullptr && !vehicle.next.road->lanes[vehicle.next.direction].is_empty()) {
                const Vector<uint32_t> &next_lane = vehicle.next.road->lanes[vehicle.next.direction];
                const Vehicle &leader = fleet[next_lane[next_lane.size() - 1]];
                gap = vehicle.road->length - vehicle.distance + leader.distance -
                      ((leader.length + vehicle.length) / 2.0);
            }
            const double wanted = Math::min(_cruise_velocity(vehicle), Math::max(gap - MIN_GAP, 0.0) / TIME_HEADWAY);
            vehicle.speed = Math::min(wanted, vehicle.speed + (CAR_ACCELERATION * p_seconds));
            vehicle.distance += vehicle.speed * p_seconds;
            if (vehicle.distance > vehicle.road->length) {
                const Exit next = vehicle.next;
                const double beyond = vehicle.distance - vehicle.road->length;
                if (next.road == nullptr) {
                    _release(index);
                    continue;
                }
                _leave_lane(index);
                _enter(index, next.road, next.direction, beyond,
                       static_cast<uint32_t>(next.road->lanes[next.direction].size()));
            }
            _place(vehicle);
        }
    }

    TypedArray<RID> RoadTrafficServer::vehicle_get_rids() const {
        TypedArray<RID> result;
        for (const Vehicle &vehicle: fleet) {
            if (vehicle.road != nullptr) {
                result.push_back(vehicle.rid);
            }
        }
        return result;
    }

    RID RoadTrafficServer::vehicle_get_track(const RID &p_vehicle) const {
        const uint32_t *index = fleet_indices.getptr(p_vehicle);
        ERR_FAIL_NULL_V(index, RID());
        const Vehicle &vehicle = fleet[*index];
        return vehicle.road != nullptr ? vehicle.road->track : RID();
    }

    Transform3D RoadTrafficServer::vehicle_get_transform(const RID &p_vehicle) const {
        const uint32_t *index = fleet_indices.getptr(p_vehicle);
        ERR_FAIL_NULL_V(index, Transform3D());
        return fleet[*index].transform;
    }

    double RoadTrafficServer::vehicle_get_speed(const RID &p_vehicle) const {
        const uint32_t *index = fleet_indices.getptr(p_vehicle);
        ERR_FAIL_NULL_V(index, 0.0);
        return fleet[*index].speed;
    }
} // namespace godot
