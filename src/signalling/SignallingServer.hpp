#pragma once
#include "SignalHeadKind.hpp"
#include "SignallingImplementation.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// RID based registry of signal heads and of the systems that control them.
    ///
    /// A signal head is the lights of one model instance; it lives exactly as long as that
    /// instance. Low level, its lights are switched one by one. High level, signal heads and event
    /// sources are grouped into systems, and a system's SignallingImplementation decides what an
    /// event means and shows it on the system's signal heads.
    class SignallingServer : public Object {
            GDCLASS(SignallingServer, Object)

        public:
            enum LightState {
                LIGHT_STATE_OFF,
                LIGHT_STATE_ON,
                LIGHT_STATE_BLINKING,
            };

            /// The direction a sensor reports a passage in: a one-coil sensor has none, a two-coil one
            /// tells along the track from against it
            enum SourcePolarity {
                SOURCE_POLARITY_NONE,
                SOURCE_POLARITY_POSITIVE,
                SOURCE_POLARITY_NEGATIVE,
            };

            /// What a route is set for
            enum RouteKind {
                ROUTE_KIND_TRAIN,
                ROUTE_KIND_SHUNTING,
            };

            /// Where a route is between being set and released; only its system's implementation moves it
            enum RouteState {
                /// Not set
                ROUTE_STATE_IDLE,
                /// Its switches are being moved
                ROUTE_STATE_SETTING,
                /// Held while its start signal head allows the passage (zamknięcie)
                ROUTE_STATE_CLOSED,
                /// Held until released (utwierdzenie)
                ROUTE_STATE_LOCKED,
                /// Being released with a delay
                ROUTE_STATE_RELEASING,
            };

            /// iMaxNumLights, AnimModel.h:23
            static constexpr int MAX_LIGHTS = 8;
            /// A sensor's passage, sent with source_send_event(): {SOURCE_POLARITY_ARGUMENT: SourcePolarity}
            static constexpr const char *SOURCE_IMPULSE_EVENT = "impulse";
            static constexpr const char *SOURCE_POLARITY_ARGUMENT = "polarity";
            /// The isolated section a source stands for came to be occupied - its first axle came onto
            /// it - or free again, its last axle left (TIsolated::Modify(), Track.cpp:118-147, the
            /// section's `:busy` and `:free`); sent with no arguments
            static constexpr const char *SOURCE_OCCUPIED_EVENT = "occupied";
            static constexpr const char *SOURCE_FREED_EVENT = "freed";

            static const char *signal_head_registered_signal;
            static const char *signal_head_unregistered_signal;
            static const char *signal_head_light_state_changed_signal;
            static const char *signal_head_aspect_changed_signal;
            static const char *signal_head_config_changed_signal;
            static const char *system_signal_head_added_signal;
            static const char *system_signal_head_removed_signal;
            static const char *system_source_added_signal;
            static const char *system_source_removed_signal;
            static const char *system_switch_added_signal;
            static const char *system_switch_removed_signal;
            static const char *system_linked_signal;
            static const char *system_unlinked_signal;
            static const char *route_state_changed_signal;
            static const char *route_section_released_signal;
            static const char *route_config_changed_signal;

            /// The PROPERTY_HINT_ENUM string of LightState
            static String light_state_hint();
            /// The PROPERTY_HINT_ENUM string of RouteState
            static String route_state_hint();

            static SignallingServer *get_instance() {
                return singleton;
            }

        private:
            static SignallingServer *singleton;

            struct SignalHeadData {
                    RID instance;
                    StringName name;
                    RID system;
                    Ref<SignalHeadKind> kind;
                    int light_count = 0; // the model's, known once its instance is built
                    LightState light_states[MAX_LIGHTS] = {};
                    StringName aspect;
            };

            struct SourceData {
                    StringName name;
                    RID system;
                    /// The TrackServer isolated section it reports, by the section's name
                    StringName isolated;
            };

            struct RouteData {
                    StringName name;
                    RID system;
                    RouteKind kind = ROUTE_KIND_TRAIN;
                    RID start_signal_head;
                    /// None where the route ends at the station's boundary or a W5 sign
                    RID end_signal_head;
                    /// Sources of the route's sections: the path, the overlap behind its end, the approach
                    Vector<RID> sections;
                    Vector<RID> overlap_sections;
                    Vector<RID> approach_sections;
                    /// The track each switch of the route, by its name, has to lie on
                    HashMap<StringName, TrackServer::SwitchTrack> switch_positions;
                    Vector<RID> conflicts;
                    RouteState state = ROUTE_STATE_IDLE;
                    /// The sections still held, released one by one behind the train
                    Vector<RID> locked_sections;
            };

            struct SystemData {
                    StringName name;
                    Ref<SignallingImplementation> implementation;
                    Vector<RID> signal_heads;
                    Vector<RID> sources;
                    /// TrackServer's switches by name, resolved when used
                    Vector<StringName> switch_names;
                    Vector<RID> linked_systems;
                    Vector<RID> routes;
            };

            HashMap<RID, SignalHeadData> signal_heads;
            HashMap<RID, SourceData> sources;
            HashMap<RID, SystemData> systems;
            HashMap<RID, RouteData> routes;
            HashMap<StringName, RID> systems_by_switch_name;
            HashMap<StringName, RID> routes_by_name;
            HashMap<StringName, RID> signal_heads_by_name;
            HashMap<StringName, RID> sources_by_name;
            /// The source of each isolated section a source reports - one per section
            HashMap<StringName, RID> sources_by_isolated;
            HashMap<StringName, RID> systems_by_name;
            HashMap<RID, RID> signal_heads_by_instance;

            void _on_instance_freed(const RID &p_instance);
            void _on_instance_built(const RID &p_instance);
            void _set_light_state(const RID &p_signal_head, SignalHeadData &p_data, int p_light, LightState p_state);
            void _on_switch_movement_finished(const RID &p_switch, int p_active_track);
            void _on_switch_created(const RID &p_switch);
            void _on_switch_freed(const RID &p_switch, const String &p_name);
            /// What an isolated section reports, sent on as its source's p_event
            void _on_isolated_changed(const RID &p_isolated, const RID &p_vehicle, const StringName &p_event);
            /// A source joined to both a system and a section that is occupied already: the section's
            /// state is sent at once, as it changed before anything could hear it
            void _send_isolated_occupied(const RID &p_source);
            /// The implementation of the system the switch of the name belongs to, null when none
            Ref<SignallingImplementation> _switch_implementation(const StringName &p_switch, RID &p_r_system) const;
            /// Drops the element from the definition of every route that names it
            void _forget_in_routes(const RID &p_element);
            /// The implementation of the route's system, null when it has none
            Ref<SignallingImplementation> _route_implementation(const RID &p_route) const;

        protected:
            static void _bind_methods();

        public:
            SignallingServer();
            ~SignallingServer() override;

            RID signal_head_create(const RID &p_instance);
            void signal_head_free(const RID &p_signal_head);
            void signal_head_set_name(const RID &p_signal_head, const StringName &p_name);
            StringName signal_head_get_name(const RID &p_signal_head) const;
            RID signal_head_get_rid_by_name(const StringName &p_name) const;
            TypedArray<RID> signal_head_get_rids() const;
            /// The model instance (E3DRenderingServer) whose lights the signal head is
            RID signal_head_get_instance(const RID &p_signal_head) const;
            RID signal_head_get_system(const RID &p_signal_head) const;
            void signal_head_light_enable(const RID &p_signal_head, int p_light);
            void signal_head_light_disable(const RID &p_signal_head, int p_light);
            /// On for p_on_time seconds, off for p_off_time, the cycle shifted by p_phase seconds
            void signal_head_light_blink(
                    const RID &p_signal_head, int p_light, float p_on_time, float p_off_time, float p_phase);
            LightState signal_head_get_light_state(const RID &p_signal_head, int p_light) const;
            int signal_head_get_light_count(const RID &p_signal_head) const;
            void signal_head_set_kind(const RID &p_signal_head, const Ref<SignalHeadKind> &p_kind);
            Ref<SignalHeadKind> signal_head_get_kind(const RID &p_signal_head) const;
            /// The aspects the signal head's kind can show, empty without a kind
            PackedStringArray signal_head_get_aspects(const RID &p_signal_head) const;
            /// Lights the aspect the way the signal head's kind describes it
            void signal_head_set_aspect(const RID &p_signal_head, const StringName &p_aspect);
            StringName signal_head_get_aspect(const RID &p_signal_head) const;

            RID system_create();
            void system_free(const RID &p_system);
            void
            system_attach_implementation(const RID &p_system, const Ref<SignallingImplementation> &p_implementation);
            Ref<SignallingImplementation> system_get_implementation(const RID &p_system) const;
            void system_set_name(const RID &p_system, const StringName &p_name);
            StringName system_get_name(const RID &p_system) const;
            RID system_get_rid_by_name(const StringName &p_name) const;
            void system_add_signal_head(const RID &p_system, const RID &p_signal_head);
            void system_remove_signal_head(const RID &p_system, const RID &p_signal_head);
            TypedArray<RID> system_get_signal_heads(const RID &p_system) const;
            void system_add_source(const RID &p_system, const RID &p_source);
            void system_remove_source(const RID &p_system, const RID &p_source);
            TypedArray<RID> system_get_sources(const RID &p_system) const;
            void system_send_event(const RID &p_system, const StringName &p_event, const Dictionary &p_arguments);
            /// A TrackServer switch of the system's interlocking area, by its name
            /// (TrackServer.switch_get_rid_by_name()); it belongs to one system at most and may not
            /// exist yet. Its movements and its coming and going reach the system's implementation.
            void system_add_switch(const RID &p_system, const StringName &p_switch);
            void system_remove_switch(const RID &p_system, const StringName &p_switch);
            PackedStringArray system_get_switches(const RID &p_system) const;
            RID switch_get_system(const StringName &p_switch) const;
            /// Neighbouring systems (the stations a line block runs between); the link goes both ways
            void system_link(const RID &p_system, const RID &p_other);
            void system_unlink(const RID &p_system, const RID &p_other);
            TypedArray<RID> system_get_linked_systems(const RID &p_system) const;
            /// A route belongs to one system at most - the one whose implementation sets it
            void system_add_route(const RID &p_system, const RID &p_route);
            void system_remove_route(const RID &p_system, const RID &p_route);
            TypedArray<RID> system_get_routes(const RID &p_system) const;

            RID source_create();
            void source_free(const RID &p_source);
            void source_set_name(const RID &p_source, const StringName &p_name);
            StringName source_get_name(const RID &p_source) const;
            RID source_get_rid_by_name(const StringName &p_name) const;
            RID source_get_system(const RID &p_source) const;
            void source_send_event(const RID &p_source, const StringName &p_event, const Dictionary &p_arguments);
            /// The source reports the TrackServer isolated section of the name: its system's
            /// implementation gets SOURCE_OCCUPIED_EVENT and SOURCE_FREED_EVENT as the section's
            /// axles come and go - held by name, so the section may come later than the source
            void source_set_isolated(const RID &p_source, const StringName &p_isolated);
            StringName source_get_isolated(const RID &p_source) const;

            RID route_create();
            void route_free(const RID &p_route);
            void route_set_name(const RID &p_route, const StringName &p_name);
            StringName route_get_name(const RID &p_route) const;
            RID route_get_rid_by_name(const StringName &p_name) const;
            RID route_get_system(const RID &p_route) const;
            void route_set_kind(const RID &p_route, RouteKind p_kind);
            RouteKind route_get_kind(const RID &p_route) const;
            void route_set_start_signal_head(const RID &p_route, const RID &p_signal_head);
            RID route_get_start_signal_head(const RID &p_route) const;
            void route_set_end_signal_head(const RID &p_route, const RID &p_signal_head);
            RID route_get_end_signal_head(const RID &p_route) const;
            /// The sources of the route's path, in the order it is driven
            void route_set_sections(const RID &p_route, const TypedArray<RID> &p_sources);
            TypedArray<RID> route_get_sections(const RID &p_route) const;
            /// The sources of the overlap behind the route's end signal head
            void route_set_overlap_sections(const RID &p_route, const TypedArray<RID> &p_sources);
            TypedArray<RID> route_get_overlap_sections(const RID &p_route) const;
            /// The sources before the start signal head whose occupation locks the route
            void route_set_approach_sections(const RID &p_route, const TypedArray<RID> &p_sources);
            TypedArray<RID> route_get_approach_sections(const RID &p_route) const;
            /// The track the switch of the name has to lie on for the route
            void route_set_switch_position(
                    const RID &p_route, const StringName &p_switch, TrackServer::SwitchTrack p_switch_track);
            void route_remove_switch_position(const RID &p_route, const StringName &p_switch);
            /// switch name -> TrackServer::SwitchTrack
            Dictionary route_get_switch_positions(const RID &p_route) const;
            /// A special exclusion: the routes may not be set together; it goes both ways
            void route_add_conflict(const RID &p_route, const RID &p_other);
            void route_remove_conflict(const RID &p_route, const RID &p_other);
            TypedArray<RID> route_get_conflicts(const RID &p_route) const;
            /// Called by the route's system's implementation
            void route_set_state(const RID &p_route, RouteState p_state);
            RouteState route_get_state(const RID &p_route) const;
            /// The train left the section: the route no longer holds it
            void route_release_section(const RID &p_route, const RID &p_source);
            TypedArray<RID> route_get_locked_sections(const RID &p_route) const;
            /// Requests handed to the implementation of the route's system, which decides
            void route_request_set(const RID &p_route);
            void route_request_cancel(const RID &p_route);
            void route_request_release(const RID &p_route);
            void route_request_emergency_release(const RID &p_route);
    };
} // namespace godot

VARIANT_ENUM_CAST(SignallingServer::LightState)
VARIANT_ENUM_CAST(SignallingServer::SourcePolarity)
VARIANT_ENUM_CAST(SignallingServer::RouteKind)
VARIANT_ENUM_CAST(SignallingServer::RouteState)
