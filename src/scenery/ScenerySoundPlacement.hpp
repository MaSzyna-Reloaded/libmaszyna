#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// A sound a SceneryStreamingProvider places (SceneryStreamingProvider.CONTENT_SOUNDS): looped
    /// at its place while its chunk is supplied and the listener is within its reach, and named by
    /// nothing - no event plays or stops it
    class ScenerySoundPlacement : public Resource {
            GDCLASS(ScenerySoundPlacement, Resource)

        private:
            Ref<AudioStream> stream;
            Vector3 position;
            float reach = 0.0F;
            float volume_db = 0.0F;

        protected:
            static void _bind_methods();

        public:
            /// What is looped - a stream that loops itself
            void set_stream(const Ref<AudioStream> &p_value);
            Ref<AudioStream> get_stream() const;
            void set_position(const Vector3 &p_value);
            Vector3 get_position() const;
            /// How far it is heard [m]
            void set_reach(float p_value);
            float get_reach() const;
            void set_volume_db(float p_value);
            float get_volume_db() const;
    };
} // namespace godot
