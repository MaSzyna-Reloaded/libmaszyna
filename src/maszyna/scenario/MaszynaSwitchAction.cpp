#include "MaszynaSwitchAction.hpp"
#include "macros.hpp"
#include "maszyna/tracks/MaszynaTurntableServer.hpp"

namespace godot {
    void MaszynaSwitchAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &MaszynaSwitchAction::set_tracks);
        ClassDB::bind_method(D_METHOD("get_tracks"), &MaszynaSwitchAction::get_tracks);
        ClassDB::bind_method(D_METHOD("set_active_track", "active_track"), &MaszynaSwitchAction::set_active_track);
        ClassDB::bind_method(D_METHOD("get_active_track"), &MaszynaSwitchAction::get_active_track);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_tracks", "get_tracks");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "active_track", PROPERTY_HINT_ENUM,
                        enum_hint(
                                {{"Common", TrackServer::TRACK_COMMON}, {"Diverging", TrackServer::TRACK_DIVERGING}})),
                "set_active_track", "get_active_track");
        ClassDB::bind_method(D_METHOD("set_blade_speed", "blade_speed"), &MaszynaSwitchAction::set_blade_speed);
        ClassDB::bind_method(D_METHOD("get_blade_speed"), &MaszynaSwitchAction::get_blade_speed);
        ClassDB::bind_method(D_METHOD("set_blade_delay", "blade_delay"), &MaszynaSwitchAction::set_blade_delay);
        ClassDB::bind_method(D_METHOD("get_blade_delay"), &MaszynaSwitchAction::get_blade_delay);
        ClassDB::bind_method(D_METHOD("set_switches"), &MaszynaSwitchAction::set_switches);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blade_speed"), "set_blade_speed", "get_blade_speed");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blade_delay"), "set_blade_delay", "get_blade_delay");
        ClassDB::bind_method(D_METHOD("set_turntables", "turntables"), &MaszynaSwitchAction::set_turntables);
        ClassDB::bind_method(D_METHOD("get_turntables"), &MaszynaSwitchAction::get_turntables);
        ClassDB::bind_method(
                D_METHOD("set_unlock_turntables", "unlock_turntables"), &MaszynaSwitchAction::set_unlock_turntables);
        ClassDB::bind_method(D_METHOD("get_unlock_turntables"), &MaszynaSwitchAction::get_unlock_turntables);
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "turntables", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_turntables",
                "get_turntables");
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "unlock_turntables"), "set_unlock_turntables", "get_unlock_turntables");
    }

    void MaszynaSwitchAction::run(const RID &p_event, const RID &p_activator) {
        set_switches();
    }

    void MaszynaSwitchAction::set_switches() {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        // the switch of each track is taken now: a track that changed type since has another one
        for (int i = 0; i < tracks.size(); i++) {
            const RID switch_rid = track_server->track_get_switch(tracks[i]);
            if (switch_rid.is_valid()) {
                if (blade_speed > 0.0) {
                    track_server->switch_set_blade_speed(switch_rid, blade_speed);
                }
                if (blade_delay >= 0.0) {
                    track_server->switch_set_blade_delay(switch_rid, blade_delay);
                }
                track_server->switch_set_active_track(switch_rid, active_track);
                continue;
            }
            // a plain rail track switched is a derailer: its second position takes the rails away
            // (TTrack::Switch(), Track.cpp:1846-1847)
            if (track_server->track_get_group(tracks[i]) == TrackServer::GROUP_RAIL) {
                BitField<TrackServer::TrackDamage> damage = track_server->track_get_damage_flag(tracks[i]);
                if (active_track == TrackServer::TRACK_DIVERGING) {
                    damage.set_flag(TrackServer::TRACK_DAMAGE_NO_RAIL);
                } else {
                    damage.clear_flag(TrackServer::TRACK_DAMAGE_NO_RAIL);
                }
                track_server->track_set_damage_flag(tracks[i], damage);
            }
        }
        if (turntables.is_empty()) {
            return;
        }
        MaszynaTurntableServer *turntable_server = MaszynaTurntableServer::get_instance();
        ERR_FAIL_NULL(turntable_server);
        for (int i = 0; i < turntables.size(); i++) {
            if (unlock_turntables) {
                turntable_server->turntable_unlock(turntables[i]);
            } else {
                turntable_server->turntable_lock(turntables[i]);
            }
        }
    }

    void MaszynaSwitchAction::set_tracks(const TypedArray<RID> &p_tracks) {
        tracks = p_tracks;
    }

    TypedArray<RID> MaszynaSwitchAction::get_tracks() const {
        return tracks;
    }

    void MaszynaSwitchAction::set_active_track(const TrackServer::SwitchTrack p_active_track) {
        active_track = p_active_track;
    }

    TrackServer::SwitchTrack MaszynaSwitchAction::get_active_track() const {
        return active_track;
    }

    void MaszynaSwitchAction::set_blade_speed(const double p_blade_speed) {
        blade_speed = p_blade_speed;
    }

    double MaszynaSwitchAction::get_blade_speed() const {
        return blade_speed;
    }

    void MaszynaSwitchAction::set_blade_delay(const double p_blade_delay) {
        blade_delay = p_blade_delay;
    }

    double MaszynaSwitchAction::get_blade_delay() const {
        return blade_delay;
    }

    void MaszynaSwitchAction::set_turntables(const TypedArray<RID> &p_turntables) {
        turntables = p_turntables;
    }

    TypedArray<RID> MaszynaSwitchAction::get_turntables() const {
        return turntables;
    }

    void MaszynaSwitchAction::set_unlock_turntables(const bool p_unlock_turntables) {
        unlock_turntables = p_unlock_turntables;
    }

    bool MaszynaSwitchAction::get_unlock_turntables() const {
        return unlock_turntables;
    }
} // namespace godot
