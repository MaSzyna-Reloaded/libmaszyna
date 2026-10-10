#pragma once
#include "scenario/ScenarioEventAction.hpp"

namespace godot {
    /// The original's `putvalues` and `getvalues` events (Event.cpp:577-844): a command - its own, or
    /// read from a memory when the event runs. What the track does to a vehicle stays with the
    /// events: `CabSignal` is the cab signal magnet acting on the vehicle that queued the event, and
    /// `Emergency_brake` a Radio-Stop sent from where the event stands. The rest is an order for the
    /// driver of the vehicle that queued the event (DriverServer.driver_send_command()).
    class MaszynaLegacyVehicleCommandAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyVehicleCommandAction, ScenarioEventAction)

        public:
            /// The scenery's commands a vehicle acts on (Mover.cpp:12624, 12649)
            static constexpr const char *CAB_SIGNAL = "CabSignal";
            static constexpr const char *EMERGENCY_BRAKE = "Emergency_brake";

        private:
            String command;
            double value1 = 0.0;
            double value2 = 0.0;
            RID source;
            Vector3 position;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_command(const String &p_command);
            String get_command() const;
            void set_value1(double p_value1);
            double get_value1() const;
            void set_value2(double p_value2);
            double get_value2() const;
            /// `getvalues`: the memory whose text and values are the command, read when the event
            /// runs; empty for the action's own
            void set_source(const RID &p_source);
            RID get_source() const;
            /// Where the event stands: `putvalues` x y z, the memory's position for `getvalues`
            void set_position(const Vector3 &p_position);
            Vector3 get_position() const;
    };
} // namespace godot
