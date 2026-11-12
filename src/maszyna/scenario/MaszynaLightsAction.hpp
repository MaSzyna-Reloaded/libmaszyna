#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `lights` event (lights_event, Event.cpp:1741-1803). The event is an aspect of
    /// each signal head it is aimed at (MaszynaSignalHeadKindFactory); this hands it to the
    /// signal head's system, whose MaszynaSignallingImplementation shows it. A light the event sets
    /// to follow the daylight (ls_Dark, ls_Home) the aspect keeps as it is; that one is set on the
    /// model drawing the signal head, as TAnimModel::LightSet() sets it (AnimModel.cpp:664-670).
    class MaszynaLightsAction : public ScenarioEventAction {
            GDCLASS(MaszynaLightsAction, ScenarioEventAction)

        private:
            TypedArray<RID> signal_heads;
            /// The aspect each of the signal heads shows, in their order
            TypedArray<StringName> aspects;
            /// The lights the event sets to follow the daylight, with their LIGHT_MODE_DARK/HOME
            /// (MaszynaE3DRenderingServer::LightMode) and the light level they come on at
            PackedInt32Array daylight_lights;
            PackedInt32Array daylight_modes;
            PackedFloat32Array daylight_thresholds;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_signal_heads(const TypedArray<RID> &p_signal_heads);
            TypedArray<RID> get_signal_heads() const;
            void set_aspects(const TypedArray<StringName> &p_aspects);
            TypedArray<StringName> get_aspects() const;
            /// The event's list of lights, read once (MaszynaLightMode::parse()): the ones it sets to
            /// follow the daylight are kept
            void read_lights(const PackedFloat32Array &p_lights);
            PackedInt32Array get_daylight_lights() const;
            PackedInt32Array get_daylight_modes() const;
            PackedFloat32Array get_daylight_thresholds() const;
    };
} // namespace godot
