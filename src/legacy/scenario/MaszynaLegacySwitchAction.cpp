#include "MaszynaLegacySwitchAction.hpp"
#include "macros.hpp"

namespace godot {
    void MaszynaLegacySwitchAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &MaszynaLegacySwitchAction::set_tracks);
        ClassDB::bind_method(D_METHOD("get_tracks"), &MaszynaLegacySwitchAction::get_tracks);
        ClassDB::bind_method(
                D_METHOD("set_active_track", "active_track"), &MaszynaLegacySwitchAction::set_active_track);
        ClassDB::bind_method(D_METHOD("get_active_track"), &MaszynaLegacySwitchAction::get_active_track);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_tracks", "get_tracks");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "active_track", PROPERTY_HINT_ENUM,
                        enum_hint(
                                {{"Common", TrackServer::TRACK_COMMON}, {"Diverging", TrackServer::TRACK_DIVERGING}})),
                "set_active_track", "get_active_track");
    }

    void MaszynaLegacySwitchAction::run(const RID &p_event, const RID &p_activator) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        for (int i = 0; i < tracks.size(); i++) {
            track_server->switch_set_active_track(tracks[i], active_track);
        }
    }

    void MaszynaLegacySwitchAction::set_tracks(const TypedArray<RID> &p_tracks) {
        tracks = p_tracks;
    }

    TypedArray<RID> MaszynaLegacySwitchAction::get_tracks() const {
        return tracks;
    }

    void MaszynaLegacySwitchAction::set_active_track(const TrackServer::SwitchTrack p_active_track) {
        active_track = p_active_track;
    }

    TrackServer::SwitchTrack MaszynaLegacySwitchAction::get_active_track() const {
        return active_track;
    }
} // namespace godot
