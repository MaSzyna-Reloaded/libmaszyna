#include "MaszynaLegacyTrackVelocityAction.hpp"
#include "tracks/TrackServer.hpp"

namespace godot {
    void MaszynaLegacyTrackVelocityAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &MaszynaLegacyTrackVelocityAction::set_tracks);
        ClassDB::bind_method(D_METHOD("get_tracks"), &MaszynaLegacyTrackVelocityAction::get_tracks);
        ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &MaszynaLegacyTrackVelocityAction::set_velocity);
        ClassDB::bind_method(D_METHOD("get_velocity"), &MaszynaLegacyTrackVelocityAction::get_velocity);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_tracks", "get_tracks");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity"), "set_velocity", "get_velocity");
    }

    void MaszynaLegacyTrackVelocityAction::run(const RID &p_event, const RID &p_activator) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        for (int i = 0; i < tracks.size(); i++) {
            track_server->track_set_velocity(tracks[i], velocity);
        }
    }

    void MaszynaLegacyTrackVelocityAction::set_tracks(const TypedArray<RID> &p_tracks) {
        tracks = p_tracks;
    }

    TypedArray<RID> MaszynaLegacyTrackVelocityAction::get_tracks() const {
        return tracks;
    }

    void MaszynaLegacyTrackVelocityAction::set_velocity(const double p_velocity) {
        velocity = p_velocity;
    }

    double MaszynaLegacyTrackVelocityAction::get_velocity() const {
        return velocity;
    }
} // namespace godot
