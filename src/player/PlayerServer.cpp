#include "PlayerServer.hpp"
#include "driver/DriverServer.hpp"
#include "person/PersonServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *PlayerServer::player_vehicle_changed_signal = "player_vehicle_changed";
    const char *PlayerServer::player_vehicle_entered_signal = "player_vehicle_entered";

    /// The drivers riding along in the vehicle's trainset back at the controls of their cabins,
    /// where nobody else drives from (TakeControl(true), simulation.cpp:257-270)
    static void trainset_drivers_take_control(const RID &p_vehicle) {
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        VehicleServer *vehicles = VehicleServer::get_instance();
        const DriverServer *drivers = DriverServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_NULL(drivers);
        const TypedArray<RID> trainset = rail_vehicles->vehicle_get_coupled(
                p_vehicle, RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLING_FLAG_COUPLER);
        for (int index = 0; index < trainset.size(); ++index) {
            const RID driver = drivers->vehicle_get_driver(trainset[index]);
            if (vehicles->person_get_role(driver) == VehiclePersonRole::VEHICLE_PERSON_ROLE_OBSERVER) {
                vehicles->cabin_person_change_role(
                        vehicles->person_get_cabin(driver), driver, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
            }
        }
    }

    /// Another trainset's vehicle: the one left is driven by its drivers again
    static void trainset_left_drivers_take_control(const RID &p_previous, const RID &p_vehicle) {
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        if (p_previous.is_valid() && !rail_vehicles
                                              ->vehicle_get_coupled(
                                                      p_previous, RailVehicleController::COUPLER_END_FRONT,
                                                      RailVehicleController::COUPLING_FLAG_COUPLER)
                                              .has(p_vehicle)) {
            trainset_drivers_take_control(p_previous);
        }
    }

    /// Whoever drives from the cabin rides along: the controls are free for somebody else
    static void cabin_release_controls(const RID &p_cabin) {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        const TypedArray<VehiclePerson> at_controls =
                vehicles->cabin_list_persons(p_cabin, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
        for (int index = 0; index < at_controls.size(); ++index) {
            const Ref<VehiclePerson> driver = at_controls[index];
            vehicles->cabin_person_change_role(
                    p_cabin, driver->get_person(), VehiclePersonRole::VEHICLE_PERSON_ROLE_OBSERVER);
        }
    }

    void PlayerServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("player_take_over_vehicle", "vehicle"), &PlayerServer::player_take_over_vehicle);
        ClassDB::bind_method(D_METHOD("player_enter_vehicle", "vehicle"), &PlayerServer::player_enter_vehicle);
        ClassDB::bind_method(D_METHOD("player_leave_vehicle"), &PlayerServer::player_leave_vehicle);
        ClassDB::bind_method(D_METHOD("player_hand_over_vehicle"), &PlayerServer::player_hand_over_vehicle);
        ClassDB::bind_method(D_METHOD("player_take_back_vehicle"), &PlayerServer::player_take_back_vehicle);
        ClassDB::bind_method(D_METHOD("player_get_vehicle"), &PlayerServer::player_get_vehicle);
        ClassDB::bind_method(D_METHOD("player_get_person"), &PlayerServer::player_get_person);

        ADD_SIGNAL(MethodInfo(
                player_vehicle_changed_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "previous")));
        ADD_SIGNAL(MethodInfo(player_vehicle_entered_signal, PropertyInfo(Variant::RID, "vehicle")));
    }

    /// A vehicle freed with the player in it leaves the player on foot. No explicit disconnect:
    /// callable_mp reports this instance as the callable's object, so the engine drops the
    /// connection when it dies.
    PlayerServer::PlayerServer() {
        PersonServer *persons = PersonServer::get_instance();
        ERR_FAIL_NULL(persons);
        person = persons->person_create();
        _on_project_settings_changed();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &PlayerServer::_on_project_settings_changed));
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(VehicleServer::vehicle_freed_signal, callable_mp(this, &PlayerServer::_on_vehicle_freed));
        vehicles->connect(
                VehicleServer::cabin_person_moved_signal, callable_mp(this, &PlayerServer::_on_cabin_person_moved));
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        rail_vehicles->connect(
                RailVehicleServer::vehicle_placed_signal, callable_mp(this, &PlayerServer::_on_vehicle_placed));
        rail_vehicles->connect(
                RailVehicleServer::vehicle_placement_changed_signal,
                callable_mp(this, &PlayerServer::_on_vehicle_placed));
    }

    void PlayerServer::_on_project_settings_changed() {
        PersonServer *persons = PersonServer::get_instance();
        ERR_FAIL_NULL(persons);
        String name = String(ProjectSettings::get_singleton()->get_setting(NICK_SETTING, String())).strip_edges();
        if (name.is_empty()) {
            // the system's user name: USER on Linux and macOS, USERNAME on Windows
            const OS *os = OS::get_singleton();
            name = os->has_environment("USER") ? os->get_environment("USER") : os->get_environment("USERNAME");
        }
        persons->person_set_name(person, name.is_empty() ? String(UNNAMED_PLAYER) : name);
    }

    PlayerServer::~PlayerServer() {
        if (PersonServer *persons = PersonServer::get_instance(); persons != nullptr) {
            persons->person_free(person);
        }
    }

    /// The player's vehicle is kept streamed in where it stands - an AI may drive it on while the
    /// camera looks elsewhere
    void PlayerServer::_on_vehicle_placed(const RID &p_vehicle) {
        if (p_vehicle != vehicle) {
            return;
        }
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(streaming);
        ERR_FAIL_NULL(rail_vehicles);
        streaming->streaming_set_anchor_position(rail_vehicles->vehicle_get_transform(vehicle).origin);
    }

    void PlayerServer::_on_vehicle_freed(const RID &p_vehicle) {
        if (p_vehicle == vehicle) {
            _set_vehicle(RID());
        }
    }

    /* The player went to another cab (RailVehicleServer::person_change_cabin()): of the same vehicle
     * (TTrain::CabChange(), Train.cpp:10324), or through a gangway into another vehicle, as the
     * original moves simulation::Train (TTrain::MoveToVehicle(), Train.cpp:10928-10933). Driving,
     * the player takes its vehicle's driver along into every cab - the original's driver is the
     * train's, its cab the occupied one (CabOccupied) - and the driver of a vehicle entered gets
     * out: one driver to a vehicle (TController::MoveTo(), Driver.cpp:5864-5880) */
    void PlayerServer::_on_cabin_person_moved(const RID &p_person, const RID &p_cabin, const RID & /* p_previous */) {
        VehicleServer *vehicles = VehicleServer::get_instance();
        const DriverServer *drivers = DriverServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_NULL(drivers);
        const RID entered = vehicles->cabin_get_vehicle(p_cabin);
        if (p_person != person || !vehicle.is_valid()) {
            return;
        }
        if (vehicles->person_get_role(person) == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER) {
            if (const RID driver = drivers->vehicle_get_driver(vehicle); driver.is_valid()) {
                if (const RID other = drivers->vehicle_get_driver(entered); other.is_valid() && entered != vehicle) {
                    vehicles->cabin_person_leave(vehicles->person_get_cabin(other), other);
                }
                vehicles->cabin_person_move(driver, p_cabin);
            }
        }
        if (entered == vehicle) {
            return;
        }
        _set_vehicle(entered);
        emit_signal(player_vehicle_entered_signal, entered);
    }

    void PlayerServer::_set_vehicle(const RID &p_vehicle) {
        const RID previous = vehicle;
        vehicle = p_vehicle;
        // nothing is kept streamed in for a player without a vehicle
        if (vehicle.is_valid()) {
            _on_vehicle_placed(vehicle);
        } else if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->streaming_clear_anchor();
        }
        emit_signal(player_vehicle_changed_signal, vehicle, previous);
    }

    RID PlayerServer::_sit_down(const RID &p_vehicle, const VehiclePersonRole::Role p_role) {
        VehicleServer *vehicles = VehicleServer::get_instance();
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, RID());
        ERR_FAIL_NULL_V(rail_vehicles, RID());
        ERR_FAIL_COND_V(!vehicles->vehicle_exists(p_vehicle), RID());
        ERR_FAIL_COND_V_MSG(
                vehicles->vehicle_get_cabin_count(p_vehicle) == 0, RID(), "The vehicle has no cabin to sit in");
        RID cabin = rail_vehicles->vehicle_get_driver_cabin(p_vehicle);
        if (!cabin.is_valid()) {
            cabin = rail_vehicles->vehicle_get_leading_cabin(p_vehicle);
        }
        // a cabin the railway has no word for
        if (!cabin.is_valid()) {
            cabin = vehicles->vehicle_get_cabins(p_vehicle)[0];
        }
        if (const RID seat = vehicles->person_get_cabin(person); seat.is_valid()) {
            vehicles->cabin_person_leave(seat, person);
        }
        if (p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER) {
            cabin_release_controls(cabin);
        }
        ERR_FAIL_COND_V(!(vehicles->cabin_person_enter(cabin, person, p_role) == OK), RID());
        return cabin;
    }

    void PlayerServer::player_take_over_vehicle(const RID &p_vehicle) {
        VehicleServer *vehicles = VehicleServer::get_instance();
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_NULL(rail_vehicles);
        if (p_vehicle == vehicle && vehicle.is_valid()) {
            // the train already driven: the controls taken back, and the player back in its cab
            // (InOutKey(), drivermode.cpp:258-267)
            player_take_back_vehicle();
            emit_signal(player_vehicle_entered_signal, p_vehicle);
            return;
        }
        const RID previous = vehicle;
        if (!_sit_down(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER).is_valid()) {
            return;
        }
        trainset_left_drivers_take_control(previous, p_vehicle);
        // taking a vehicle over activates its cab when the FIZ allows it (Train.cpp:9147)
        vehicles->vehicle_send_command(p_vehicle, "cab_activation_auto");
        _set_vehicle(p_vehicle);
        emit_signal(player_vehicle_entered_signal, p_vehicle);
    }

    void PlayerServer::player_enter_vehicle(const RID &p_vehicle) {
        if (p_vehicle == vehicle && vehicle.is_valid()) {
            emit_signal(player_vehicle_entered_signal, p_vehicle);
            return;
        }
        const RID previous = vehicle;
        if (!_sit_down(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_OBSERVER).is_valid()) {
            return;
        }
        trainset_left_drivers_take_control(previous, p_vehicle);
        _set_vehicle(p_vehicle);
        emit_signal(player_vehicle_entered_signal, p_vehicle);
    }

    /* The driver aboard sits down at the controls of the player's cabin, the player rides along
     * (aidriverenable, Train.cpp:1088-1118) - from a standing start, as the original switches the
     * driver off first so that one already driving starts over */
    void PlayerServer::player_hand_over_vehicle() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        const DriverServer *drivers = DriverServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_NULL(drivers);
        const RID driver = drivers->vehicle_get_driver(vehicle);
        if (!driver.is_valid()) {
            return;
        }
        const RID cabin = vehicles->person_get_cabin(person);
        vehicles->cabin_person_change_role(cabin, person, VehiclePersonRole::VEHICLE_PERSON_ROLE_OBSERVER);
        vehicles->cabin_person_change_role(
                vehicles->person_get_cabin(driver), driver, VehiclePersonRole::VEHICLE_PERSON_ROLE_OBSERVER);
        vehicles->cabin_person_move(driver, cabin);
        vehicles->cabin_person_change_role(cabin, driver, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
    }

    /* The player back at the controls of its cabin, whoever drove from it riding along
     * (aidriverdisable, Train.cpp:1088-1118) - the view stays where it is */
    void PlayerServer::player_take_back_vehicle() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        if (!vehicle.is_valid()) {
            return;
        }
        const RID cabin = vehicles->person_get_cabin(person);
        cabin_release_controls(cabin);
        vehicles->cabin_person_change_role(cabin, person, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
    }

    void PlayerServer::player_leave_vehicle() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        if (!vehicle.is_valid()) {
            return;
        }
        if (const RID seat = vehicles->person_get_cabin(person); seat.is_valid()) {
            vehicles->cabin_person_leave(seat, person);
        }
        trainset_drivers_take_control(vehicle);
        _set_vehicle(RID());
    }

    RID PlayerServer::player_get_vehicle() const {
        return vehicle;
    }

    RID PlayerServer::player_get_person() const {
        return person;
    }
} // namespace godot
