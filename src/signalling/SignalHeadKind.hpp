#pragma once
#include "SignalAspect.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    /// What a kind of signal head can show: its aspects by name. A signal head is given its kind with
    /// SignallingServer.signal_head_set_kind(); the aspects are data, not code, so a new kind of
    /// signal head is a new resource.
    class SignalHeadKind : public Resource {
            GDCLASS(SignalHeadKind, Resource)

        public:
            /// The original's default blinking (fOnTime/fOffTime, AnimModel.h:208-209), for an aspect
            /// that gives no times of its own
            static constexpr float DEFAULT_BLINK_TIME = 0.5;

        private:
            Dictionary aspects;
            float blink_on_time = DEFAULT_BLINK_TIME;
            float blink_off_time = DEFAULT_BLINK_TIME;

        protected:
            static void _bind_methods();

        public:
            void set_aspects(const Dictionary &p_aspects);
            Dictionary get_aspects() const;
            void set_blink_on_time(float p_time);
            float get_blink_on_time() const;
            void set_blink_off_time(float p_time);
            float get_blink_off_time() const;

            /// The aspect names, in the order they are declared
            PackedStringArray get_aspect_names() const;
            bool has_aspect(const StringName &p_aspect) const;
            Ref<SignalAspect> get_aspect(const StringName &p_aspect) const;
    };
} // namespace godot
