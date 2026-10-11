#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// The behaviour of a DriverServer driver: what it makes of the orders it gets, and later how it
    /// drives. A driver is always the same object of the system; what differs between the original's
    /// AI driver and any other is its implementation. Implement it in C++ by overriding the virtual methods,
    /// or in GDScript by overriding their script counterparts.
    ///
    /// Every callback carries the driver: a Resource is shared, so one implementation serves several drivers
    /// and keeps what it needs per driver RID. An implementation drives the vehicle only the way a player
    /// does, through the cab.
    class DriverImplementation : public Resource {
            GDCLASS(DriverImplementation, Resource)
            friend class DriverServer;

        protected:
            static void _bind_methods();

            GDVIRTUAL1(_driver_attached, RID)
            GDVIRTUAL1(_driver_detached, RID)
            GDVIRTUAL5(_handle_command, RID, String, double, double, Vector3)
            GDVIRTUAL1(_update, RID)
            GDVIRTUAL1(_control_taken, RID)
            GDVIRTUAL1RC(Dictionary, _get_timetable_state, RID)
            GDVIRTUAL2RC(double, _get_seconds_until_departure, RID, double)
            GDVIRTUAL1RC(Dictionary, _get_state, RID)

            /// Called by DriverServer. A C++ implementation overrides these; the default forwards to the
            /// script.
            virtual void driver_attached(const RID &p_driver);
            virtual void driver_detached(const RID &p_driver);
            /// An order for the driver, with where what sent it stands
            virtual void handle_command(
                    const RID &p_driver, const String &p_command, double p_value1, double p_value2,
                    const Vector3 &p_position);
            /// The time the driver asked for has come (DriverServer.driver_schedule_update())
            virtual void update(const RID &p_driver);
            /// The driver drives its vehicle again - it sits at the controls (VehicleServer's
            /// driver's role) after a player left them; the vehicle is as the player left it
            virtual void control_taken(const RID &p_driver);
            /// The driver's timetable and how far it got through it: "timetable" (Timetable or
            /// null), "station_index" (the entry it drives to next), "station_start" (the entry
            /// shown as the station it stands at or has just left), "latency" (early on arriving at
            /// the last station [min], late when negative), "delay" (late at the station reached
            /// last [min]: its arrival, then its departure), "arrived" (it has reached the next
            /// station and not gone on yet); empty for an implementation that follows no timetable
            virtual Dictionary get_timetable_state(const RID &p_driver) const;
            /// The seconds from `p_hours` (the time of day) to the departure from the station the
            /// driver's train stands at or has just left, 0 where it only passes
            /// (seconds_until_departure(), mtable.cpp:184-190); NAN for a driver without a
            /// timetable
            virtual double get_seconds_until_departure(const RID &p_driver, double p_hours) const;
            /// What the driver keeps - its orders and what they asked for; the keys are the
            /// implementation's own, empty for an implementation that shows nothing
            virtual Dictionary get_state(const RID &p_driver) const;
    };
} // namespace godot
