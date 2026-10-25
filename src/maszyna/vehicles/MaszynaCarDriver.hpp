#pragma once
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* The original's driver of a car (TController of is_car()), one for every car a scenery seats a
     * driver in, by the car's VehicleServer handle - no driver object per car. It drives the way
     * the original's drives a car without a timetable: shunting, at its shunting velocity at most,
     * slowing down for and stopping at what the memories read by the road's passive events say
     * (SetVelocity), and moving off a while after the way clears. How it follows the vehicle ahead,
     * keeps the roads' limits and turns at a road's end is RoadVehicleServer's, by the car's
     * configuration (FizVehicleBuilder). It acts on the car by its commands only. */
    class MaszynaCarDriver : public Object {
            GDCLASS(MaszynaCarDriver, Object)

        public:
            static MaszynaCarDriver *get_instance() {
                return singleton;
            }

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline MaszynaCarDriver *singleton = nullptr;

            /* Original engine: a driver thinks this often (EasyReactionTime, Driver.cpp:150) [s] */
            static constexpr double REACTION_TIME = 0.5;
            /* No timetable: the driver shunts (OrdersInit(), Driver.cpp:5243-5249), at its shunting
             * velocity at most (Driver.cpp:7322-7327; fShuntVelocity, Driver.h:431) [km/h], a
             * scenery's velocity under this one telling it to stand (Driver.cpp:5316-5319) */
            static constexpr double SHUNT_VELOCITY = 40.0;
            static constexpr double READY_VELOCITY = 1.0;
            /* How far ahead it reads the road (awarenessrange, Driver.cpp:4993-4997) [m] */
            static constexpr double AWARENESS_RANGE = 750.0;
            /* Its braking distance [m] is this times v * (40 + v), v in km/h (fDriverBraking of a car,
             * Driver.cpp:1877; determine_braking_distance(), Driver.cpp:6601) */
            static constexpr double DRIVER_BRAKING = 0.03;
            static constexpr double BRAKING_VELOCITY_OFFSET = 40.0;
            /* It stops between these short of a memory saying stop, the more the faster it goes
             * (fMinProximityDist, fMaxProximityDist of a car, Driver.cpp:6789-6790) [m, m a km/h] */
            static constexpr double MIN_PROXIMITY = 3.5;
            static constexpr double MIN_PROXIMITY_PER_SPEED = 0.2;
            static constexpr double MAX_PROXIMITY = 9.5;
            static constexpr double MAX_PROXIMITY_PER_SPEED = 0.375;
            /* It waits this long before moving off (WaitingExpireTime of a car, Driver.cpp:2748) [s] */
            static constexpr double WAITING_EXPIRE_TIME = 3.0;
            /* A memory's text that is a velocity for the cars (TMemCell, the crossings' and limits'
             * memories) and its value that leaves the velocity free */
            static constexpr const char *SET_VELOCITY = "SetVelocity";
            static constexpr double NO_LIMIT = -1.0;
            /* A scenery's order to sound the horn: for value1 seconds the horns of value2's bits
             * (Driver.cpp:4857-4862; WarningSignal, MOVER.h:2101) */
            static constexpr const char *WARNING_SIGNAL = "Warning_signal";

            struct Car {
                    /* The velocity its orders give it [km/h] - 0 for one told to stand - and the
                     * one it last commanded */
                    double ordered = 0.0;
                    double wanted = 0.0;
                    /* The simulated time it moves off at, after the way cleared while it stood */
                    double move_off_time = 0.0;
                    bool stopped = true;
                    /* The simulated time its horn falls silent at (fWarningDuration,
                     * Driver.cpp:5993) */
                    double horn_off_time = 0.0;
            };

            RIDMap<Car> cars;
            /* The cars in the order they think, round-robin, and the next to think */
            LocalVector<RID> order;
            uint32_t next = 0;
            /* What of a reaction time is left to the cars not thought yet this round */
            double carried = 0.0;
            double time = 0.0;
            bool processing = false;

            /* The velocity [km/h] the car may want now, within p_ordered, for what the road ahead says */
            double _road_velocity(const RID &p_vehicle, double p_ordered) const;
            void _think(const RID &p_vehicle, Car &p_car);
            void _set_processing(bool p_processing);
            void _on_simulation_advanced(double p_seconds);

        protected:
            static void _bind_methods();

        public:
            MaszynaCarDriver();
            ~MaszynaCarDriver() override;

            /* The car - a RoadVehicleServer vehicle - is driven from now on; p_velocity is its
             * scenery velocity [km/h], whose orders it starts with. A freed car is let go of on its own */
            void vehicle_attach(const RID &p_vehicle, double p_velocity);
            void vehicle_detach(const RID &p_vehicle);
            bool vehicle_is_attached(const RID &p_vehicle) const;
            /* An order of the scenery for the car's driver - what an event the car ran sends it
             * (MaszynaVehicleCommandAction); a car's driver takes only the horn's */
            void
            vehicle_handle_command(const RID &p_vehicle, const String &p_command, double p_value1, double p_value2);
    };
} // namespace godot
