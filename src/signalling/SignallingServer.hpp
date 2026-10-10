#pragma once
#include "SignalHeadKind.hpp"
#include "SignallingImplementation.hpp"
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

            /// iMaxNumLights, AnimModel.h:23
            static constexpr int MAX_LIGHTS = 8;

            static const char *signal_head_registered_signal;
            static const char *signal_head_unregistered_signal;
            static const char *signal_head_light_state_changed_signal;
            static const char *signal_head_aspect_changed_signal;
            static const char *signal_head_config_changed_signal;
            static const char *system_signal_head_added_signal;
            static const char *system_signal_head_removed_signal;
            static const char *system_source_added_signal;
            static const char *system_source_removed_signal;

            /// The PROPERTY_HINT_ENUM string of LightState
            static String light_state_hint();

            static SignallingServer *get_instance() {
                return Object::cast_to<SignallingServer>(Engine::get_singleton()->get_singleton("SignallingServer"));
            }

        private:
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
            };

            struct SystemData {
                    StringName name;
                    Ref<SignallingImplementation> implementation;
                    Vector<RID> signal_heads;
                    Vector<RID> sources;
            };

            HashMap<RID, SignalHeadData> signal_heads;
            HashMap<RID, SourceData> sources;
            HashMap<RID, SystemData> systems;
            HashMap<StringName, RID> signal_heads_by_name;
            HashMap<StringName, RID> sources_by_name;
            HashMap<StringName, RID> systems_by_name;
            HashMap<RID, RID> signal_heads_by_instance;

            void _on_instance_freed(const RID &p_instance);
            void _on_instance_built(const RID &p_instance);
            void _set_light_state(const RID &p_signal_head, SignalHeadData &p_data, int p_light, LightState p_state);

        protected:
            static void _bind_methods();

        public:
            SignallingServer();

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

            RID source_create();
            void source_free(const RID &p_source);
            void source_set_name(const RID &p_source, const StringName &p_name);
            StringName source_get_name(const RID &p_source) const;
            RID source_get_rid_by_name(const StringName &p_name) const;
            RID source_get_system(const RID &p_source) const;
            void source_send_event(const RID &p_source, const StringName &p_event, const Dictionary &p_arguments);
    };
} // namespace godot

VARIANT_ENUM_CAST(SignallingServer::LightState)
