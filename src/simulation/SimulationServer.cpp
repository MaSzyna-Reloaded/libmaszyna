#include "SimulationServer.hpp"
#include "utils/LibMaszynaUnits.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
    namespace {
        /* cSun::move() and cSun::refract() (sun.cpp:104-240), the coefficients as the original
         * has them */
        // the day number counts from 2000-01-01, in the original's integer arithmetic (sun.cpp:122-128)
        constexpr int DAY_NUMBER_YEAR_DAYS = 367;
        constexpr int DAY_NUMBER_MONTH_SHIFT = 9;
        constexpr int DAY_NUMBER_LEAP_FACTOR = 7;
        constexpr int DAY_NUMBER_LEAP_DIVISOR = 4;
        constexpr int DAY_NUMBER_MONTH_DAYS = 275;
        constexpr int DAY_NUMBER_MONTH_DIVISOR = 9;
        constexpr int DAY_NUMBER_EPOCH = 730530;
        constexpr int MONTHS_PER_YEAR = 12;
        // orbital elements (sun.cpp:133-141)
        constexpr double PERIHELION_LONGITUDE = 282.9404;
        constexpr double PERIHELION_LONGITUDE_RATE = 4.70935e-5;
        constexpr double ECCENTRICITY = 0.016709;
        constexpr double ECCENTRICITY_RATE = 1.151e-9;
        constexpr double MEAN_ANOMALY = 356.0470;
        constexpr double MEAN_ANOMALY_RATE = 0.9856002585;
        constexpr double OBLIQUITY = 23.4393;
        constexpr double OBLIQUITY_RATE = 3.563e-7;
        constexpr double FULL_CIRCLE = 360.0;
        constexpr double HALF_CIRCLE = 180.0;
        constexpr double RIGHT_ANGLE = 90.0;
        // Greenwich mean sidereal time [h] and its rate, and 15 degrees an hour (sun.cpp:177-180)
        constexpr double SIDEREAL_TIME = 6.697375;
        constexpr double SIDEREAL_TIME_RATE = 0.0657098242;
        constexpr double DEGREES_PER_HOUR = 15.0;
        // refraction (sun.cpp:215-233): none near the zenith, three fits below it, scaled by the
        // pressure and the temperature
        constexpr double REFRACTION_ZENITH_ELEVATION = 85.0;
        constexpr double REFRACTION_HIGH_ELEVATION = 5.0;
        constexpr double REFRACTION_LOW_ELEVATION = -0.575;
        constexpr double REFRACTION_HIGH_A = 58.1;
        constexpr double REFRACTION_HIGH_B = 0.07;
        constexpr double REFRACTION_HIGH_C = 0.000086;
        constexpr double REFRACTION_LOW_0 = 1735.0;
        constexpr double REFRACTION_LOW_1 = -518.2;
        constexpr double REFRACTION_LOW_2 = 103.4;
        constexpr double REFRACTION_LOW_3 = -12.79;
        constexpr double REFRACTION_LOW_4 = 0.711;
        constexpr double REFRACTION_BELOW = -20.774;
        constexpr double REFRACTION_TEMPERATURE_REFERENCE = 283.0;
        constexpr double REFRACTION_STANDARD_PRESSURE = 1013.0;
        constexpr double CELSIUS_TO_KELVIN = 273.0;
        // the observer's surface pressure, millibars (sun.cpp:14)
        constexpr double SURFACE_PRESSURE = 1013.0;
        constexpr int CUBE = 3;
        constexpr int FIFTH_POWER = 5;

    } // namespace

    const char *SimulationServer::simulation_paused_signal = "simulation_paused";
    const char *SimulationServer::simulation_unpaused_signal = "simulation_unpaused";
    const char *SimulationServer::simulation_speed_changed_signal = "simulation_speed_changed";
    const char *SimulationServer::simulation_current_speed_changed_signal = "simulation_current_speed_changed";
    const char *SimulationServer::time_of_day_changed_signal = "time_of_day_changed";
    const char *SimulationServer::time_of_day_hour_changed_signal = "time_of_day_hour_changed";
    const char *SimulationServer::date_changed_signal = "date_changed";
    const char *SimulationServer::light_level_changed_signal = "light_level_changed";
    const char *SimulationServer::simulation_advanced_signal = "simulation_advanced";

    SimulationServer *SimulationServer::singleton = nullptr;

    SimulationServer::SimulationServer() {
        singleton = this;
        _on_project_settings_changed();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &SimulationServer::_on_project_settings_changed));
    }

    SimulationServer::~SimulationServer() {
        singleton = nullptr;
    }

    void SimulationServer::_on_project_settings_changed() {
        const ProjectSettings *settings = ProjectSettings::get_singleton();
        speed_change_time = settings->get_setting(SPEED_CHANGE_TIME_SETTING, SPEED_CHANGE_TIME_DEFAULT);
        light_night_altitude =
                settings->get_setting(LIGHT_LEVEL_NIGHT_ALTITUDE_SETTING, LIGHT_LEVEL_NIGHT_ALTITUDE_DEFAULT);
        light_day_altitude = settings->get_setting(LIGHT_LEVEL_DAY_ALTITUDE_SETTING, LIGHT_LEVEL_DAY_ALTITUDE_DEFAULT);
        _update_light_level();
    }

    void SimulationServer::_bind_methods() {

        ClassDB::bind_method(D_METHOD("set_time_of_day", "hours"), &SimulationServer::set_time_of_day);
        ClassDB::bind_method(D_METHOD("get_time_of_day"), &SimulationServer::get_time_of_day);
        ClassDB::bind_method(D_METHOD("set_simulation_speed", "speed"), &SimulationServer::set_simulation_speed);
        ClassDB::bind_method(D_METHOD("get_simulation_speed"), &SimulationServer::get_simulation_speed);
        ClassDB::bind_method(D_METHOD("simulation_get_current_speed"), &SimulationServer::simulation_get_current_speed);
        ClassDB::bind_method(D_METHOD("simulation_reset_speed"), &SimulationServer::simulation_reset_speed);
        ClassDB::bind_method(D_METHOD("get_light_level"), &SimulationServer::get_light_level);
        ClassDB::bind_method(D_METHOD("date_set", "year", "month", "day"), &SimulationServer::date_set);
        ClassDB::bind_method(D_METHOD("date_get_year"), &SimulationServer::date_get_year);
        ClassDB::bind_method(D_METHOD("date_get_month"), &SimulationServer::date_get_month);
        ClassDB::bind_method(D_METHOD("date_get_day"), &SimulationServer::date_get_day);
        ClassDB::bind_method(D_METHOD("set_timezone_offset", "hours"), &SimulationServer::set_timezone_offset);
        ClassDB::bind_method(D_METHOD("get_timezone_offset"), &SimulationServer::get_timezone_offset);
        ClassDB::bind_method(D_METHOD("set_latitude", "degrees"), &SimulationServer::set_latitude);
        ClassDB::bind_method(D_METHOD("get_latitude"), &SimulationServer::get_latitude);
        ClassDB::bind_method(D_METHOD("set_longitude", "degrees"), &SimulationServer::set_longitude);
        ClassDB::bind_method(D_METHOD("get_longitude"), &SimulationServer::get_longitude);
        ClassDB::bind_method(D_METHOD("set_cloud_cover", "cover"), &SimulationServer::set_cloud_cover);
        ClassDB::bind_method(D_METHOD("get_cloud_cover"), &SimulationServer::get_cloud_cover);
        ClassDB::bind_method(D_METHOD("set_use_system_time", "use"), &SimulationServer::set_use_system_time);
        ClassDB::bind_method(D_METHOD("get_use_system_time"), &SimulationServer::get_use_system_time);
        ClassDB::bind_method(D_METHOD("environment_reset"), &SimulationServer::environment_reset);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "timezone_offset"), "set_timezone_offset", "get_timezone_offset");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude"), "set_latitude", "get_latitude");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "longitude"), "set_longitude", "get_longitude");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cloud_cover"), "set_cloud_cover", "get_cloud_cover");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_system_time"), "set_use_system_time", "get_use_system_time");
        ADD_SIGNAL(MethodInfo(time_of_day_hour_changed_signal));
        ADD_SIGNAL(MethodInfo(date_changed_signal));
        ADD_SIGNAL(MethodInfo(light_level_changed_signal, PropertyInfo(Variant::FLOAT, "light_level")));
        ClassDB::bind_method(D_METHOD("set_air_temperature", "temperature"), &SimulationServer::set_air_temperature);
        ClassDB::bind_method(D_METHOD("get_air_temperature"), &SimulationServer::get_air_temperature);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_of_day"), "set_time_of_day", "get_time_of_day");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "simulation_speed"), "set_simulation_speed", "get_simulation_speed");
        ADD_SIGNAL(MethodInfo(simulation_speed_changed_signal));
        ADD_SIGNAL(MethodInfo(simulation_current_speed_changed_signal));
        ADD_SIGNAL(MethodInfo(time_of_day_changed_signal));
        ClassDB::bind_method(D_METHOD("simulation_get_time"), &SimulationServer::simulation_get_time);
        ClassDB::bind_method(D_METHOD("clock_hold"), &SimulationServer::clock_hold);
        ClassDB::bind_method(D_METHOD("clock_release"), &SimulationServer::clock_release);
        ClassDB::bind_method(D_METHOD("simulation_advance", "frame_delta"), &SimulationServer::simulation_advance);
        ADD_SIGNAL(MethodInfo(simulation_advanced_signal, PropertyInfo(Variant::FLOAT, "seconds")));
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "light_level"), "", "get_light_level");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_temperature"), "set_air_temperature", "get_air_temperature");
        ClassDB::bind_method(D_METHOD("simulation_pause"), &SimulationServer::simulation_pause);
        ClassDB::bind_method(D_METHOD("simulation_unpause"), &SimulationServer::simulation_unpause);
        ClassDB::bind_method(D_METHOD("simulation_is_paused"), &SimulationServer::simulation_is_paused);
        ADD_SIGNAL(MethodInfo(simulation_paused_signal));
        ADD_SIGNAL(MethodInfo(simulation_unpaused_signal));
    }

    /// Set, not run: the time jumps, and the date stays the one set
    void SimulationServer::set_time_of_day(const double p_hours) {
        if (time_of_day == p_hours) {
            return;
        }
        const double previous = time_of_day;
        time_of_day = p_hours;
        emit_signal(time_of_day_changed_signal);
        if (!(Math::floor(previous) == Math::floor(time_of_day))) {
            emit_signal(time_of_day_hour_changed_signal);
        }
        _update_light_level();
    }

    /// The running clock moved the time of day from p_previous, by one slice
    void SimulationServer::_time_of_day_moved(const double p_previous) {
        // past midnight it is the next day
        if (time_of_day < p_previous) {
            date_set(date_year, date_month, date_day + 1);
        }
        // the launchers look at whole minutes (EvLaunch.cpp:197-211): the clock says so when
        // one passes, not every slice - and the sun is looked at as often
        if (Math::floor(p_previous * LibMaszynaUnits::MINUTES_PER_HOUR) ==
            Math::floor(time_of_day * LibMaszynaUnits::MINUTES_PER_HOUR)) {
            return;
        }
        emit_signal(time_of_day_changed_signal);
        if (!(Math::floor(p_previous) == Math::floor(time_of_day))) {
            emit_signal(time_of_day_hour_changed_signal);
        }
        _update_light_level();
    }

    double SimulationServer::get_time_of_day() const {
        return time_of_day;
    }

    double SimulationServer::simulation_get_time() const {
        return simulation_time;
    }

    void SimulationServer::clock_hold() {
        ++clock_holders;
        _refresh_clock();
    }

    void SimulationServer::clock_release() {
        ERR_FAIL_COND(clock_holders <= 0);
        --clock_holders;
        _refresh_clock();
    }

    void SimulationServer::runtime_attach() {
        ++runtimes;
        _refresh_clock();
    }

    void SimulationServer::runtime_detach() {
        ERR_FAIL_COND(runtimes <= 0);
        --runtimes;
        _refresh_clock();
    }

    void SimulationServer::clock_subscribe(const Callable &p_on_advanced) {
        clock_hold();
        connect(simulation_advanced_signal, p_on_advanced);
    }

    void SimulationServer::clock_unsubscribe(const Callable &p_on_advanced) {
        disconnect(simulation_advanced_signal, p_on_advanced);
        clock_release();
    }

    /// The clock runs while it is held, a SimulationRuntime is attached and the runtime is not
    /// paused. It ticks on SceneTree's `process_frame`, before any node's `_process` - not in a node
    /// of its own: one created on the first hold was added to the root while the root was adding
    /// the main scene, and never ticked (FINDINGS.md 2026-09-30)
    void SimulationServer::_refresh_clock() {
        const bool running = clock_holders > 0 && runtimes > 0 && !paused;
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (running == clock_running || tree == nullptr) {
            return;
        }
        clock_running = running;
        if (clock_running) {
            tree->connect("process_frame", callable_mp(this, &SimulationServer::_on_process_frame));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &SimulationServer::_on_process_frame));
    }

    void SimulationServer::_on_process_frame() {
        const SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        simulation_advance(tree->get_root()->get_process_delta_time());
    }

    void SimulationServer::simulation_advance(const double p_frame_delta) {
        const double frame_delta = MIN(p_frame_delta, MAX_FRAME_DELTA);
        if (!(current_simulation_speed == simulation_speed)) {
            // a tape's motor: the running speed closes on the one set, most of the way in
            // speed_change_time
            const double closing = speed_change_time > 0.0 ? 1.0 - Math::exp(-frame_delta / speed_change_time) : 1.0;
            current_simulation_speed += (simulation_speed - current_simulation_speed) * closing;
            if (Math::abs(simulation_speed - current_simulation_speed) < SPEED_SETTLED) {
                current_simulation_speed = simulation_speed;
            }
            emit_signal(simulation_current_speed_changed_signal);
        }
        const double seconds = frame_delta * current_simulation_speed;
        if (seconds <= 0.0) {
            return;
        }
        // equal slices, so a frame of 0.25 s is 3 x 0.083 s and not 0.1 + 0.1 + 0.05
        const int slices = static_cast<int>(Math::ceil(seconds / MAX_SLICE_TIME));
        const double slice = seconds / slices;
        for (int index = 0; index < slices; ++index) {
            simulation_time += slice;
            const double previous = time_of_day;
            time_of_day = use_system_time ? Math::fposmod(
                                                    (Time::get_singleton()->get_unix_time_from_system() +
                                                     static_cast<double>(system_time_bias)) /
                                                            LibMaszynaUnits::SECONDS_PER_HOUR,
                                                    static_cast<double>(LibMaszynaUnits::HOURS_PER_DAY))
                                          : Math::fposmod(
                                                    time_of_day + (slice / LibMaszynaUnits::SECONDS_PER_HOUR),
                                                    static_cast<double>(LibMaszynaUnits::HOURS_PER_DAY));
            _time_of_day_moved(previous);
            emit_signal(simulation_advanced_signal, slice);
        }
    }

    void SimulationServer::set_simulation_speed(const double p_speed) {
        if (simulation_speed == p_speed) {
            return;
        }
        simulation_speed = p_speed;
        emit_signal(simulation_speed_changed_signal);
    }

    double SimulationServer::get_simulation_speed() const {
        return simulation_speed;
    }

    double SimulationServer::simulation_get_current_speed() const {
        return current_simulation_speed;
    }

    void SimulationServer::simulation_reset_speed() {
        set_simulation_speed(1.0);
        if (current_simulation_speed == simulation_speed) {
            return;
        }
        current_simulation_speed = simulation_speed;
        emit_signal(simulation_current_speed_changed_signal);
    }

    double SimulationServer::get_light_level() const {
        return light_level;
    }

    void SimulationServer::set_air_temperature(const double p_temperature) {
        air_temperature = p_temperature;
        _update_light_level();
    }

    void SimulationServer::date_set(const int p_year, const int p_month, const int p_day) {
        Time *time = Time::get_singleton();
        ERR_FAIL_NULL(time);
        Dictionary first_of_month;
        first_of_month["year"] =
                p_year + static_cast<int>(Math::floor((p_month - 1) / static_cast<double>(MONTHS_PER_YEAR)));
        first_of_month["month"] = Math::posmod(p_month - 1, MONTHS_PER_YEAR) + 1;
        first_of_month["day"] = 1;
        const Dictionary date = time->get_date_dict_from_unix_time(
                time->get_unix_time_from_datetime_dict(first_of_month) +
                (static_cast<int64_t>(p_day - 1) * LibMaszynaUnits::SECONDS_PER_DAY));
        const int year = date["year"];
        const int month = date["month"];
        const int day = date["day"];
        if (year == date_year && month == date_month && day == date_day) {
            return;
        }
        date_year = year;
        date_month = month;
        date_day = day;
        emit_signal(date_changed_signal);
        _update_light_level();
    }

    int SimulationServer::date_get_year() const {
        return date_year;
    }

    int SimulationServer::date_get_month() const {
        return date_month;
    }

    int SimulationServer::date_get_day() const {
        return date_day;
    }

    void SimulationServer::set_timezone_offset(const int p_hours) {
        timezone_offset = p_hours;
        _update_light_level();
    }

    int SimulationServer::get_timezone_offset() const {
        return timezone_offset;
    }

    void SimulationServer::set_latitude(const double p_degrees) {
        latitude = p_degrees;
        _update_light_level();
    }

    double SimulationServer::get_latitude() const {
        return latitude;
    }

    void SimulationServer::set_longitude(const double p_degrees) {
        longitude = p_degrees;
        _update_light_level();
    }

    double SimulationServer::get_longitude() const {
        return longitude;
    }

    void SimulationServer::set_cloud_cover(const double p_cover) {
        cloud_cover = p_cover;
        _update_light_level();
    }

    double SimulationServer::get_cloud_cover() const {
        return cloud_cover;
    }

    /// Switched on, the time and the date are the system's at once; from then on the clock takes
    /// the system's time every slice and rolls the date at its midnight
    void SimulationServer::set_use_system_time(const bool p_use) {
        use_system_time = p_use;
        if (!use_system_time) {
            return;
        }
        Time *time = Time::get_singleton();
        ERR_FAIL_NULL(time);
        system_time_bias =
                static_cast<int64_t>(time->get_time_zone_from_system()["bias"]) * LibMaszynaUnits::SECONDS_PER_MINUTE;
        const Dictionary now = time->get_datetime_dict_from_system();
        date_set(now["year"], now["month"], now["day"]);
        set_time_of_day(
                static_cast<double>(now["hour"]) +
                (static_cast<double>(now["minute"]) / LibMaszynaUnits::MINUTES_PER_HOUR) +
                (static_cast<double>(now["second"]) / LibMaszynaUnits::SECONDS_PER_HOUR));
    }

    bool SimulationServer::get_use_system_time() const {
        return use_system_time;
    }

    void SimulationServer::environment_reset() {
        use_system_time = false;
        timezone_offset = DEFAULT_TIMEZONE_OFFSET;
        latitude = DEFAULT_LATITUDE;
        longitude = DEFAULT_LONGITUDE;
        cloud_cover = DEFAULT_CLOUD_COVER;
        air_temperature = DEFAULT_AIR_TEMPERATURE;
        date_set(DEFAULT_YEAR, DEFAULT_MONTH, DEFAULT_DAY);
        set_time_of_day(DEFAULT_TIME_OF_DAY);
        _update_light_level();
    }

    /// cSun::move() and cSun::refract() (sun.cpp:104-240) - the location in decimal degrees (the
    /// original reads its own minutes-as-fraction notation) - then the refracted altitude ramped
    /// from night to day and dimmed by the cloud cover (simulationenvironment.cpp:163, 184)
    void SimulationServer::_update_light_level() {
        // whole days in the original's integer arithmetic, then the fraction of this one
        const int whole_days =
                (DAY_NUMBER_YEAR_DAYS * date_year) -
                (DAY_NUMBER_LEAP_FACTOR * (date_year + ((date_month + DAY_NUMBER_MONTH_SHIFT) / MONTHS_PER_YEAR)) /
                 DAY_NUMBER_LEAP_DIVISOR) +
                (DAY_NUMBER_MONTH_DAYS * date_month / DAY_NUMBER_MONTH_DIVISOR) + date_day - DAY_NUMBER_EPOCH;
        const double day_number = whole_days + (time_of_day / LibMaszynaUnits::HOURS_PER_DAY);
        const double universal_time = time_of_day - timezone_offset;
        const double perihelion_longitude = PERIHELION_LONGITUDE + (PERIHELION_LONGITUDE_RATE * day_number);
        const double eccentricity = ECCENTRICITY - (ECCENTRICITY_RATE * day_number);
        const double mean_anomaly = Math::fposmod(MEAN_ANOMALY + (MEAN_ANOMALY_RATE * day_number), FULL_CIRCLE);
        const double obliquity = OBLIQUITY - (OBLIQUITY_RATE * day_number);
        const double eccentric_anomaly =
                mean_anomaly + Math::rad_to_deg(
                                       eccentricity * Math::sin(Math::deg_to_rad(mean_anomaly)) *
                                       (1.0 + (eccentricity * Math::cos(Math::deg_to_rad(mean_anomaly)))));
        const double xv = Math::cos(Math::deg_to_rad(eccentric_anomaly)) - eccentricity;
        const double yv =
                Math::sin(Math::deg_to_rad(eccentric_anomaly)) * Math::sqrt(1.0 - (eccentricity * eccentricity));
        const double ecliptic_longitude =
                Math::fposmod(Math::rad_to_deg(Math::atan2(yv, xv)) + perihelion_longitude, FULL_CIRCLE);
        const double declination =
                Math::asin(Math::sin(Math::deg_to_rad(obliquity)) * Math::sin(Math::deg_to_rad(ecliptic_longitude)));
        const double right_ascension = Math::fposmod(
                Math::rad_to_deg(
                        Math::atan2(
                                Math::cos(Math::deg_to_rad(obliquity)) *
                                        Math::sin(Math::deg_to_rad(ecliptic_longitude)),
                                Math::cos(Math::deg_to_rad(ecliptic_longitude)))),
                FULL_CIRCLE);
        const double sidereal_time = Math::fposmod(
                SIDEREAL_TIME + (SIDEREAL_TIME_RATE * day_number) + universal_time,
                static_cast<double>(LibMaszynaUnits::HOURS_PER_DAY));
        const double hour_angle = Math::wrapf(
                Math::fposmod((sidereal_time * DEGREES_PER_HOUR) + longitude, FULL_CIRCLE) - right_ascension,
                -HALF_CIRCLE, HALF_CIRCLE);
        const double zenith_cosine =
                CLAMP((Math::sin(declination) * Math::sin(Math::deg_to_rad(latitude))) +
                              (Math::cos(declination) * Math::cos(Math::deg_to_rad(latitude)) *
                               Math::cos(Math::deg_to_rad(hour_angle))),
                      -1.0, 1.0);
        const double elevation = RIGHT_ANGLE - Math::rad_to_deg(Math::acos(zenith_cosine));
        double refraction = 0.0;
        if (elevation <= REFRACTION_ZENITH_ELEVATION) {
            const double tangent = Math::tan(Math::deg_to_rad(elevation));
            if (elevation >= REFRACTION_HIGH_ELEVATION) {
                refraction = (REFRACTION_HIGH_A / tangent) - (REFRACTION_HIGH_B / Math::pow(tangent, CUBE)) +
                             (REFRACTION_HIGH_C / Math::pow(tangent, FIFTH_POWER));
            } else if (elevation >= REFRACTION_LOW_ELEVATION) {
                refraction = REFRACTION_LOW_0 +
                             (elevation *
                              (REFRACTION_LOW_1 +
                               (elevation * (REFRACTION_LOW_2 +
                                             (elevation * (REFRACTION_LOW_3 + (elevation * REFRACTION_LOW_4)))))));
            } else {
                refraction = REFRACTION_BELOW / tangent;
            }
            refraction *= (SURFACE_PRESSURE * REFRACTION_TEMPERATURE_REFERENCE) /
                          (REFRACTION_STANDARD_PRESSURE * (CELSIUS_TO_KELVIN + air_temperature)) /
                          LibMaszynaUnits::ARCSECONDS_PER_DEGREE;
        }
        const double daylight = Math::smoothstep(light_night_altitude, light_day_altitude, elevation + refraction);
        const double level = daylight * (1.0 - (CLAMP(cloud_cover, 0.0, 1.0) * LIGHT_LEVEL_OVERCAST_FACTOR));
        if (level == light_level) {
            return;
        }
        light_level = level;
        emit_signal(light_level_changed_signal, light_level);
    }

    double SimulationServer::get_air_temperature() const {
        return air_temperature;
    }

    void SimulationServer::simulation_pause() {
        if (paused) {
            return;
        }
        paused = true;
        _refresh_clock();
        emit_signal(simulation_paused_signal);
    }

    void SimulationServer::simulation_unpause() {
        if (!paused) {
            return;
        }
        paused = false;
        _refresh_clock();
        emit_signal(simulation_unpaused_signal);
    }

    bool SimulationServer::simulation_is_paused() const {
        return paused;
    }
} // namespace godot
