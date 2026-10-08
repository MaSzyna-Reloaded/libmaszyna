#pragma once
#include "vehicles/base/VehiclePersonRole.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /// The player - a PersonServer person, and the one owner of what the player drives (the
    /// original's simulation::Train). Taking a vehicle over, entering its cab while its driver
    /// drives on and letting it go happen here, as the player's seat and role in a VehicleServer
    /// cabin, whoever asks - the keys, the HUD, scripts - and are announced; the player's node
    /// follows the signal into the cab and out. It knows nothing of the view: PlayerCameraServer
    /// follows player_vehicle_entered into the cab.
    class PlayerServer : public Object {
            GDCLASS(PlayerServer, Object)

        public:
            /// The player's nick (the Settings' Player section); empty: the system's user name
            static constexpr const char *NICK_SETTING = "maszyna/player/nick";
            /// The player's name with neither a nick nor a system user name
            static constexpr const char *UNNAMED_PLAYER = "unnamed";

            /// What the player drives changed (vehicle: RID, previous: RID; invalid for none)
            static const char *player_vehicle_changed_signal;
            /// The player entered a vehicle (vehicle: RID) - also the one it already drives
            static const char *player_vehicle_entered_signal;

            static PlayerServer *get_instance() {
                return Object::cast_to<PlayerServer>(Engine::get_singleton()->get_singleton("PlayerServer"));
            }

        private:
            RID person;
            /// The vehicle last announced as the player's (player_vehicle_changed) - kept, for a
            /// vehicle freed with the player in it has taken the player's seat along already
            RID vehicle;

            /// The player in the vehicle's driver's cabin - else the one leading - in p_role, out of
            /// the vehicle it was in; RID() when it has no cabin to sit in
            RID _sit_down(const RID &p_vehicle, VehiclePersonRole::Role p_role);

            void _set_vehicle(const RID &p_vehicle);
            void _on_vehicle_freed(const RID &p_vehicle);
            void _on_cabin_person_moved(const RID &p_person, const RID &p_cabin, const RID &p_previous);
            void _on_vehicle_placed(const RID &p_vehicle);
            /// The player's person is named by the player's nick
            void _on_project_settings_changed();

        protected:
            static void _bind_methods();

        public:
            PlayerServer();
            ~PlayerServer() override;

            /// The player takes the vehicle over: sits down at the controls of its driver's cabin,
            /// whose driver rides along as an observer; the one left - of another trainset - is
            /// driven by its drivers again (TakeControl(), Driver.cpp:5700). Refused for a vehicle
            /// without a cabin. The vehicle already driven is only taken back from its driver
            /// (drivermode.cpp:258-267).
            void player_take_over_vehicle(const RID &p_vehicle);
            /// The player sits in the vehicle's driver's cabin as an observer and its driver, if it
            /// has one, drives on; the one left - of another trainset - is driven by its drivers
            /// again. Refused for a vehicle without a cabin.
            void player_enter_vehicle(const RID &p_vehicle);
            /// The player lets the trainset go: its driver drives it again (simulation.cpp:257-270)
            void player_leave_vehicle();
            /// The driver aboard the player's vehicle drives it from the player's cabin, the player
            /// rides along (aidriverenable, Train.cpp:1088-1118); player_take_back_vehicle() takes
            /// it back. Nothing without a driver aboard.
            void player_hand_over_vehicle();
            /// The player takes the controls of its vehicle back from the driver aboard
            /// (aidriverdisable) - only the roles change, the view stays where it is; F5's
            /// player_take_over_vehicle() of the same vehicle also puts the player back in the cab
            void player_take_back_vehicle();
            /// What the player drives, an invalid RID for none
            RID player_get_vehicle() const;
            /// The player as a person (PersonServer) - where it sits and in what role is VehicleServer's
            RID player_get_person() const;
    };
} // namespace godot
