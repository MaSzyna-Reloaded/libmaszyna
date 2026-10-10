#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace godot {
    /// One aspect of a SignalHeadKind: what each light does when the signal head shows it, light 0
    /// first. A light past the list, or one set to LIGHT_KEEP, is left as it is. Blink times missing
    /// from their lists are the kind's.
    class SignalAspect : public Resource {
            GDCLASS(SignalAspect, Resource)

        public:
            enum LightCommand {
                LIGHT_OFF,
                LIGHT_ON,
                LIGHT_BLINK,
                LIGHT_KEEP,
            };

        private:
            PackedInt32Array lights;
            PackedFloat32Array on_times;
            PackedFloat32Array off_times;
            PackedFloat32Array phases;

        protected:
            static void _bind_methods();

        public:
            void set_lights(const PackedInt32Array &p_lights);
            PackedInt32Array get_lights() const;
            void set_on_times(const PackedFloat32Array &p_times);
            PackedFloat32Array get_on_times() const;
            void set_off_times(const PackedFloat32Array &p_times);
            PackedFloat32Array get_off_times() const;
            void set_phases(const PackedFloat32Array &p_phases);
            PackedFloat32Array get_phases() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(SignalAspect::LightCommand)
