#pragma once
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The dispatch of a train at a stop (odprawa): its steps one after another, each over when
    /// the vehicles report it done - the cars' passengers exchanged, the train let go, its doors
    /// closed. A dispatch is kept under the vehicle its train is driven from. The cars open their
    /// doors and exchange by themselves (RailVehicleLoad); whoever drives works the cab's controls
    /// as the step asks. The original keeps it as a negative fStopTime of the driver and the
    /// driver's Doors() (Driver.cpp:1233-1241, 4266-4356, 6767-6801).
    class StationServer : public Object {
            GDCLASS(StationServer, Object)

        public:
            enum DispatchStep {
                /// No dispatch
                DISPATCH_STEP_NONE,
                /// The passengers get off and on
                DISPATCH_STEP_EXCHANGE,
                /// The exchange is over, the train is not let go yet
                DISPATCH_STEP_WAIT_DEPARTURE,
                /// The train is let go: its doors close
                DISPATCH_STEP_CLOSE_DOORS,
            };

            /// The dispatch of the train driven from the vehicle went to another step
            /// (vehicle: RID, step: DispatchStep)
            static const char *dispatch_step_changed_signal;
            /// The dispatch is over, the train may go (vehicle: RID)
            static const char *dispatch_finished_signal;

            static StationServer *get_instance() {
                return Object::cast_to<StationServer>(Engine::get_singleton()->get_singleton("StationServer"));
            }

        private:
            /// A car of the dispatch and what it reports, followed while the dispatch lasts
            struct Car {
                    RID vehicle;
                    Ref<RailVehicleLoad> load;
                    Ref<RailVehicleDoors> doors;
            };
            struct Dispatch {
                    DispatchStep step = DISPATCH_STEP_NONE;
                    /// Let go while its passengers still exchanged: the doors close once they are done
                    bool departure_allowed = false;
                    Vector<Car> cars;
                    /// The cars still exchanging, and those whose doors are still not closed
                    HashSet<RID> exchanging;
                    HashSet<RID> doors_not_closed;
            };
            HashMap<RID, Dispatch> dispatches;

            void _set_step(const RID &p_vehicle, Dispatch &p_dispatch, DispatchStep p_step);
            void _close_doors(const RID &p_vehicle, Dispatch &p_dispatch);
            void _finish(const RID &p_vehicle);
            void _on_load_exchange_finished(const RID &p_vehicle, const RID &p_car);
            void _on_doors_closed(int p_side, const RID &p_vehicle, const RID &p_car);
            void _on_vehicle_freed(const RID &p_vehicle);

        protected:
            static void _bind_methods();

        public:
            StationServer();

            /// The dispatch of the train driven from the vehicle, of the cars given: those whose
            /// load is being exchanged (RailVehicleServer.load_add/load_remove) are waited for, the
            /// train's earlier dispatch is dropped
            void dispatch_start(const RID &p_vehicle, const TypedArray<RID> &p_cars);
            /// The train may go as far as its timetable says: once the passengers are done, its
            /// doors close
            void dispatch_depart(const RID &p_vehicle);
            /// The dispatch is dropped - the train left the stop
            void dispatch_cancel(const RID &p_vehicle);
            DispatchStep dispatch_get_step(const RID &p_vehicle) const;
            /// The seconds the longest exchange of the dispatch still takes, 0 when none
            double dispatch_get_exchange_time(const RID &p_vehicle) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(StationServer::DispatchStep)
