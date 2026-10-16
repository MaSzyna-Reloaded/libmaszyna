#pragma once
#include "traffic/RoadTrafficVehicleType.hpp"
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* Road traffic around the camera: vehicles driven along TrackServer's roads (GROUP_ROAD) the
     * server is given (road_add_many()) - no physics and no driver, a body keeping its distance to
     * the one ahead and taking a random way at a junction, as the original's car does
     * (Driver.cpp:660-690). A vehicle with nowhere to go at the end of its road - a dead end, the
     * edge of the roads given - is taken off.
     *
     * The vehicles are a fleet built once (traffic_set_vehicle_types(), the max_vehicles setting):
     * building a model's instance makes its materials on the main thread, so none is built while
     * the traffic runs - a vehicle taken off waits hidden for its next road. How many drive on a
     * road is its length, the density setting, a heat map (traffic_set_density_noise()), its width
     * and the time of day (traffic_set_time_of_day_curve()). The traffic moves with the simulation's
     * clock (SimulationServer.simulation_advanced) - it stands while the simulation is paused. */
    class RoadTrafficServer : public Object {
            GDCLASS(RoadTrafficServer, Object)

        public:
            static constexpr const char *DENSITY_SETTING = "maszyna/traffic/density";
            static constexpr double DEFAULT_DENSITY = 0.5;
            static constexpr const char *MAX_VEHICLES_SETTING = "maszyna/traffic/max_vehicles";
            static constexpr int DEFAULT_MAX_VEHICLES = 60;

            static RoadTrafficServer *get_instance() {
                return Object::cast_to<RoadTrafficServer>(Engine::get_singleton()->get_singleton("RoadTrafficServer"));
            }

        private:
            /* Which way along its road a vehicle drives */
            enum TravelDirection {
                TOWARD_P2,
                TOWARD_P1,
            };
            static constexpr int TRAVEL_DIRECTION_COUNT = 2;
            /* A road narrower than this is driven in its middle (DynObj.cpp:2311) */
            static constexpr double NARROW_ROAD_WIDTH = 3.5;
            /* A driven car's lane (DynObj.cpp:2316-2321): with more free space in its half of the
             * road than WIDE_LANE_FREE_SPACE it keeps WIDE_LANE_MARGIN off the middle, with more
             * than NARROW_LANE_FREE_SPACE it takes the middle of its half, else it keeps
             * TIGHT_LANE_MARGIN off the middle [m] */
            static constexpr double WIDE_LANE_FREE_SPACE = 1.5;
            static constexpr double WIDE_LANE_MARGIN = 0.75;
            static constexpr double NARROW_LANE_FREE_SPACE = 0.1;
            static constexpr double HALF_LANE_SHARE = 0.25;
            static constexpr double TIGHT_LANE_MARGIN = 0.075;
            /* A car's acceleration and braking at most [m/s2] (Driver.cpp:7397) */
            static constexpr double CAR_ACCELERATION = 2.0;
            /* The gap a vehicle keeps to the one ahead standing [m], and the time it keeps to it
             * driving [s] */
            static constexpr double MIN_GAP = 2.0;
            static constexpr double TIME_HEADWAY = 1.5;
            /* A road without a speed limit of its own is driven at this [km/h] - a built-up area's */
            static constexpr double DEFAULT_ROAD_VELOCITY = 50.0;
            /* How much faster or slower than the limit a driver goes */
            static constexpr double DRIVER_FACTOR_MIN = 0.85;
            static constexpr double DRIVER_FACTOR_MAX = 1.05;
            /* Vehicles a kilometre of road holds at full density, both ways together */
            static constexpr double BASE_VEHICLES_PER_KM = 20.0;
            /* A road this wide [m] takes its length's share of the traffic; a narrower one less, a
             * wider one more, within the weights */
            static constexpr double REFERENCE_ROAD_WIDTH = 6.0;
            static constexpr double MIN_WIDTH_WEIGHT = 0.25;
            static constexpr double MAX_WIDTH_WEIGHT = 2.0;
            /* The quietest place of the heat map keeps this share of the traffic */
            static constexpr double MIN_HEAT = 0.1;
            /* The roads within this of the camera have traffic [m]; they are looked for again when
             * the camera has moved ACTIVE_REFRESH_DISTANCE, and a vehicle further than
             * DESPAWN_RADIUS is taken off */
            static constexpr double ACTIVE_RADIUS = 1000.0;
            static constexpr double ACTIVE_REFRESH_DISTANCE = 100.0;
            static constexpr double DESPAWN_RADIUS = 1200.0;
            /* Every SPAWN_INTERVAL [s] of the simulation at most SPAWNS_PER_TICK vehicles come onto
             * the roads, none within SPAWN_HIDDEN_DISTANCE [m] of the camera */
            static constexpr double SPAWN_INTERVAL = 1.0;
            static constexpr int SPAWNS_PER_TICK = 4;
            static constexpr double SPAWN_HIDDEN_DISTANCE = 250.0;
            /* How far ahead along the road a vehicle's heading is taken [m] */
            static constexpr double HEADING_SAMPLE_DISTANCE = 1.0;
            /* A vehicle's windows are drawn as a rail vehicle's are (RailVehicleRenderingServer) */
            static constexpr bool FORCE_ALPHA = true;

            /* Where a vehicle comes onto a road: anywhere, while the world is loaded under a
             * loading screen, or out of the camera's sight */
            enum SpawnPlace {
                SPAWN_ANYWHERE,
                SPAWN_HIDDEN,
            };

            struct Road;
            /* Where a vehicle leaving a road goes on: the road, and which way along it */
            struct Exit {
                    Road *road = nullptr;
                    TravelDirection direction = TOWARD_P2;
            };
            /* Copied into the HashMap that holds it: its lists are Vectors, which a LocalVector cannot be */
            struct Road {
                    RID track;
                    Ref<Curve3D> curve;
                    /* Of its curve [m] */
                    double length = 0.0;
                    double width = 0.0;
                    /* [m/s] */
                    double velocity = 0.0;
                    /* The vehicles it holds at full density, at any time of day: its length, width and
                     * heat */
                    double demand = 0.0;
                    Vector3 middle;
                    /* By the way it is driven: the roads its end leads on to */
                    Vector<Exit> exits[TRAVEL_DIRECTION_COUNT];
                    /* By the way it is driven: the vehicles on it, by index of the fleet, the
                     * frontmost first */
                    Vector<uint32_t> lanes[TRAVEL_DIRECTION_COUNT];
            };
            struct Vehicle {
                    RID rid;
                    /* E3DRenderingServer's */
                    RID instance;
                    double length = 0.0;
                    double width = 0.0;
                    /* [m/s]; 0 for none */
                    double max_velocity = 0.0;
                    /* Drawn when it comes onto a road */
                    double driver_factor = 1.0;
                    /* None while it waits in the fleet */
                    Road *road = nullptr;
                    TravelDirection direction = TOWARD_P2;
                    /* Where it is in its lane (Road::lanes), 0 the frontmost */
                    uint32_t lane_index = 0;
                    /* Of its middle, from where it came onto the road [m] */
                    double distance = 0.0;
                    /* [m/s] */
                    double speed = 0.0;
                    /* Its right [m] off the road's middle */
                    double lane_offset = 0.0;
                    /* Where it goes on at the road's end; none when it is taken off there */
                    Exit next;
                    Transform3D transform;
            };

            /* Their pointers stand while they are registered: a HashMap keeps every element where it
             * was allocated */
            HashMap<RID, Road> roads;
            LocalVector<Vehicle> fleet;
            /* The fleet's vehicles off the roads */
            LocalVector<uint32_t> idle;
            HashMap<RID, uint32_t> fleet_indices;
            /* The roads near the camera (ACTIVE_RADIUS), and where they were looked for from */
            LocalVector<Road *> active_roads;
            Vector3 active_center;
            bool active_roads_known = false;
            double spawn_time = 0.0;
            int64_t next_vehicle_id = 0;
            bool processing = false;

            TypedArray<RoadTrafficVehicleType> vehicle_types;
            Ref<Curve> time_of_day_curve;
            Ref<FastNoiseLite> density_noise;
            RID scenario;
            double density = DEFAULT_DENSITY;
            int max_vehicles = DEFAULT_MAX_VEHICLES;

            static TravelDirection _opposite(TravelDirection p_direction);
            void _on_project_settings_changed();
            /* The tick of the traffic, connected to SimulationServer's clock while there are roads
             * and a fleet to drive them */
            void _on_simulation_advanced(double p_seconds);
            void _set_processing(bool p_processing);
            void _build_fleet();
            void _free_fleet();
            void _refresh_active_roads(const Vector3 &p_camera);
            void _spawn(int p_count, SpawnPlace p_place, const Vector3 &p_camera);
            /* The vehicle comes onto the road at p_distance, into its lane at p_lane_index */
            void
            _enter(uint32_t p_vehicle, Road *p_road, TravelDirection p_direction, double p_distance,
                   uint32_t p_lane_index);
            /* The vehicle leaves its lane, the ones behind it moving up */
            void _leave_lane(uint32_t p_vehicle);
            /* The vehicle goes back to the fleet, hidden */
            void _release(uint32_t p_vehicle);
            /* How fast the vehicle drives its road with nothing ahead [m/s] */
            static double _cruise_velocity(const Vehicle &p_vehicle);
            void _choose_next(Vehicle &p_vehicle);
            void _place(Vehicle &p_vehicle);

        protected:
            static void _bind_methods();

        public:
            RoadTrafficServer();
            ~RoadTrafficServer() override;

            /* The kinds of vehicles the fleet is drawn from, by their weights; the fleet is built
             * again from them */
            void traffic_set_vehicle_types(const TypedArray<RoadTrafficVehicleType> &p_types);
            /* The density's multiplier by the hours of the day - the curve's domain is 0-24 */
            void traffic_set_time_of_day_curve(const Ref<Curve> &p_curve);
            /* The heat map: a road's traffic grows with the noise where its middle is */
            void traffic_set_density_noise(const Ref<FastNoiseLite> &p_noise);
            /* Where the fleet is drawn; none takes it out of every scenario */
            void traffic_set_scenario(const RID &p_scenario);
            /* Roads of TrackServer to drive on; the tracks of another group are passed over */
            void road_add_many(const TypedArray<RID> &p_tracks);
            /* Roads no longer driven on - their vehicles go back to the fleet - before they are freed */
            void road_remove_many(const TypedArray<RID> &p_tracks);
            /* The vehicles on the roads */
            TypedArray<RID> vehicle_get_rids() const;
            RID vehicle_get_track(const RID &p_vehicle) const;
            Transform3D vehicle_get_transform(const RID &p_vehicle) const;
            /* [m/s] */
            double vehicle_get_speed(const RID &p_vehicle) const;
    };
} // namespace godot
