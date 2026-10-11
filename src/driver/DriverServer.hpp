#pragma once
#include "DriverImplementation.hpp"
#include "vehicles/base/VehiclePersonRole.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <queue>
#include <vector>

namespace godot {
    /// The AI drivers - PersonServer persons declared drivers of a named DriverImplementation - by the
    /// person's handle. A driver takes the orders a scenario gives (driver_send_command()) and,
    /// through its implementation, what they mean; it drives the vehicle it sits in (VehicleServer)
    /// as a player does, through the cab, while it sits there in the driver's role. A player at the
    /// controls needs no driver.
    ///
    /// The implementations are registered by name (implementation_register()), as VehicleServer's
    /// are: whoever declares a driver (a scenery's trainset) names what it thinks with, and knows
    /// nothing of the class. A driver of a name nobody registered yet thinks with nothing - its
    /// orders are lost - until the implementation is registered, so it is registered before the
    /// drivers get their orders.
    ///
    /// A driver acts in moments, as the original's does after its reaction time (TController::
    /// ReactionTime, Driver.cpp:150-158): its implementation asks for the next one
    /// (driver_schedule_update()) and is called when it comes. The time is SimulationServer's
    /// simulation time, which the physics and the events read too.
    class DriverServer : public Object {
            GDCLASS(DriverServer, Object)

        public:
            static DriverServer *get_instance() {
                return Object::cast_to<DriverServer>(Engine::get_singleton()->get_singleton("DriverServer"));
            }

        private:
            struct DriverData {
                    /// What the driver was declared to think with, and the implementation registered
                    /// under it, null while none is
                    StringName implementation_name;
                    Ref<DriverImplementation> implementation;
                    /// The sequence of its scheduled update in the queue, 0 while none is
                    uint64_t update_sequence = 0;
            };

            /// A driver's update at a time; the sequence keeps entries of one time in the order
            /// they were scheduled
            struct UpdateEntry {
                    double time = 0.0;
                    uint64_t sequence = 0;
                    RID driver;

                    bool operator>(const UpdateEntry &p_other) const {
                        return time == p_other.time ? sequence > p_other.sequence : time > p_other.time;
                    }
            };

            HashMap<RID, DriverData> drivers;
            HashMap<StringName, Ref<DriverImplementation>> implementations;
            std::priority_queue<UpdateEntry, std::vector<UpdateEntry>, std::greater<UpdateEntry>> updates;
            uint64_t next_sequence = 1;
            /// While something is scheduled it holds the runtime's clock and runs as it advances
            bool processing = false;

            void _on_person_freed(const RID &p_person);
            void _on_cabin_person_role_changed(const RID &p_cabin, const RID &p_person, VehiclePersonRole::Role p_role);
            void _detach(const RID &p_driver);
            void _set_implementation(const RID &p_driver, const Ref<DriverImplementation> &p_implementation);
            void _set_processing(bool p_processing);
            void _process_updates(double p_seconds);

        protected:
            static void _bind_methods();

        public:
            static const char *driver_timetable_changed_signal;
            static const char *driver_order_changed_signal;
            /// The person is no driver any more (driver: RID) - its implementation was taken, or the
            /// person freed
            static const char *driver_freed_signal;
            /// The person is a driver now (driver: RID) - it was declared one
            static const char *driver_attached_signal;

            DriverServer();
            ~DriverServer() override;

            /// Every driver there is
            TypedArray<RID> driver_get_rids() const;
            /// p_implementation thinks for the drivers declared with p_name, from now on those declared
            /// before it as well; a name registered again is thought with by the new one
            void implementation_register(const StringName &p_name, const Ref<DriverImplementation> &p_implementation);
            /// The drivers declared with p_name think with nothing until it is registered again
            void implementation_unregister(const StringName &p_name);
            /// The person becomes a driver thinking with the implementation registered as
            /// p_implementation (implementation_register()), as soon as there is one; an empty name
            /// makes it none
            void driver_attach_implementation(const RID &p_driver, const StringName &p_implementation);
            Ref<DriverImplementation> driver_get_implementation(const RID &p_driver) const;
            /// The driver aboard the vehicle, in whatever role (the original's Mechanik); RID() for none
            RID vehicle_get_driver(const RID &p_vehicle) const;
            /// An order for the driver - a scenario's command with its two values, and where what
            /// sent it stands
            void driver_send_command(
                    const RID &p_driver, const String &p_command, double p_value1, double p_value2,
                    const Vector3 &p_position = Vector3());
            /// The driver's implementation is updated in p_seconds of simulated time; a later call
            /// replaces the one pending
            void driver_schedule_update(const RID &p_driver, double p_seconds);
            /// Whether the vehicle's driver sits at its controls (AIControllFlag) - not while it
            /// rides along in a cab a player drives from; it still takes its orders then, but
            /// touches no control. False for a vehicle without a driver.
            bool vehicle_is_control_active(const RID &p_vehicle) const;
            /// The driver's timetable and its progress (DriverImplementation::get_timetable_state()); empty
            /// without an implementation
            Dictionary driver_get_timetable_state(const RID &p_driver) const;
            /// The seconds from p_hours (the time of day) to the departure of the vehicle's train
            /// (DriverImplementation::get_seconds_until_departure()): by the timetable of its own
            /// driver, else of the first driver of its trainset with one (Mechanik, else ctOwner,
            /// Event.cpp:2431-2435); 0 for a train without a timetable
            double vehicle_get_seconds_until_departure(const RID &p_vehicle, double p_hours) const;
            /// The driver's implementation reports that its timetable, or how far it got through it, has
            /// changed - announced as driver_timetable_changed
            void driver_report_timetable_changed(const RID &p_driver);
            /// The driver's implementation reports that the driver took up another order (OrderCheck(),
            /// Driver.cpp:5161) - announced as driver_order_changed
            void driver_report_order_changed(const RID &p_driver);
            /// What the driver keeps (DriverImplementation::get_state()); empty without an implementation
            Dictionary driver_get_state(const RID &p_driver) const;
    };
} // namespace godot
