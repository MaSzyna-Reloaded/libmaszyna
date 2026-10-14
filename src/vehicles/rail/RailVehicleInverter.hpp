#pragma once
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {
    /// The state of one inverter of an induction motor (TMoverParameters::Inverters, MOVER.h:845)
    class RailVehicleInverter : public RefCounted {
            GDCLASS(RailVehicleInverter, RefCounted);

        private:
            bool active = true;
            bool error = false;
            bool allow = true;

        protected:
            static void _bind_methods();

        public:
            /// It works (IsActive)
            void set_active(bool p_active);
            bool get_active() const;
            /// It failed (Error)
            void set_error(bool p_error);
            bool get_error() const;
            /// It is allowed to work (Activate)
            void set_allow(bool p_allow);
            bool get_allow() const;
    };
} // namespace godot
