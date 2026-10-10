#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `animation` event, `rotate` and `translate` (animation_event,
    /// Event.cpp:1569-1716): moves a submodel of its model instances towards the given angles or
    /// offset at a speed. `digital` and `.vmd` animations are not ported (TODO.md).
    class MaszynaLegacyAnimationAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyAnimationAction, ScenarioEventAction)

        public:
            enum Mode {
                /// Degrees about the submodel's x, y and z
                MODE_ROTATE,
                /// Metres
                MODE_TRANSLATE,
            };

        private:
            TypedArray<RID> instances;
            String submodel;
            Mode mode = MODE_ROTATE;
            Vector3 target;
            double speed = 0.0;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            /// E3DRenderingServer instances
            void set_instances(const TypedArray<RID> &p_instances);
            TypedArray<RID> get_instances() const;
            void set_submodel(const String &p_submodel);
            String get_submodel() const;
            void set_mode(Mode p_mode);
            Mode get_mode() const;
            void set_target(const Vector3 &p_target);
            Vector3 get_target() const;
            /// Degrees or metres per second
            void set_speed(double p_speed);
            double get_speed() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaLegacyAnimationAction::Mode)
