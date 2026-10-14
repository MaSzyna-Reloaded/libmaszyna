#include "DriverImplementation.hpp"
#include <godot_cpp/core/math.hpp>

namespace godot {
    void DriverImplementation::_bind_methods() {
        GDVIRTUAL_BIND(_driver_attached, "driver");
        GDVIRTUAL_BIND(_driver_detached, "driver");
        GDVIRTUAL_BIND(_handle_command, "driver", "command", "value1", "value2", "position");
        GDVIRTUAL_BIND(_update, "driver");
        GDVIRTUAL_BIND(_control_taken, "driver");
        GDVIRTUAL_BIND(_get_timetable_state, "driver");
        GDVIRTUAL_BIND(_get_seconds_until_departure, "driver", "hours");
        GDVIRTUAL_BIND(_get_state, "driver");
    }

    void DriverImplementation::driver_attached(const RID &p_driver) {
        GDVIRTUAL_CALL(_driver_attached, p_driver);
    }

    void DriverImplementation::driver_detached(const RID &p_driver) {
        GDVIRTUAL_CALL(_driver_detached, p_driver);
    }

    void DriverImplementation::handle_command(
            const RID &p_driver, const String &p_command, const double p_value1, const double p_value2,
            const Vector3 &p_position) {
        GDVIRTUAL_CALL(_handle_command, p_driver, p_command, p_value1, p_value2, p_position);
    }

    void DriverImplementation::update(const RID &p_driver) {
        GDVIRTUAL_CALL(_update, p_driver);
    }

    void DriverImplementation::control_taken(const RID &p_driver) {
        GDVIRTUAL_CALL(_control_taken, p_driver);
    }

    Dictionary DriverImplementation::get_timetable_state(const RID &p_driver) const {
        Dictionary state;
        GDVIRTUAL_CALL(_get_timetable_state, p_driver, state);
        return state;
    }

    double DriverImplementation::get_seconds_until_departure(const RID &p_driver, const double p_hours) const {
        double seconds = Math::NaN;
        GDVIRTUAL_CALL(_get_seconds_until_departure, p_driver, p_hours, seconds);
        return seconds;
    }

    Dictionary DriverImplementation::get_state(const RID &p_driver) const {
        Dictionary state;
        GDVIRTUAL_CALL(_get_state, p_driver, state);
        return state;
    }
} // namespace godot
