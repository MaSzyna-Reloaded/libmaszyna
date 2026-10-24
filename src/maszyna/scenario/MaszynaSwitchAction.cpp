#include "MaszynaSwitchAction.hpp"
#include "macros.hpp"

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
    }

    void MaszynaSwitchAction::run(const RID &p_event, const RID &p_activator) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        // the switch of each track is taken now: a track that changed type since has another one
        for (int i = 0; i < tracks.size(); i++) {
            const RID switch_rid = track_server->track_get_switch(tracks[i]);
            if (switch_rid.is_valid()) {
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
} // namespace godot
